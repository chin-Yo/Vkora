set(TARGET_NAME Launch)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


file(GLOB_RECURSE LIB_HEADERS
    "Include/*.h"
    "Include/*.hpp"
)

set(LIB_SOURCES
    Source/Launch.cpp
    Source/LaunchEngineLoop.cpp
)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    file(GLOB PLATFORM_SOURCES "Source/Linux/*.cpp")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin") # macOS == Darwin
    file(GLOB PLATFORM_SOURCES "Source/MacOS/*.cpp")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    file(GLOB PLATFORM_SOURCES "Source/Windows/*.cpp")
else()
    message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME}")
endif()

list(APPEND LIB_SOURCES ${PLATFORM_SOURCES})

add_executable(${TARGET_NAME} ${LIB_SOURCES} ${LIB_HEADERS})
target_compile_features(${TARGET_NAME} PRIVATE cxx_std_17)

set_source_files_properties(Source/Launch.cpp PROPERTIES
    COMPILE_DEFINITIONS VOLK_IMPLEMENTATION
)

target_include_directories(${TARGET_NAME} PUBLIC 
    ${THIRD_PARTY_DIR}/glfw/include
    ${RUNTIME_DIR}/Launch/Include
)

target_link_libraries(${TARGET_NAME} PUBLIC Engine)


set_property(TARGET ${TARGET_NAME} PROPERTY VS_GLOBAL_DisableExternalDependencies true)

set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Runtime")
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Source PREFIX "Source" FILES ${LIB_SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Include PREFIX "Include" FILES ${LIB_HEADERS})