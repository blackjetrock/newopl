# New OPL

*** WORK IN PROGRESS ***

This is the start of an attempt to build a toolchain for NewOPL, a re-creation of OPL for the Psion Organiser. The plan is to build the toolchain on Linux in C and then port it over to the RP Pico running under the Psion Organiser re-creation code on that hardware. 
The plan for the language is to support the original OPL command set and the translated code (QCodes). Extensions will then be added that allow use of the extra features of the re-creation hardware (and future versions of the hardware), such as graphics commands and SD card support. SD card support will be added on a later hardware revision that has an SD card on it.

Current Status
==============

The translator can translate OPL and generate QCode and a .ob3 file. The QCode is byte compatible with the Psion Organiser. It generates the same QCode as an Organiser LZ apart from one situation where it generate sightly different QCode, but the QCode still works properly.

The runtime can run almost all of the QCodes (check the big table in qcode.c to see what is supported). Output is on the terminal at the moment, or in a curses TUI.
If you build with mtui then the runtime uses curses and sends output to a TUI. The TUI also has a debugger for QCode and allows the stack to be examined as well as the file record fields an variables.

Tools
=====
To build these tools on Linux, use:

<code>m</code> script for a text based runtime.
<br><code>mtui</code> script for a curses based TUI runtime with a debugger.

newopl_objdump
--------------

This tool dumps object files, which are files containg the QCode byte stream. 

<code>newopl_objdump [objfile] </code>

It can read binary QCode files and also OB3 files.

newopl_exec
-----------

Executes an OPL procedure. If the TUI version is built then the output is in a windowed curses form and includes a QCode debugger.

For example:

<code>./newopl_exec ../examples/PRNT1.OB3</code>

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

** Not all qcodes are implemented, check qcode.c for the full list


newopl_tran
-----------

Translates an OPL text file into qcode. 

For example:

./newopl_tran ../examples/expr1.opl

will turn the expr1.opl example into output tokens in the 'intcode.txt' file. Those output tokens are then turned into the final qcode in an ob3 file.


Runtime
=======

The runtime uses the same stack structure as original OPL. The stack is limited to 64K, but could use all of that 64K if needed.
The original binaries should be able to be run, assuing they use only supported QCodes.

The model is the same stack based system as organiser OPL, with externals, globals and locals handled in the same way as the original. OPL translated o an original organiser should work, subject to all the QCodes being implemented.

Debugger
--------

When run in TUI mode, the runtime opens an output window and a printer window for PRINT and LPRINT output respectively, and also several debugger windows. The code is single stepped in this mode. The stack and other memory can be examined, as well as file records and variables.

![image](https://github.com/user-attachments/assets/8bde2bf1-19b8-4b7d-b2c7-665068bb8aa8)

The memory window shows memory. Typing hex digits sets the address of the dat shown in this window.
The QCode window shows the QCode that will be executed when ENTER is pressed. There's a decode of the QCode and the output in a similar form to the output of newopl_onbjdump.
The Output window shows the PRINT statement outputs.
The Machine window has the system variables used by the QCode engine.
The Variables window shows the variables used by the current PROC. 
The Printer window shows output fro the LPRINT statements.

Variables Window
----------------

This window shows variables:

![image](https://github.com/user-attachments/assets/d19399c3-0a4c-415c-9d58-a7e2f5bd89e5)

and also the calculator memories:

![image](https://github.com/user-attachments/assets/50726dcb-2c16-48b9-8ad7-86323b250658)

It shows the file fields and information about the files:

![image](https://github.com/user-attachments/assets/37fdef34-2b2a-4211-9606-0061a970c6ac)

Variable values cannot be altered in these windows.

Translation
===========

The translation model at the moment is to use a recursive descent parser to parse the command lines (colon separated lines are split into lines) and generate the tokens that will be turned into qcode using the parser.

Commands are generally treated as expressions, so:

<code>X% = X% + 1</code>

is regarded as an expression with operators of '=' and '+' and atoms of X% (a variable reference) and 1 (an integer.

Obviously functions like SIN and COS work well with this scheme, and a lot of the commands also work well as they accept a variety of arguments that can be expressed as expressions. Some commands like IF and PRINT have theor own unique requirements and are implemented using their own parsers. The expressions model does work well when attempting to create code that will run on the original Organiser QCode runtime.

The algorithm used to process expressions is the 'shunting yard algorithm' (see here: https://en.wikipedia.org/wiki/Shunting_yard_algorithm). this parses an expression and outputs an RPN stack based version of the infix expression. This seems to match quite well with the qcode that the organiser translater outputs. 

The expressions in OPL have to be typed as different qcodes are used for different types of code. For example, the PRINT qcode has versions for integers, floats and strings. The correct code has to be used for the argument the code is presented with. This is done at translate time by the original OPL.

The text is parsed on a line by line basis and then passed as tokens to an expression processor that handles these in isolation. This uses the shunting yard algorithm to order the expressions in postfix form from the OPL infix form. 

As the translator is going to run on a small Pico sized processor we cannot use a file system for the core processing (the PC version has files for debug but they aren't used in the core translation) and we also can't have large memory based syntax trees as the memory is not available. This is very like the original Organiser which did all this in the organiser ROM. Translation is done on a line by line basis, delimited by colons. REMs are treated as any other function, but no QCodes are generated.

Each expression is turned into an intermediate stream of codes which also have syntax and type information. These are then processed and automatic type conversion codes inserted where needed in the arithmetical codes. This is only needed for the integer and float types which need to interoperate within expressions. A syntax tree is build and used to check the types of the expressions and also to insert the auto conversion codes.

The GLOBAL and LOCAL statements are not treated as expressions and are parsed separately. They are used to build a variable table which holds the variable names and types. Local variables are added to this table when references to them are found. The offset of the variables is calculated and used when needed in the QCode. Variable names are used for globals and externals.

The control structures (IF/ENDIF, DO/UNTIL and WHILE/ENDWH) all have their own parsers. These parsers are called by the scan_line() parser function, and then the control parsers themselves recursively call the scan_line() parser to parse the blocks of code that the control. This allows the nesting structure to be easily reflected in the output stream using tags for each level of IF, DO and WHILE. To see more detail, translate, say, do.opl and look in the intcode.txt file. The level tags for each block are shown there.


Negative integers are parsed in the recursive descent parser and so the negation of positive integers to get negative integers isn't used. The recursive descent parser does have unary operator support, however, so the unary '-' can be used in expressions.

The translation is performed in two passes. The first pass is used to build tables and fixup information, some of which can't be determined on one pass, and the second pass uses this information to build the QCode. QCode is not built on the first pass.

Tests
=====

There are two sets of tests, one for translation and one for execution.

To run the translation tests, run:

<br><code>./tr_test.tcl</code>

from the <code>pc</code> directory.

To run the execution tests, run:

<br><code>./ex_test.tcl</code>

from the <code>pc</code> directory.

The scripts run all the tests, displaying results as they go, counting successes and failures which are displayed at the end of the run.
If you want to run the execution test extst_XXXX.opl then use:

<br><code>./ef XXXX</code>

If you want to execute a test in the debugger use:

<br><code>./eft XXXX</code>


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
* A statement can be an expression. This is done frequently e.g:
  <code>
    GET
  </code>
  A less useful form is:
  <code>
    SIN(90)
  </code>

  What I have found is that you cannot doi:

  <code>
    SIN(90)+COS(90)
  </code>code>
  which gives an error when translated on an Organiser.

* Even though all the documentation says the DAYS keyword returns an integer, it doesn't, it returns a float. This is because the number of days from 1/1/1900 won't fit in an integer after 18th Sep 1989. The RTF_DAYS QCode returns a float. There are examples of OPL code that treat the result of DAYS as a float and assign it to a float variable.
    


