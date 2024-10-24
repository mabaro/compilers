## print
add_test(NAME lang_print COMMAND clox -code "print \"hello world\";")
set_tests_properties(lang_print PROPERTIES PASS_REGULAR_EXPRESSION "hello world")

## string
add_test(NAME lang_string_concat COMMAND clox -code "var a=\"hello\"; var b=\"world\"; print a + \" \" + b;")
set_tests_properties(lang_string_concat PROPERTIES PASS_REGULAR_EXPRESSION "hello world")

# # arith
add_test(NAME lang_equal1 COMMAND clox -code "print true == true;")
add_test(NAME lang_equal2 COMMAND clox -code "print 0 == 0;")
add_test(NAME lang_equal3 COMMAND clox -code "print 1 == 1;")
add_test(NAME lang_equal4 COMMAND clox -code "print \"abcd\" == \"abcd\";")
set_tests_properties(
    lang_equal1
    lang_equal2
    lang_equal3
    lang_equal4
PROPERTIES PASS_REGULAR_EXPRESSION "true")

add_test(NAME lang_not_equal1 COMMAND clox -code "print true != true;")
add_test(NAME lang_not_equal2 COMMAND clox -code "print 0 != 0;")
add_test(NAME lang_not_equal3 COMMAND clox -code "print 1 != 1;")
add_test(NAME lang_not_equal4 COMMAND clox -code "print \"a\" != \"a\";")
set_tests_properties(
    lang_not_equal1
    lang_not_equal2
    lang_not_equal3
    lang_not_equal4
PROPERTIES PASS_REGULAR_EXPRESSION "false")

add_test(NAME lang_arith_sum COMMAND clox -code "var a=1; var b=2; print a + b;")
set_tests_properties(lang_arith_sum PROPERTIES PASS_REGULAR_EXPRESSION "3")
add_test(NAME lang_arith_sub COMMAND clox -code "var a=1; var b=2; print a - b;")
set_tests_properties(lang_arith_sub PROPERTIES PASS_REGULAR_EXPRESSION "-1")
add_test(NAME lang_arith_mul COMMAND clox -code "var a=3; var b=5; print a * b;")
set_tests_properties(lang_arith_mul PROPERTIES PASS_REGULAR_EXPRESSION "15")
add_test(NAME lang_arith_div COMMAND clox -code "var a=3; var b=2; print a / b;")
set_tests_properties(lang_arith_div PROPERTIES PASS_REGULAR_EXPRESSION "1.5")

# # flow control
add_test(NAME lang_flow_if1 COMMAND clox -code "if (true) print(\"true\"); else print(\"false\");")
set_tests_properties(lang_flow_if1 PROPERTIES PASS_REGULAR_EXPRESSION "true")
add_test(NAME lang_flow_if2 COMMAND clox -code "if (false) print(\"true\"); else print(\"false\");")
set_tests_properties(lang_flow_if2 PROPERTIES PASS_REGULAR_EXPRESSION "false")
add_test(NAME lang_flow_if3 COMMAND clox -code "var a=2; var b=1; if (a > b && b > 0) print(\"bigger\"); else print(\"smaller\");")
set_tests_properties(lang_flow_if3 PROPERTIES PASS_REGULAR_EXPRESSION "bigger")

add_test(NAME lang_flow_while COMMAND clox -code "var a=0; while(a<5) { a=a+1; } print a;")
set_tests_properties(lang_flow_while PROPERTIES PASS_REGULAR_EXPRESSION "5")

add_test(NAME lang_flow_do_while COMMAND clox -code "var a=0; do { a=a+1; }while(a<5); print a==5;")
set_tests_properties(lang_flow_do_while PROPERTIES PASS_REGULAR_EXPRESSION "true")

## Compilation fail (should catch undefined/undeclared variables at least)
add_test(NAME lang_error COMMAND clox -code "asdf print 1;")
set_tests_properties(lang_error PROPERTIES PASS_REGULAR_EXPRESSION "Compilation failed.")


set(PREFIX lang_file_)
set(TESTS_LIST
    arith
    scope
    do_while
    while
)
foreach (test ${TESTS_LIST})
    set(test_name ${PREFIX}${test})
    add_test(NAME ${test_name} COMMAND clox ${test}.clox)
    set_tests_properties(${test_name} PROPERTIES
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        LABELS lang
        PASS_REGULAR_EXPRESSION "true"
    )
endforeach ()
