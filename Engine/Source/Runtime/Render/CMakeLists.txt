set(TARGET_NAME VkWrap)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE LIB_SOURCES CONFIGURE_DEPENDS
    "Source/*.cpp"
    "Source/*.cxx"
    "*.cpp"
)

file(GLOB_RECURSE LIB_HEADERS
    "Include/*.h"
    "Include/*.hpp"
)
#message(STATUS "LIB_HEADERS: ${LIB_HEADERS}")

add_library(${TARGET_NAME} STATIC ${LIB_SOURCES} ${LIB_HEADERS})

#Enable experimental glm
target_compile_definitions(${TARGET_NAME} PRIVATE GLM_ENABLE_EXPERIMENTAL)

target_include_directories(${TARGET_NAME} PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/Include/Core
    ${RUNTIME_DIR}/Physical/Include
    ${RUNTIME_DIR}/Physical/Include
    ${RUNTIME_DIR}/Core/Include
    ${THIRD_PARTY_DIR}/stb
    ${THIRD_PARTY_DIR}/ktx/include
    ${THIRD_PARTY_DIR}/tinygltf
    ${THIRD_PARTY_DIR}/imgui
    ${THIRD_PARTY_DIR}/SPIRV-Cross
)

if(DEFINED ENV{VULKAN_SDK})
    message(STATUS "Found Vulkan SDK at: $ENV{VULKAN_SDK}")
    target_include_directories(${TARGET_NAME} PUBLIC "$ENV{VULKAN_SDK}/include")
else()
    message(FATAL_ERROR "VULKAN_SDK environment variable not set!")
endif()

target_include_directories(${TARGET_NAME} PUBLIC 
    ${THIRD_PARTY_DIR}/glm
)

target_link_libraries(${TARGET_NAME} PUBLIC PhysicsEngine volk Core spdlog::spdlog)
target_link_libraries(${TARGET_NAME} PUBLIC glm spirv-cross-glsl spirv-cross-reflect) 
target_link_libraries(${TARGET_NAME} PUBLIC imgui vma)
target_link_libraries(${TARGET_NAME} PUBLIC ktx)


# Organize the folder structure for the IDE
set_property(TARGET ${TARGET_NAME} PROPERTY VS_GLOBAL_DisableExternalDependencies true)

set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Runtime")
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Source PREFIX "Source" FILES ${LIB_SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Include PREFIX "Include" FILES ${LIB_HEADERS})

set_property(GLOBAL PROPERTY DEFAULT_PROJECT ${TARGET_NAME})