# New OPL

*** WORK IN PROGRESS ***

This is the start of an attempt to build a toolchain for NewOPL, a re-creation of OPL for the Psion Organiser. The plan is to build the toolchain on Linux in C and then port it over to the RP Pico running under the Psion Organiser re-creation code on that hardware. 
The plan for the language is to support the original OPL command set and the translated code (QCodes). Extensions will then be added that allow use of the extra features of the re-creation hardware (and future versions of the hardware), such as graphics commands and SD card support. SD card support will be added on a later hardware revision that has an SD card on it.

Tools
=====
To build these tools on Linux, use the <code>m</code> script.

newopl_objdump
--------------

This tool dumps object files, which are files containg the QCode byte stream. 

<code>newopl_objdump [objfile] </code>

It can read binary QCode files and also OB3 files.

newopl_exec
-----------

Executes an OPL procedure

For example:

./newopl_exec ../examples/PRNT1.OB3

Runs the PRNT1.OB3 exmple file. This contains:

<code>
prnt1:

LOCAL A%, A1%
GLOBAL G%,G1%,GG%

A%=1
A1%=11
G%=6
GG%=66
G1%=61
PRINT A%
PRINT A1%
PRINT G%
PRINT G1%
PRINT GG%

</code>

and results in the output:

<code>
1
11
6
61
66
</code>

** Not all qcodes are implemented, only a minimum set at the moment.


newopl_tran
-----------

Translates an OPL text file into qcode. 

For example:

./newopl_tran ../examples/expr1.opl

will turn the expr1.opl example into output tokens in the 'intcode.txt' file. The intention is that those output tokens will then be turned into the final qcode.



Runtime
=======

The runtime uses the same stack structure as original OPL. The stack is limited to 64K, but could use all of that 64K if needed.
The original binaries should be able to be run.
The model is the same stack based system as organiser OPL, with externals, globals and locals handled in the same way as the original.

Translation
===========

The translation model at the moment is to use a recursive descent parser to parse the command lines (colon separated lines are split into lines) and generate the tokens that will be turned into qcode using the parser.

Commands are generally treated as expressions, so:

<code>X% = X% + 1</code>

is regarded as an expression with operators of '=' and '+' and atoms of X% (a variable reference) and 1 (an integer.

Obviously functions like SIN and COS work well with this scheme, and a lot of the commands also work well as they accept a variety of arguments that can be expressed as expressions. Some commands like IF and PRINT have theor own unique requirements and are implemented using their own parsers. The expressions model does work well when attempting to create code that will run on the original Organiser QCode runtime.

The algorithm used to process expressions is the 'shunting yard algorithm' (see here: https://en.wikipedia.org/wiki/Shunting_yard_algorithm). this parses an expression and outputs an RPN stack based version of the infix expression. This seems to match quite well with the qcode that the organiser translater outputs. 

The expressions in OPL have to be typed as different qcodes are used for different types of code. For example, the PRINT qcode has versions for integers, floats and strings. The correct code has to be used for the argument the code is presented with. This is done at translate time by the original OPL.

The text is parsed and then passed as tokens to an expression processor that handles these in isolation. This uses the shunting yard algorithm to order the expressions in postfix form from the OPL infix form. 

As the translator is going to run on a small Pico sized processor we cannot use a file system for the core processing (the PC version has files for debug but they aren't used in the core translation) and we also can't have large memory based syntax trees as the memory is not available. This is very like the original Organiser which did all this in the organiser ROM. Translation is done on a line by line basis, delimited by colons. REMs are treated as any other function, but no QCodes are generated.

Each expression is turned into an intermediate stream of codes which also have syntax and type information. These are then processed and automatic type conversion codes inserted where needed in the arithmetical codes. This is only needed for the integer and float types which need to interoperate within expressions. A syntax tree is build and used to check the types of the expressions and also to insert the auto conversion codes.

The GLOBAL and LOCAL statements are not treated as expressions and are parsed separately. They are used to build a variable table which holds the variable names and types. Local variables are added to this table when references to them are found. The offset of the variables is calculated and used when needed in the QCode. Variable names are used for globals and externals.

The control structures (IF/ENDIF, DO/UNTIL and WHILE/ENDWH) all have their own parsers. These parsers are called by the scan_line() parser function, and then the control parsers themselves recursively call the scan_line() parser to parse the blocks of code that the control. This allows the nesting structure to be easily reflected in the output stream using tags for each level of IF, DO and WHILE. To see more detail, translate, say, do.opl and look in the intcode.txt file. The level tags for each block are shown there.


Negative integers are parsed in the recursive descent parser and so the negation of positive integers to get negative integers isn't used. The recursive descent parser does have unary operator support, however, so the unary '-' can be used in expressions.

The translation is performed in two passes. The first pass is used to build tables and fixup information, some of which can't be determined on one pass, and the second pass uses this information to build the QCode. QCode is not built on the first pass.


Things I didn't know about OPL when I started
=============================================

* You can have a variable called p% and also an array called p%() and they are different variables
* REM statements can occur before and between LOCAL and GLOBAL
* You can have a character constant for the space character:
     <code>
     C% = %
     </code>
  The statement:
  <code>
     C%=%%+%++% +%A
  </code>
  is valid.
  


