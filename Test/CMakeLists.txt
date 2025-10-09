set(TARGET_NAME Refl_Test)


add_executable(${TARGET_NAME} Refl_Test.cpp)

target_include_directories(${TARGET_NAME} PUBLIC ${VKORAENGINE_ROOT_DIR}/Engine/Source/Runtime/Core/Include)

set_property(TARGET ${TARGET_NAME} PROPERTY VS_GLOBAL_DisableExternalDependencies true)

set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Tests")
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Source" FILES Refl_Test.cpp)