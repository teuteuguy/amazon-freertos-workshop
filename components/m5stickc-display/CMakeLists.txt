# include paths of this components.
set(COMPONENT_ADD_INCLUDEDIRS . src include util)

# source files of this components.
set(COMPONENT_SRCDIRS src util util/font)

# add this components, this will define a CMake library target.
register_component()

