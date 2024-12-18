# Compiler features
add_test(NAME lang_escape_text COMMAND cloxc  -code "print 1+1; print \"\nthis is a \\\"escape\\\" 'test' \n\";")
set_tests_properties(lang_escape_text PROPERTIES PASS_REGULAR_EXPRESSION "this is a \"escape\" 'test'")

# > Using dynamic variables
add_test(NAME lang_var_dyn_var_undeclared COMMAND cloxc  -allow_dynamic_variables -code "a = 1;")
add_test(NAME lang_var_dyn_var_assignment COMMAND cloxc  -allow_dynamic_variables -code "a=1; b=a;")
add_test(NAME lang_var_dyn_var_arithmetic COMMAND cloxc  -allow_dynamic_variables -code "a=1; b=a;print(a+b);")
add_test(NAME lang_var_fail_dyn_var_fail_undefined COMMAND cloxc  -allow_dynamic_variables -code "var a; b=a;")
set_tests_properties(lang_var_fail_dyn_var_fail_undefined PROPERTIES PASS_REGULAR_EXPRESSION "(ASSERTION|Error).*Runtime error.*undefined variable")
add_test(NAME lang_var_fail_dyn_var_invalid_assign_target1 COMMAND cloxc  -allow_dynamic_variables -code "a=2; a*2=1;")
add_test(NAME lang_var_fail_dyn_var_invalid_assign_target2 COMMAND cloxc  -allow_dynamic_variables -code "a=2; 2*a=1;")
add_test(NAME lang_var_fail_dyn_var_invalid_assign_target3 COMMAND cloxc  -allow_dynamic_variables -code "a=2;b=3; a*b=1;")
set_tests_properties(
    lang_var_fail_dyn_var_invalid_assign_target1
    lang_var_fail_dyn_var_invalid_assign_target2
    lang_var_fail_dyn_var_invalid_assign_target3
    PROPERTIES
    PASS_REGULAR_EXPRESSION "(ASSERTION|Error).*Invalid assignment target")

# < Using dynamic variables

# > Not using dynamic variables
add_test(NAME lang_var_declaration COMMAND cloxc  -code "var a;")
add_test(NAME lang_var_assignment COMMAND cloxc  -code "var a=1; var b=2;var c=a+b; print(c);")
set_tests_properties(lang_var_assignment PROPERTIES PASS_REGULAR_EXPRESSION "3.00")
add_test(NAME lang_var_arithmetic_and_strings COMMAND cloxc -code "var A=1;var B=2;var C=A+B;print(\"Sum of \"); print(A); print(\" + \"); print(B); print(\" = \"); print(C);")
set_tests_properties(lang_var_arithmetic_and_strings PROPERTIES PASS_REGULAR_EXPRESSION "= 3")
add_test(NAME lang_var_fail_var_undefined_read COMMAND cloxc -code "a; b=a;")
add_test(NAME lang_var_fail_invalid_assign_target_simple1 COMMAND cloxc -code "1*2=1;")
add_test(NAME lang_var_fail_invalid_assign_target_simple2 COMMAND cloxc -code "var a=1; a*2=1;")
add_test(NAME lang_var_fail_invalid_assign_target_simple3 COMMAND cloxc -code "var a=1; 2*a=1;")
add_test(NAME lang_var_fail_invalid_assign_target COMMAND cloxc -code "var a=2;var b=3; a*b=1;")
add_test(NAME lang_var_fail_undeclared_var COMMAND cloxc -code "a = 1;")
add_test(NAME lang_var_fail_undefined_var_read COMMAND cloxc -code "var a; b=a;")
set_tests_properties(
    lang_var_fail_invalid_assign_target_simple1
    lang_var_fail_invalid_assign_target_simple2
    lang_var_fail_invalid_assign_target_simple3
    lang_var_fail_invalid_assign_target
    PROPERTIES PASS_REGULAR_EXPRESSION "(ASSERTION|Error).*Invalid assignment target"
)
set_tests_properties(
    lang_var_fail_var_undefined_read
    lang_var_fail_undeclared_var
    lang_var_fail_undefined_var_read
    PROPERTIES PASS_REGULAR_EXPRESSION "(ASSERTION|Error).*(read|write).*(undefined|undeclared) variable"
)

# ##
add_test(NAME lang_from_file_print COMMAND cloxc ${CMAKE_CURRENT_SOURCE_DIR}/test.clox)
add_test(NAME lang_from_file_arith_and_print COMMAND cloxc  ${CMAKE_CURRENT_SOURCE_DIR}/test2.clox)
add_test(NAME lang_from_file_stringConcat COMMAND cloxc  ${CMAKE_CURRENT_SOURCE_DIR}/test3.clox)

add_test(NAME lang_serialize COMMAND cloxc  -compile -output test.cloxbin ${CMAKE_CURRENT_SOURCE_DIR}/test.clox)
add_test(NAME lang_deserialize COMMAND cloxc  -run test.cloxbin)

add_test(NAME lang_serialize2 COMMAND cloxc  -compile -output test2.cloxbin ${CMAKE_CURRENT_SOURCE_DIR}/test2.clox)
add_test(NAME lang_deserialize2 COMMAND cloxc  -run test2.cloxbin)

add_test(NAME lang_serialize3 COMMAND cloxc  -compile -output test3.cloxbin ${CMAKE_CURRENT_SOURCE_DIR}/test3.clox)
add_test(NAME lang_deserialize3 COMMAND cloxc  -run test3.cloxbin)