add_library(glad STATIC include/glad/glad.h src/glad.c)
target_include_directories(glad PRIVATE include)