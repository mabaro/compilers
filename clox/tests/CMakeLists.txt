set(ENV{CTEST_OUTPUT_ON_FAILURE}, 1)
set(ENV{CTEST_OUTPUT_ON_SUCCESS}, 1)

add_subdirectory(cmd)
add_subdirectory(compiler)
add_subdirectory(lang)
