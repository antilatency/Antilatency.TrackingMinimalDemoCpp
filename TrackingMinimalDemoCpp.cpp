#include <iostream>

#include <Antilatency.InterfaceContract.LibraryLoader.h>
#include <Antilatency.DeviceNetwork.h>


Antilatency::DeviceNetwork::NodeHandle getIdleTrackingNode(Antilatency::DeviceNetwork::INetwork network, Antilatency::Alt::Tracking::ITrackingCotaskConstructor altTrackingCotaskConstructor) {
    // Get all currently connected nodes, that supports alt tracking task.
    std::vector<Antilatency::DeviceNetwork::NodeHandle> altNodes = altTrackingCotaskConstructor.findSupportedNodes(network);
    if (altNodes.size() == 0) {
        std::cout << "No nodes with Alt Tracking Task support found" << std::endl;
        return Antilatency::DeviceNetwork::NodeHandle::Null;
    }

    // Return first idle node.
    for (auto node : altNodes) {
        if (network.nodeGetStatus(node) == Antilatency::DeviceNetwork::NodeStatus::Idle) {
            return node;
        }
    }

    std::cout << "No idle nodes with Alt Tracking Task support found" << std::endl;
    return Antilatency::DeviceNetwork::NodeHandle::Null;
}

int main(int argc, char* argv[]) {
    // Load the Antilatency Device Network library
    Antilatency::DeviceNetwork::ILibrary deviceNetworkLibrary = Antilatency::InterfaceContract::getLibraryInterface<Antilatency::DeviceNetwork::ILibrary>("AntilatencyDeviceNetwork");
    if (deviceNetworkLibrary == nullptr) {
        std::cout << "Failed to get Antilatency Device Network Library" << std::endl;
        return 1;
    }

    // Load the Antilatency Alt Tracking library
    Antilatency::Alt::Tracking::ILibrary altTrackingLibrary = Antilatency::InterfaceContract::getLibraryInterface<Antilatency::Alt::Tracking::ILibrary>("AntilatencyAltTracking");
    if (altTrackingLibrary == nullptr) {
        std::cout << "Failed to get Antilatency Alt Tracking Library" << std::endl;
        return 1;
    }

    // Load the Antilatency Storage Client library
    Antilatency::StorageClient::ILibrary storageClientLibrary = Antilatency::InterfaceContract::getLibraryInterface<Antilatency::StorageClient::ILibrary>("AntilatencyStorageClient");
    if (altTrackingLibrary == nullptr) {
        std::cout << "Failed to get Antilatency Storage Client Library" << std::endl;
        return 1;
    }

    // Load the Antilatency Alt Environment Selector library
    Antilatency::Alt::Environment::Selector::ILibrary environmentSelectorLibrary = Antilatency::InterfaceContract::getLibraryInterface<Antilatency::Alt::Environment::Selector::ILibrary>("AntilatencyAltEnvironmentSelector");
    if (altTrackingLibrary == nullptr) {
        std::cout << "Failed to get Antilatency Alt Environment Selector Library" << std::endl;
        return 1;
    }

    // Create a device network filter and then create a network using that filter.
    Antilatency::DeviceNetwork::IDeviceFilter filter = deviceNetworkLibrary.createFilter();
    filter.addUsbDevice(Antilatency::DeviceNetwork::Constants::AllUsbDevices);
    Antilatency::DeviceNetwork::INetwork network = deviceNetworkLibrary.createNetwork(filter);
    if (network == nullptr) {
        std::cout << "Failed to create Antilatency Device Network" << std::endl;
        return 1;
    }

    std::cout << "Antilatency Device Network created" << std::endl;

    // Get local storage. You can skip this step using the hardcoded environment and placement codes.
    Antilatency::StorageClient::IStorage localStorage = storageClientLibrary.getLocalStorage();
    if (localStorage == nullptr) {
        std::cout << "Failed to get local storage. Is Antilatency Service installed?" << std::endl;
        return 1;
    }

    // Get environment serialized data from the storage.
    const std::string environmentData = localStorage.read("environment", "default");
    // Get placement serialized data from the storage.
    const std::string placementData = localStorage.read("placement", "default");

    // Create environment object from the serialized data.
    const Antilatency::Alt::Environment::IEnvironment environment = environmentSelectorLibrary.createEnvironment(environmentData);
    if (environment == nullptr) {
        std::cout << "Failed to create environment" << std::endl;
        return 1;
    }

    // Create placement from the serialized data.
    const Antilatency::Math::floatP3Q placement = altTrackingLibrary.createPlacement(placementData);

    // Create alt tracking cotask constructor to find tracking-supported nodes and start tracking task on node.
    Antilatency::Alt::Tracking::ITrackingCotaskConstructor altTrackingCotaskConstructor = altTrackingLibrary.createTrackingCotaskConstructor();
    if (altTrackingCotaskConstructor == nullptr) {
        std::cout << "Failed to create Antilatency Alt Tracking Cotask Constructor" << std::endl;
        return 1;
    }

    // Each time the device network is changed due to connection or disconnection of a device that matches the device filter of the network,
    // or start or stop of a task on any network device, the network update id is incremented by 1. 
    uint32_t prevUpdateId = 0;

    while (network != nullptr) {
        // Check if the network has been changed.
        const uint32_t currentUpdateId = network.getUpdateId();

        if (prevUpdateId != currentUpdateId) {
            prevUpdateId = currentUpdateId;
            std::cout << "--- Device network changed, update id: " << currentUpdateId << " ---" << std::endl;

            // Get first idle node that supports tracking task.
            const Antilatency::DeviceNetwork::NodeHandle trackingNode = getIdleTrackingNode(network, altTrackingCotaskConstructor);

            if (trackingNode != Antilatency::DeviceNetwork::NodeHandle::Null) {
                // Start tracking task on node.
                Antilatency::Alt::Tracking::ITrackingCotask altTrackingCotask = altTrackingCotaskConstructor.startTask(network, trackingNode, environment);
                if (altTrackingCotask != nullptr) {
                    while (altTrackingCotask != nullptr) {
                        // Print the extrapolated state of node to the console every 500 ms (2FPS).
                        if (altTrackingCotask.isTaskFinished()) {
                            std::cout << "Tracking task finished" << std::endl;
                            break;
                        }
                        
                        Antilatency::Alt::Tracking::State state = altTrackingCotask.getExtrapolatedState(placement, 0.03f);

                        std::cout << "State:" << std::endl;
                        
                        std::cout << "\tPose:" << std::endl;
                        std::cout << "\t\tPosition: x: " << state.pose.position.x << ", y: " << state.pose.position.y << ", z: " << state.pose.position.z << std::endl;
                        std::cout << "\t\tRotation: x: " << state.pose.rotation.x << ", y: " << state.pose.rotation.y << ", z: " << state.pose.rotation.z  << ", w: " << state.pose.rotation.w << std::endl;
                        
                        std::cout << "\tStability:" << std::endl;
                        std::cout << "\t\tStage: " << static_cast<int32_t>(state.stability.stage) << std::endl;
                        std::cout << "\t\tValue: " << state.stability.value << std::endl;
                        
                        std::cout << "\tVelocity:" << state.velocity.x << ", y: " << state.velocity.y << ", z: " << state.velocity.z << std::endl;

                        std::cout << "\tLocalAngularVelocity:" << state.localAngularVelocity.x << ", y: " << state.localAngularVelocity.y << ", z: " << state.localAngularVelocity.z << std::endl << std::endl;
                        
                        Sleep(500);
                    }
                } else {
                    std::cout << "Failed to start tracking task on node" << std::endl;
                }
            }
        }
        Yield();
    }
    
    return 0;
}
