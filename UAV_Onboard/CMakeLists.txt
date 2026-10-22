cmake_minimum_required(VERSION 3.10)

if(WIN32)
    if(NOT CMAKE_TOOLCHAIN_FILE)
        set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
    endif()
    if(NOT VCPKG_TARGET_TRIPLET)
        set(VCPKG_TARGET_TRIPLET "x64-windows" CACHE STRING "Vcpkg triplet")
    endif()
endif()

project(UAV_Onboard)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    if(NOT OpenCV_DIR)
        set(OpenCV_DIR "C:/opencv/build")
    endif()
    if(NOT ORT_DIR)
        set(ORT_DIR "C:/onnxruntime")
    endif()
else()
    if(NOT ORT_DIR)
        message(FATAL_ERROR "ORT_DIR no definido. En Linux, pasa -DORT_DIR=/ruta/a/onnxruntime")
    endif()
endif()

find_package(OpenCV REQUIRED)
find_package(unofficial-mosquitto CONFIG REQUIRED)

include_directories(${ORT_DIR}/include)
link_directories(${ORT_DIR}/lib)

add_executable(UAV_Onboard 
    src/main.cpp 
    src/Detection/OnnxYoloAdapter.cpp
    src/Video/OpenCvVideoSource.cpp
    src/Telemetry/SimulatedTelemetryProvider.cpp
    src/Network/UDPTelemetrySink.cpp
    src/Network/TCPTelemetrySink.cpp
    src/Network/UDPVideoSink.cpp
    src/Network/MqttTelemetrySink.cpp
    src/App/EdgeStreamingService.cpp
)

target_include_directories(UAV_Onboard PRIVATE ${OpenCV_INCLUDE_DIRS} Includes)

if(WIN32)
    target_link_libraries(UAV_Onboard ${OpenCV_LIBS} onnxruntime ws2_32 unofficial::mosquitto::mosquitto)
else()
    target_link_libraries(UAV_Onboard ${OpenCV_LIBS} onnxruntime)
endif()

if(WIN32)
    file(GLOB ORT_DLLS "${ORT_DIR}/lib/*.dll")
    add_custom_command(TARGET UAV_Onboard POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ORT_DLLS} $<TARGET_FILE_DIR:UAV_Onboard>
        COMMENT "Copiando DLLs de ONNX Runtime junto al ejecutable"
    )
endif()

add_custom_command(TARGET UAV_Onboard POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/Models" "$<TARGET_FILE_DIR:UAV_Onboard>/Models"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/config.json" "$<TARGET_FILE_DIR:UAV_Onboard>/config.json"
    COMMENT "Copiando Models/ y config.json junto al ejecutable"
)

if(WIN32)
    set_target_properties(UAV_Onboard PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:UAV_Onboard>"
    )
endif()