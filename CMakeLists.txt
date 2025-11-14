cmake_minimum_required(VERSION 3.10)
project(dpopen C CXX)

set(CMAKE_C_STANDARD 90)
set(CMAKE_CXX_STANDARD 17)

# Thread support
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Some platforms do not use "-pthread" even when multi-threading is
# turned on, but it is actually needed.  The alternative is manually
# define the macro "_REENTRANT".
add_compile_options(-pthread)

# dpopen library
add_library(dpopen STATIC dpopen.c dpopen.h)
target_link_libraries(dpopen PUBLIC Threads::Threads)

# C example
add_executable(example example.c)
target_link_libraries(example dpopen)

# C++ pipeline library
add_library(pipeline STATIC pipeline.cpp pipeline.h)
target_link_libraries(pipeline PUBLIC dpopen)

# C++ example
add_executable(pipeline_example pipeline_example.cpp)
target_link_libraries(pipeline_example pipeline)
