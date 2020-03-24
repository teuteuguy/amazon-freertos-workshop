set(module_name "foo")

add_library(${module_name})

target_sources(
    ${module_name} 
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/src/foo.c"
)

target_include_directories(
    ${module_name}
    PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}/include"
)

target_link_libraries(
    ${module_name} 
    PRIVATE
        AFR::mqtt
)