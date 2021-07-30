# Antilatency Alt Tracking Minimal Demo C++

## Overview
This project provides minimal demo of Antilatency tracking.
Structure: 
./AntilatencySdk/Api contains headers for Antilatency libraries;
./AntilatencySdk/Bin/<os>/<architecture> contains prebuilt libraries;
./TrackingMinimalDemoCpp.cpp contains code of example.

## Build on raspberry:
1) Place project folder somewhere on the filesystem.
2) Execute in terminal:
  * `cd <full path to project directory>`
  * `mkdir build`
  * `cd build`
  * `cmake ../`
  * `make`
  
3) Directory ./build should now contain TrackingMinimalDemo executable and libraries.

## Build on windows:
1) Open cmake-gui. 
2) Set path to project directory in "Where is the source code"
3) Specify directory where you want to create project files in "Where to build the binaries"
4) Click "Configure" button.
5) Choose "generator"(e.g. Visual Studio).
6) Click "Generate" button.
7) Directory specified on step 3 should now contain project files depending on selected generator(TrackingMinimalDemo.sln for Visual Studio)
8) Build it.

## Run
TrackingMinimalDemo executable expects environment code as first argument and placement code as second(both can be obtained from AntilatencyService by Share->copy link in environment/placement menu). 

### Windows example(execute in cmd):
`TrackingMinimalDemo.exe AntilatencyAltEnvironmentHorizontalGrid~AgZ5ZWxsb3cEBLhTiT_cRqA-r45jvZqZmT4AAAAAAAAAAACamRk_AQQAAQEBAwABAAADAQE AAAAAAAAAAAAAAAAAAAAAIAAAAAAAAAAAA`


### Linux example(execute in terminal): 
`./TrackingMinimalDemo AntilatencyAltEnvironmentHorizontalGrid~AgZ5ZWxsb3cEBLhTiT_cRqA-r45jvZqZmT4AAAAAAAAAAACamRk_AQQAAQEBAwABAAADAQE AAAAAAAAAAAAAAAAAAAAAIAAAAAAAAAAAA`


