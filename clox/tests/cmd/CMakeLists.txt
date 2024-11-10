# Command line tests
add_test(NAME cmd_help COMMAND cloxc -help)
set_tests_properties(cmd_help PROPERTIES PASS_REGULAR_EXPRESSION "Usage:")
add_test(NAME cmd_fail_no_args COMMAND cloxc)
set_tests_properties(cmd_fail_no_args PROPERTIES WILL_FAIL TRUE)

add_test(NAME cmd_interpret_from_code COMMAND cloxc -code "print \"hello world\";")
add_test(NAME cmd_interpret_from_file COMMAND cloxc ${CMAKE_CURRENT_SOURCE_DIR}/test.clox)
add_test(NAME cmd_compile_from_code COMMAND cloxc -compile -code "print \"hello world\";")
set_tests_properties(cmd_compile_from_code
    PROPERTIES PASS_REGULAR_EXPRESSION ".*_CODE42_.*")
add_test(NAME cmd_compile_from_file COMMAND cloxc -compile ${CMAKE_CURRENT_SOURCE_DIR}/test.clox -output ${CMAKE_CURRENT_SOURCE_DIR}/helloworld.cloxbin)
add_test(NAME cmd_run_from_file COMMAND cloxc -run ${CMAKE_CURRENT_SOURCE_DIR}/helloworld.cloxbin)
set_tests_properties(
    cmd_interpret_from_code
    cmd_interpret_from_file
    cmd_compile_from_code
    cmd_run_from_file
    PROPERTIES PASS_REGULAR_EXPRESSION "hello world")
