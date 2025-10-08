set(TARGET_NAME Core)

Include(${CMAKE_DIR}/LibBase.cmake)


target_link_libraries(${TARGET_NAME} PUBLIC spdlog::spdlog)

set_property(TARGET ${TARGET_NAME} PROPERTY VS_GLOBAL_DisableExternalDependencies true)

set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Runtime")
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Source PREFIX "Source" FILES ${LIB_SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Include PREFIX "Include" FILES ${LIB_HEADERS})