cmake_minimum_required(VERSION 3.10)
project(TrackingMinimalDemo)
set(CMAKE_CXX_STANDARD 17)
include_directories(${CMAKE_SOURCE_DIR}/AntilatencySdk/Api)
add_executable(TrackingMinimalDemo TrackingMinimalDemoCpp.cpp)
if (WIN32)
    file(GLOB ANTILATENCY_LIBS ${CMAKE_SOURCE_DIR}/AntilatencySdk/Bin/Windows/x64/*.dll)
endif (WIN32)
if (UNIX)
	target_link_libraries(TrackingMinimalDemo dl stdc++fs pthread)
    file(GLOB ANTILATENCY_LIBS ${CMAKE_SOURCE_DIR}/AntilatencySdk/Bin/RaspberryPi/arm_linux_gnueabihf/*.so)
endif (UNIX)
#file(COPY ${ANTILATENCY_LIBS} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
add_custom_command(TARGET TrackingMinimalDemo POST_BUILD      
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${ANTILATENCY_LIBS}  
        $<TARGET_FILE_DIR:TrackingMinimalDemo>) 