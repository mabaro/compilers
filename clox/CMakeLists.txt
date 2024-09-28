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
    src/assert.h
    src/assert.cpp
    src/chunk.h
    src/common.h
    src/common.cpp
    src/compiler.h
    src/debug.h
    src/debug.cpp
    src/scanner.h
    src/value.h
    src/value.cpp
    src/vm.h)
add_library(clox_lib ${SOURCES})
target_include_directories(clox_lib PUBLIC src thirdparty)
target_compile_features(clox_lib PRIVATE cxx_std_20)

add_executable(clox src/main.cpp)
target_link_libraries(clox clox_lib)
target_compile_features(clox PRIVATE cxx_std_20)

add_test(NAME no_args COMMAND clox)
add_test(NAME from_arg COMMAND clox -code "var a=1; var b=2;var c=a+b;print(c);")
add_test(NAME from_arg2 COMMAND clox -code "var A=0;var B=1;var C=A+B;print(\"El resultado de sumar \"); print(A); print(\" + \"); print(B); print(\" = \"); print(C);" )
add_test(NAME from_file COMMAND clox ${PROJECT_SOURCE_DIR}/tests/test.clox)
add_test(NAME from_file2 COMMAND clox ${PROJECT_SOURCE_DIR}/tests/test2.clox)

enable_testing()
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
    add_subdirectory(tests)
endif()