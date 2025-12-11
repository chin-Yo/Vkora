set(TARGET_NAME VkoraEditor)

Include(${CMAKE_DIR}/LibBase.cmake)

target_include_directories(${TARGET_NAME} PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/Include
    ${RUNTIME_DIR}
    ${EDITOR_DIR}
    ${RUNTIME_DIR}/Core/Inlcude
)

target_link_libraries(${TARGET_NAME} PUBLIC Engine eventpp)


set_property(TARGET ${TARGET_NAME} PROPERTY VS_GLOBAL_DisableExternalDependencies true)

set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Editor")
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Source PREFIX "Source" FILES ${LIB_SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Include PREFIX "Include" FILES ${LIB_HEADERS})