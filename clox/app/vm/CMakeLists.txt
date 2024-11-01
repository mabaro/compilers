# Virtual Machine

add_executable(cloxvm main.cpp)
target_link_libraries(cloxvm cloxvm_lib)
target_compile_features(cloxvm PRIVATE cxx_std_20)

set(cloxvm_compile_options "")
set(cloxvm_linker_options "")

if(MSVC)
    string(APPEND cloxvm_compile_options "/DVM_BUILD")
    string(APPEND cloxvm_compile_options "/W4" "$<$<CONFIG:RELEASE>: /O2>")
    # list(APPEND cloxvm_compile_options "/NODEFAULT")
else()
    string(APPEND cloxvm_compile_options "-DVM_BUILD")
    # string(APPEND cloxvm_compile_options "/W4" "$<$<CONFIG:RELEASE>: -O3>")

    # set(cloxvm_linker_options ${cloxvm_linker_options} -nodefaultlibs)
    # set(cloxvm_linker_options ${cloxvm_linker_options} -nolibc)
    # set(cloxvm_linker_options ${cloxvm_linker_options} -nostdlib)
    # set(cloxvm_linker_options ${cloxvm_linker_options} -nostartfiles)
    # set(cloxvm_linker_options ${cloxvm_linker_options} --entry=main)


    # set(cloxvm_linker_options ${cloxvm_linker_options} -static-libgcc )
    # set(cloxvm_linker_options ${cloxvm_linker_options} -lstdc++) 

    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        # add_compile_options("-stdlib=libc++")
    else()
       set(cloxvm_linker_options ${cloxvm_linker_options} -lgcc) # required for platform fixes, even when no stdlib is wanted
    endif()
endif()

target_compile_options(cloxvm PRIVATE ${cloxvm_compile_options})
target_link_options(cloxvm PRIVATE ${cloxvm_linker_options})

add_test(NAME app_cloxvm COMMAND cloxvm ${CMAKE_BINARY_DIR}/../tests/cmd/helloworld.cloxbin)
set_tests_properties(app_cloxvm PROPERTIES PASS_REGULAR_EXPRESSION "hello world")
