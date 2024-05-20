https://craftinginterpreters.com/compiling-expressions.html
https://craftinginterpreters.com/chunks-of-bytecode.html
https://craftinginterpreters.com/compiling-expressions.html#parsing-prefix-expressions
https://craftinginterpreters.com/types-of-values.html

LOX grammar (BNF definition):

expression -> literal | unary | binary | grouping ;
literal    -> NUMBER | STRING | "true" | "false" | "nil" ;
grouping   -> "(" expression ")" ;
unary      -> ( "-" | "!" ) expression ;
binary     -> expression operator expression ;
operator   -> "==" | "!+" | "<" | "<=" | ">" | ">="
              "+" | "-" | "*" | "/" ;
