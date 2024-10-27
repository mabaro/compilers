# list uses ';' and ' ' as separators, thus we need to encode/decode those 
function(encode_for_list INOUT)
    string(REPLACE ";" "#" ${INOUT} ${${INOUT}})
    string(REPLACE " " "##" ${INOUT} ${${INOUT}})
    set(${INOUT} ${${INOUT}} PARENT_SCOPE)
endfunction()

function(decode_for_list INOUT)
    string(REPLACE "##" " " ${INOUT} ${${INOUT}})
    string(REPLACE "#" ";" ${INOUT} ${${INOUT}})
    set(${INOUT} ${${INOUT}} PARENT_SCOPE)
endfunction()

function(add_to_list_encoded LIST ARG)
    encode_for_list(ARG)
    list(APPEND ${LIST} ${ARG})
    set(${LIST} ${${LIST}} PARENT_SCOPE)
endfunction()