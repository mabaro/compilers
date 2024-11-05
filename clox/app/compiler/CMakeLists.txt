add_executable(cloxc main.cpp)
target_link_libraries(cloxc clox_lib)
target_compile_features(cloxc PRIVATE cxx_std_20)
target_compile_definitions(cloxc PRIVATE TOOL_BUILD)

set(cloxc_compile_options "")
set(cloxc_linker_options "")

if(MSVC)
    # list(APPEND cloxc_compile_options /W4)
    # list(APPEND cloxc_compile_options $<$<CONFIG:RELEASE>: /O2>)
else()
    # list(APPEND cloxc_compile_options /W4)
    # list(APPEND cloxc_compile_options $<$<CONFIG:RELEASE>: /O2>)
endif()

target_compile_options(cloxc PRIVATE ${cloxc_compile_options})
target_link_options(cloxc PRIVATE ${cloxc_linker_options})

add_test(NAME app_cloxc COMMAND cloxc -code "var a=1; var b=2;var c=a+b; print(c);")
set_tests_properties(app_cloxc PROPERTIES PASS_REGULAR_EXPRESSION "3")
