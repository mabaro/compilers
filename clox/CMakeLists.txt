cmake_minimum_required(VERSION 3.5) 
project(clox) 
#
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/") # .exe and .dll 
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib") # .so and .dylib 
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib") # .lib and .a 
#
set(SOURCES
src/chunk.h
src/common.h
src/compiler.h
src/debug.h
src/debug.cpp
src/scanner.h
src/value.h
src/vm.h)
add_library(clox_lib ${SOURCES})
target_include_directories(clox_lib PRIVATE src)
add_executable(clox src/main.cpp)
target_link_libraries(clox clox_lib)
#
add_custom_command( 
   TARGET clox POST_BUILD 
   COMMAND echo ***************** 
   COMMAND echo Running output...  
   COMMAND echo ***************** 
   COMMAND clox  
   VERBATIM)  
