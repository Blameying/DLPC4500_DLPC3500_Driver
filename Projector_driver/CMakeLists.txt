project(projector)

add_subdirectory(hidapi-master)

add_library(${PROJECT_NAME} STATIC
    dlpc350_api.cpp
    dlpc350_BMPParser.cpp
    dlpc350_common.cpp
    dlpc350_firmware.cpp
    dlpc350_usb.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC ./ hidapi-master/hidapi)
target_link_libraries(${PROJECT_NAME} hidapi_include hidapi::hidapi)