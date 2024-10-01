https://craftinginterpreters.com/compiling-expressions.html
https://craftinginterpreters.com/chunks-of-bytecode.html
https://craftinginterpreters.com/compiling-expressions.html#parsing-prefix-expressions
https://craftinginterpreters.com/global-variables.html#assignment
https://craftinginterpreters.com/types-of-values.html

LOX grammar (BNF definition):
```
expression -> literal | unary | binary | grouping ;
literal    -> NUMBER | STRING | "true" | "false" | "nil" ;
grouping   -> "(" expression ")" ;
unary      -> ( "-" | "!" ) expression ;
binary     -> expression operator expression ;
operator   -> "==" | "!+" | "<" | "<=" | ">" | ">="
              "+" | "-" | "*" | "/" ;
```

pending:
- add mut/const modifiers (ideally const by default?)
- validation of variables/scope...
- so much more...

* creating portable executable:
https://whereisr0da.github.io/blog/posts/2020-10-21-inject-code/

https://alexm.ro/en/blog/build-the-smallest-portable-executable-with-assembly

https://www.codeproject.com/Articles/24417/Portable-Executable-P-E-Code-Injection-Injecting-a

https://www.archcloudlabs.com/projects/binary_loaders_1/
