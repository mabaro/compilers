add_executable(cloxc main.cpp)
target_link_libraries(cloxc clox_lib)
target_compile_features(cloxc PRIVATE cxx_std_20)

set(cloxc_compile_options "")
set(cloxc_linker_options "")

if(MSVC)
    string(APPEND cloxc_compile_options "/DTOOL_BUILD")
    string(APPEND cloxc_compile_options "/W4" "$<$<CONFIG:RELEASE>: /O2>")
    # list(APPEND cloxc_compile_options "/NODEFAULT")
else()
    string(APPEND cloxc_compile_options "-DTOOL_BUILD")
    # string(APPEND cloxc_compile_options "/W4" "$<$<CONFIG:RELEASE>: -O3>")

    # set(cloxc_linker_options ${cloxc_linker_options} -nodefaultlibs)
    # set(cloxc_linker_options ${cloxc_linker_options} -nolibc)
    # set(cloxc_linker_options ${cloxc_linker_options} -nostdlib)
    # set(cloxc_linker_options ${cloxc_linker_options} -nostartfiles)
    # set(cloxc_linker_options ${cloxc_linker_options} --entry=main)


    # set(cloxc_linker_options ${cloxc_linker_options} -static-libgcc )
    # set(cloxc_linker_options ${cloxc_linker_options} -lstdc++) 

    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        # add_compile_options("-stdlib=libc++")
    else()
       set(cloxc_linker_options ${cloxc_linker_options} -lgcc) # required for platform fixes, even when no stdlib is wanted
    endif()
endif()

target_compile_options(cloxc PRIVATE ${cloxc_compile_options})
target_link_options(cloxc PRIVATE ${cloxc_linker_options})

add_test(NAME app_cloxc COMMAND cloxc -code "var a=1; var b=2;var c=a+b; print(c);")
set_tests_properties(app_cloxc PROPERTIES PASS_REGULAR_EXPRESSION "3")
