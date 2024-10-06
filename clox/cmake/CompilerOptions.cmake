set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# # log all *_INIT variables
# get_cmake_property(_varNames VARIABLES)
# list(REMOVE_DUPLICATES _varNames)
# list(SORT _varNames)
# foreach(_varName ${_varNames})
#     if(_varName MATCHES "_INIT$")
#         message(STATUS "${_varName}=${${_varName}}")
#     endif()
# endforeach()

function(ADD_IF_NOT_PRESSENT TARGET ARG)
    if(NOT ${TARGET} MATCHES ${ARG})
        set(${TARGET} "${${TARGET}} ${ARG}" PARENT_SCOPE)
    endif()
endfunction()

if(MSVC)
    # remove default warning level from CMAKE_CXX_FLAGS_INIT
    string(REGEX REPLACE "/W[0-4]" "" CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT}")

    ADD_IF_NOT_PRESSENT(CMAKE_CXX_FLAGS_INIT "/WX")  # warnings as errors
    ADD_IF_NOT_PRESSENT(CMAKE_CXX_FLAGS_INIT "/Wall") # all warnings
    ADD_IF_NOT_PRESSENT(CMAKE_CXX_FLAGS_INIT "/GS-") # NO BUFFER SECURITY CHECK
    ADD_IF_NOT_PRESSENT(CMAKE_CXX_FLAGS_INIT "/EHsc")# disable exceptions
    ADD_IF_NOT_PRESSENT(CMAKE_CXX_FLAGS_INIT "/GR-")    # disable RTT in runtime
    # disable custom warnings
    ADD_IF_NOT_PRESSENT(CMAKE_CXX_FLAGS_INIT "/wd4061")
    ADD_IF_NOT_PRESSENT(CMAKE_CXX_FLAGS_INIT "/wd4514")
    ADD_IF_NOT_PRESSENT(CMAKE_CXX_FLAGS_INIT "/wd4505")
    # /wdNNNN # supress warning NNNN
    # /weNNNN # error instead of warning NNNN
    # /woNNNN # notify only once for warning NNNN
    # message("[DEBUG] CMAKE_CXX_FLAGS_INIT = ${CMAKE_CXX_FLAGS_INIT}")
else()
    list(APPEND CMAKE_CXX_FLAGS "-fPIC" "-Wall" "-Werror" "-pedantic")

    if(CMAKE_CXX_COMPILER_ID MATCHES GNU)
        list(APPEND CXX_FLAGS "-fno-rtti" "-fno-exceptions")
        list(APPEND CXX_FLAGS_DEBUG "-Wsuggest-final-types"
            "-Wsuggest-final-methods" "-Wsuggest-override")
        list(APPEND CXX_FLAGS_RELEASE "-O3" "-Wno-unused")
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES Clang)
        list(APPEND CXX_FLAGS "-fno-rtti" "-fno-exceptions"
            "-Qunused-arguments" "-fcolor-diagnostics")
        list(APPEND CXX_FLAGS_DEBUG "-Wdocumentation")
        list(APPEND CXX_FLAGS_RELEASE "-O3" "-Wno-unused")
    endif()
endif()