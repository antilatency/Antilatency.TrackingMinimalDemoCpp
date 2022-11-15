#include <iostream>

#include <Antilatency.InterfaceContract.LibraryLoader.h>
#include <Antilatency.DeviceNetwork.h>
#if defined(__linux__)
	#include <dlfcn.h>
#endif
#include <thread>
#include <chrono>

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


#if defined(__linux__)
std::string getParentPath(const char *inp){
    auto len = strlen(inp);
    if(len == 0) throw std::runtime_error("no parent path: " + std::string(inp));
    int i = len - 1;
    while(i > 0){
        if(inp[i] == '/'){
            return std::string(inp, inp + i + 1);
        }
        --i;
    }
    throw std::runtime_error("no parent path: " + std::string(inp));
}
#endif

int main(int argc, char* argv[]) {
    if(argc != 3){
        std::cout << "Wrong arguments. Pass environment data string as first argument and placement data as second.";
        return 1;
    }
    #if defined(__linux__)
        Dl_info dlinfo;
        dladdr(reinterpret_cast<void*>(&main), &dlinfo);
        std::string path = getParentPath(dlinfo.dli_fname);
        std::string libNameADN = path + "/libAntilatencyDeviceNetwork.so";
        std::string libNameTracking = path + "/libAntilatencyAltTracking.so";
        std::string libNameEnvironmentSelector = path + "/libAntilatencyAltEnvironmentSelector.so";
    #else
        std::string libNameADN = "AntilatencyDeviceNetwork";
        std::string libNameTracking = "AntilatencyAltTracking";
        std::string libNameEnvironmentSelector = "AntilatencyAltEnvironmentSelector";
    #endif

    // Load the Antilatency Device Network library
    Antilatency::DeviceNetwork::ILibrary deviceNetworkLibrary = Antilatency::InterfaceContract::getLibraryInterface<Antilatency::DeviceNetwork::ILibrary>(libNameADN.c_str());
    if (deviceNetworkLibrary == nullptr) {
        std::cout << "Failed to get Antilatency Device Network Library" << std::endl;
        return 1;
    }

    // Load the Antilatency Alt Tracking library
    Antilatency::Alt::Tracking::ILibrary altTrackingLibrary = Antilatency::InterfaceContract::getLibraryInterface<Antilatency::Alt::Tracking::ILibrary>(libNameTracking.c_str());
    if (altTrackingLibrary == nullptr) {
        std::cout << "Failed to get Antilatency Alt Tracking Library" << std::endl;
        return 1;
    }

    // Load the Antilatency Alt Environment Selector library
    Antilatency::Alt::Environment::Selector::ILibrary environmentSelectorLibrary = Antilatency::InterfaceContract::getLibraryInterface<Antilatency::Alt::Environment::Selector::ILibrary>(libNameEnvironmentSelector.c_str());
    if (environmentSelectorLibrary == nullptr) {
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

    // Get environment serialized data.
    const std::string environmentData = "AntilatencyAltEnvironmentHorizontalGrid~AgdjZWlsaW5nCwsIBV8_zczMPiaOzr2amZk-AAAAAAEK13NAmpkZPwAQCgIDCQMDBgAABgIAAwMDAgACAAMDAAcDAgoCBQoCCAoABwgDCgcDBgYDBAUCAwcC";
    // Get placement serialized data.
    const std::string placementData = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";

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

                        std::cout << "\tLocalAngularVelocity: x:" << state.localAngularVelocity.x << ", y: " << state.localAngularVelocity.y << ", z: " << state.localAngularVelocity.z << std::endl << std::endl;
                        std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(500));
                    }
                } else {
                    std::cout << "Failed to start tracking task on node" << std::endl;
                }
            }
        }else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    return 0;
}
