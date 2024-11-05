# PE_BUILD
add_executable(clox_pe main.cpp)
target_link_libraries(clox_pe clox_lib)
target_compile_features(clox_pe PRIVATE cxx_std_20)
target_compile_definitions(clox_pe PRIVATE PE_BUILD)

set(pe_compile_options "")
set(pe_linker_options "")

if(MSVC)
    string(APPEND pe_compile_options "/W4" "$<$<CONFIG:RELEASE>: /O2>")
    # list(APPEND pe_compile_options "/NODEFAULT")
else()
    # string(APPEND pe_compile_options "/W4" "$<$<CONFIG:RELEASE>: -O3>")

    # set(pe_linker_options ${pe_linker_options} -nodefaultlibs)
    # set(pe_linker_options ${pe_linker_options} -nolibc)
    # set(pe_linker_options ${pe_linker_options} -nostdlib)
    # set(pe_linker_options ${pe_linker_options} -nostartfiles)
    # set(pe_linker_options ${pe_linker_options} --entry=main)


    # set(pe_linker_options ${pe_linker_options} -static-libgcc )
    # set(pe_linker_options ${pe_linker_options} -lstdc++) 

    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        # add_compile_options("-stdlib=libc++")
    else()
       set(pe_linker_options ${pe_linker_options} -lgcc) # required for platform fixes, even when no stdlib is wanted
    endif()
endif()

target_compile_options(clox_pe PRIVATE ${pe_compile_options})
target_link_options(clox_pe PRIVATE ${pe_linker_options})

add_test(NAME app_clox_pe COMMAND clox_pe)
set_tests_properties(app_clox_pe PROPERTIES PASS_REGULAR_EXPRESSION "hello world")
