# Introduction
<details>
<summary>
The book "Crafting interpreters" by R. Nystrom shows a very deep and complete travel through a compiler's pipeline.
</summary>

Highlighted book entries:
- [Pratt parser entry](https://craftinginterpreters.com/compiling-expressions.html#a-pratt-parser)

[GitHub project](https://github.com/munificent/craftinginterpreters)
  
</details>

# The book
<details>
<summary>This section contains relevant points from the book.</summary>
  
## LOX grammar (BNF definition):
(wip)
```
expression -> literal | unary | binary | grouping ;
literal    -> NUMBER | STRING | "true" | "false" | "nil" ;
grouping   -> "(" expression ")" ;
unary      -> ( "-" | "!" ) expression ;
binary     -> expression operator expression ;
operator   -> "==" | "!+" | "<" | "<=" | ">" | ">="
              "+" | "-" | "*" | "/" ;
```
</details>

# The implementation
<details>
  <summary>Implementation related stuff.</summary>

  <details>
    <summary>TO-DO</summary>
    
    - add mut/const modifiers (ideally const by default?)
    - validation of variables/scope...
    - so much more...
  </details>
  <details>
    <summary>Creating a binary executable</summary>
  
    The main idea is to have a binary with the VM with bytecode incrusted that just runs as a common binary.
    Then the compiler would generate bytecode+data and inject those into a new section in the PortableExecutable pointing the VM to the new section.

    <details open>
      <summary>Some relevant links on Portable Executables and code injection </summary>
      * https://gourish-singla.medium.com/pe-code-injection-in-windows-program-exe-ce65f70bf10a
      * https://github.com/evilsocket/libpe/blob/master/peview/peview.cpp
      * https://whereisr0da.github.io/blog/posts/2020-10-21-inject-code
      * https://alexm.ro/en/blog/build-the-smallest-portable-executable-with-assembly
      * https://www.codeproject.com/Articles/24417/Portable-Executable-P-E-Code-Injection-Injecting-a
      * https://code.google.com/archive/p/portable-executable-library  
      * https://www.archcloudlabs.com/projects/binary_loaders_1  
    </details>
  </details>
</details>

</details>

WIP
<details>

https://craftinginterpreters.com/calls-and-functions.html

</details>
