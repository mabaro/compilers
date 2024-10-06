cmake_minimum_required(VERSION 3.5) 
project(clox) 

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    include(CTest)
endif()

#
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/") # .exe and .dll 
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib") # .so and .dylib 
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib") # .lib and .a 
#
set(SOURCES
    src/utils/assert.h
    src/utils/assert.cpp
    src/utils/common.h
    src/utils/common.cpp
    src/chunk.h
    src/chunk.cpp
    src/compiler.h
    src/compiler.cpp
    src/debug.h
    src/debug.cpp
    src/scanner.h
    src/scanner.cpp
    src/value.h
    src/value.cpp
    src/vm.h
    src/vm.cpp
    )
add_library(clox_lib ${SOURCES})
target_include_directories(clox_lib PUBLIC src thirdparty)
target_compile_features(clox_lib PRIVATE cxx_std_20)
# target_compile_options(clox_lib PUBLIC "/GS-")#NO BUFFER SECURITY CHECK

add_executable(clox src/main.cpp)
target_link_libraries(clox clox_lib)
target_compile_features(clox PRIVATE cxx_std_20)

enable_testing()
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
    add_subdirectory(tests)
endif()