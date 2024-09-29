set(ENV{CTEST_OUTPUT_ON_FAILURE}, 1)
set(ENV{CTEST_OUTPUT_ON_SUCCESS}, 1)

add_test(NAME no_args_fail COMMAND clox)
add_test(NAME from_args_undeclared_var COMMAND clox -code "a;")
add_test(NAME from_arg_undefined_var_read COMMAND clox -code "var a; b=a;")
add_test(NAME from_arg_undeclared_var_write COMMAND clox -code "a=1; b=a;")
set_tests_properties(
    no_args_fail
    from_args_undeclared_var
    from_arg_undefined_var_read
    from_arg_undeclared_var_write
    PROPERTIES WILL_FAIL TRUE)

add_test(NAME help COMMAND clox -help)
add_test(NAME from_arg COMMAND clox -code "var a=1; var b=2;var c=a+b;print(c);")
add_test(NAME from_arg2 COMMAND clox -code "var A=0;var B=1;var C=A+B;print(\"Sum of \"); print(A); print(\" + \"); print(B); print(\" = \"); print(C);")
add_test(NAME from_arg_dynamic_var_write COMMAND clox -allow_dynamic_variables -code "a=1; b=a;")
add_test(NAME from_arg_dynamic_var_undefined_read COMMAND clox -allow_dynamic_variables -code "a; b=a;")
set_tests_properties(
    from_arg_dynamic_var_undefined_read
    PROPERTIES WILL_FAIL TRUE)

add_test(NAME from_file COMMAND clox ${PROJECT_SOURCE_DIR}/tests/test.clox)
add_test(NAME from_file2 COMMAND clox ${PROJECT_SOURCE_DIR}/tests/test2.clox)
