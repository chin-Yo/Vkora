set(TARGET_NAME CyREngineEditor)

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

add_executable(${TARGET_NAME} ${LIB_SOURCES} ${LIB_HEADERS})
target_compile_features(${TARGET_NAME} PRIVATE cxx_std_17)
target_include_directories(${TARGET_NAME} PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/Include
    ${RUNTIME_DIR}
    ${EDITOR_DIR}
    ${RUNTIME_DIR}/Core/Inlcude
    #${THIRD_PARTY_DIR}/spdlog/include
)

set_source_files_properties(Source/main.cpp PROPERTIES
    COMPILE_DEFINITIONS VOLK_IMPLEMENTATION
)

target_link_libraries(${TARGET_NAME} PRIVATE Application)
target_link_libraries(${TARGET_NAME} PRIVATE glfw)
target_link_libraries(${TARGET_NAME} PRIVATE Core)


set_property(TARGET ${TARGET_NAME} PROPERTY VS_GLOBAL_DisableExternalDependencies true)

set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Editor")
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Source PREFIX "Source" FILES ${LIB_SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Include PREFIX "Include" FILES ${LIB_HEADERS})