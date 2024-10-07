set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# # log all *_INIT variables
# get_cmake_property(_varNames VARIABLES)
# list(REMOVE_DUPLICATES _varNames)
# list(SORT _varNames)
# foreach(_varName ${_varNames})
# if(_varName MATCHES "_INIT$")
# message(STATUS "${_varName}=${${_varName}}")
# endif()
# endforeach()
function(ADD_IF_NOT_PRESSENT TARGET ARG)
    if(NOT ${TARGET} MATCHES ${ARG})
        set(${TARGET} "${${TARGET}} ${ARG}" PARENT_SCOPE)
    endif()
endfunction()

if(MSVC)
    # remove default warning level from CMAKE_CXX_FLAGS_INIT
    string(REGEX REPLACE "/W[^ ]*" "" CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT}")
    ADD_IF_NOT_PRESSENT(CMAKE_CXX_FLAGS_INIT "/WX") # warnings as errors
    ADD_IF_NOT_PRESSENT(CMAKE_CXX_FLAGS_INIT "/Wall") # all warnings

    string(REGEX REPLACE "/GS[^ ]*" "" CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT}")
    ADD_IF_NOT_PRESSENT(CMAKE_CXX_FLAGS_INIT "/GS-") # NO BUFFER SECURITY CHECK
    string(REGEX REPLACE "/EH[^ ]*" "" CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT}")
    ADD_IF_NOT_PRESSENT(CMAKE_CXX_FLAGS_INIT "/EHsc") # disable exceptions
    string(REGEX REPLACE "/GR[^ ]*" "" CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT}")
    ADD_IF_NOT_PRESSENT(CMAKE_CXX_FLAGS_INIT "/GR-") # disable RTT in runtime

    # disable custom warnings
    ADD_IF_NOT_PRESSENT(CMAKE_CXX_FLAGS_INIT "/wd4061")
    ADD_IF_NOT_PRESSENT(CMAKE_CXX_FLAGS_INIT "/wd4365") # conversion from int to unsigned int64
    ADD_IF_NOT_PRESSENT(CMAKE_CXX_FLAGS_INIT "/wd4625") # copy constructor was implicitly defined as deleted
    ADD_IF_NOT_PRESSENT(CMAKE_CXX_FLAGS_INIT "/wd4626") # assignment operator was implicitly defined as deleted
    ADD_IF_NOT_PRESSENT(CMAKE_CXX_FLAGS_INIT "/wd4820") # padding after data member
    ADD_IF_NOT_PRESSENT(CMAKE_CXX_FLAGS_INIT "/wd4514") # removed unused inlined function
    ADD_IF_NOT_PRESSENT(CMAKE_CXX_FLAGS_INIT "/wd4505") # unreferenced function with internal linkage has been removed
    ADD_IF_NOT_PRESSENT(CMAKE_CXX_FLAGS_INIT "/wd4710") # function was selected for inline
    ADD_IF_NOT_PRESSENT(CMAKE_CXX_FLAGS_INIT "/wd4711") # function XXX selected for automatic inline expansion
    ADD_IF_NOT_PRESSENT(CMAKE_CXX_FLAGS_INIT "/wd4774") # print argument for format is not a literal
    ADD_IF_NOT_PRESSENT(CMAKE_CXX_FLAGS_INIT "/wd5045") # Compiler will insert Spectre mitigation for memory load if
    ADD_IF_NOT_PRESSENT(CMAKE_CXX_FLAGS_INIT "/wd5246") # initialization of a subobject should be wrapped in braces

    # remove double spaces
    string(REGEX REPLACE "[ ]+" " " CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT}")

# /wdNNNN # supress warning NNNN
# /weNNNN # error instead of warning NNNN
# /woNNNN # notify only once for warning NNNN
# message("[DEBUG] CMAKE_CXX_FLAGS_INIT = ${CMAKE_CXX_FLAGS_INIT}")
else()
    list(APPEND CMAKE_CXX_FLAGS_INIT "-fPIC" "-Wall" "-pedantic")

    # "-Werror" )
    if(CMAKE_CXX_COMPILER_ID MATCHES GNU)
        list(APPEND CMAKE_CXX_FLAGS_INIT "-fno-rtti" "-fno-exceptions")
        list(APPEND CMAKE_CXX_FLAGS_DEBUG_INIT "-Wsuggest-final-types"
            "-Wsuggest-final-methods" "-Wsuggest-override")
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES Clang)
        list(APPEND CMAKE_CXX_FLAGS_INIT "-fno-rtti" "-fno-exceptions"
            "-Qunused-arguments" "-fcolor-diagnostics")
        list(APPEND CMAKE_CXX_FLAGS_DEBUG_INIT "-Wdocumentation")
    endif()

    string(REGEX REPLACE "-O[^ ]*" "" CMAKE_CXX_FLAGS_RELEASE_INIT "${CMAKE_CXX_FLAGS_RELEASE_INIT}")
    list(APPEND CMAKE_CXX_FLAGS_RELEASE_INIT "-O3")

    # disable warnings
    ADD_IF_NOT_PRESSENT(CMAKE_CXX_FLAGS_INIT "-Wno-gnu-zero-variadic-macro-arguments")
    ADD_IF_NOT_PRESSENT(CMAKE_CXX_FLAGS_INIT "-Wno-format-security")
    ADD_IF_NOT_PRESSENT(CMAKE_CXX_FLAGS_RELEASE_INIT "-Wno-error=unused-function")

    ADD_IF_NOT_PRESSENT(CMAKE_CXX_FLAGS_RELEASE_INIT "-D NDEBUG")
    ADD_IF_NOT_PRESSENT(CMAKE_CXX_FLAGS_DEBUG_INIT "-D DEBUG")

    string(REPLACE ";" " " CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT}")
    string(REPLACE ";" " " CMAKE_CXX_FLAGS_DEBUG_INIT "${CMAKE_CXX_FLAGS_DEBUG_INIT}")
    string(REPLACE ";" " " CMAKE_CXX_FLAGS_RELEASE_INIT "${CMAKE_CXX_FLAGS_RELEASE_INIT}")
endif()
