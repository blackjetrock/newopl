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

and results in the output:

1
11
6
61
66

** Not all qcodes are implemented, only a minimum set at the moment.


newopl_tran
-----------

Translates an OPL text file into qcode. 

For example:

./newopl_tran ../examples/expr1.opl

will turn the expr1.opl example into output tokens in the 'output.txt' file. The intention is that those output tokens will then be turned into the final qcode.



Runtime
=======

The runtime uses the same stack structure as original OPL. The stack is limited to 64K, but could use all of that 64K if needed.
The original binaries can be run.
The model is the same stack based system as organiser OPL, with externals, globals and locals handled in the same way as the original.

Translation
===========

The translation model at the moment (hopefully it will work), is to treat every OPL statement as an expression. So, for instance, 

PRINT X%

is regarded as a function called PRINT with an argument of X%.

Obviously functions like SIN and COS work well with this scheme, and as commands like PRINT behave like functions with no return values, they work well too.
The algorithm used to process expressions is the 'shunting yard algorithm' (see here: https://en.wikipedia.org/wiki/Shunting_yard_algorithm). this parses an expression and outputs an RPN stack based version of the infix expression. This seems to match quite well with the qcode that the organiser translater outputs. 
The expressions in OPL have to be typed as different qcodes are used for different types of code. For example, the PRINT qcode has versions for integers, floats and strings. The correct code has to be used for the argument the code is presented with. This is done at translate time by the original OPL.

Assignment statements are also treated as expressions, with the '=' symbol represented by an assignment operator. This seems to match with the original OPL as well, but is a bit counter-intuitive.

The GLOBAL and LOCAL statemants are not treated as expressions and are parsed separately.

The conditional and branch statements are treated as expressions as they use expressions as their control values. There is additional processing with these statements, however, as the destinations of branches need to be stored for each statement, and this also needs to handle nested branches correctly.
