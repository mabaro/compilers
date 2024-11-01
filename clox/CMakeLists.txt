cmake_minimum_required(VERSION 3.5)
set(CMAKE_USER_MAKE_RULES_OVERRIDE "${CMAKE_CURRENT_LIST_DIR}/cmake/CompilerOptions.cmake")
project(clox)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    include(CTest)
endif()

set(CMAKE_VERBOSE_MAKEFILE ON)

include(CMakePrintHelpers)
cmake_print_variables(CMAKE_CXX_FLAGS_INIT)
cmake_print_variables(CXX_FLAGS_DEBUG)
cmake_print_variables(CXX_FLAGS_RELEASE)

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
    src/utils/input.h
    src/utils/input.cpp
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
add_library(clox_lib STATIC ${SOURCES})
target_include_directories(clox_lib PUBLIC src thirdparty)
target_compile_features(clox_lib PRIVATE cxx_std_20)

set(SOURCES
    src/utils/assert.h
    src/utils/assert.cpp
    src/utils/common.h
    src/utils/common.cpp
    src/chunk.h
    src/chunk.cpp
    src/value.h
    src/value.cpp
    src/vm.h
    src/vm.cpp
)
add_library(cloxvm_lib STATIC ${SOURCES})
target_include_directories(cloxvm_lib PUBLIC src thirdparty)
target_compile_features(cloxvm_lib PRIVATE cxx_std_20)


add_subdirectory(app)

###############################

enable_testing()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
    include(cmake/ListHelpers.cmake)
    add_subdirectory(tests)
endif()
