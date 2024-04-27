cmake_minimum_required(VERSION 3.5) 
project(clox) 
#
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/") # .exe and .dll 
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib") # .so and .dylib 
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib") # .lib and .a 
#
add_executable(clox src/main.cpp) 
#
add_custom_command( 
   TARGET clox POST_BUILD 
   COMMAND echo ***************** 
   COMMAND echo Running output...  
   COMMAND echo ***************** 
   COMMAND clox  
   VERBATIM)  