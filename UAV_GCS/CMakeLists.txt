cmake_minimum_required(VERSION 3.15)

if(NOT CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
endif()
if(NOT VCPKG_TARGET_TRIPLET)
    set(VCPKG_TARGET_TRIPLET "x64-windows" CACHE STRING "Vcpkg triplet")
endif()

project(UAV_GCS)

list(APPEND CMAKE_PREFIX_PATH "C:/Qt/6.11.2/msvc2022_64")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)
find_package(unofficial-mosquitto CONFIG REQUIRED)

add_executable(UAV_GCS 
    "src/main.cpp"
    "src/UI/ConnectionWindow.cpp"
    "src/Networks/UDPReceiver.cpp"
    "src/UI/DashboardWindow.cpp"
    "Includes/UI/ConnectionWindow.hpp"   
    "Includes/Networks/UDPReceiver.hpp" 
    "Includes/UI/DashboardWindow.hpp"
    "Includes/Ports/IReceiver.hpp"  
    "Includes/Ports/IVideoReceiver.hpp"
    "Includes/Networks/UDPVideoReceiver.hpp"
    "src/Networks/UDPVideoReceiver.cpp"
    "Includes/Domain/Protocol.hpp"
    "Includes/Domain/Common.hpp"
    "Resources/resources.qrc"
    "Includes/Networks/TCPReceiver.hpp"
    "src/Networks/TCPReceiver.cpp"
    "src/Networks/MQTTReceiver.cpp"
    "Includes/Networks/MQTTReceiver.hpp"
) 
 
target_include_directories(UAV_GCS PRIVATE Includes)
target_link_libraries(UAV_GCS PRIVATE Qt6::Widgets Qt6::Network unofficial::mosquitto::mosquitto)