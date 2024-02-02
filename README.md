# New OPL

*** WORK IN PROGRESS ***

This is the start of an attempt to build a toolchain for NewOPL, a re-ctreation oif OPL for the Psion Organiser. The plan is to build the toolchain on Linux in C and then port it over to the RP Pico running under the Psion Organiser re-creation code. 
The plan for th elanguage is to support the original OPL command set and the translated code (QCodes). Extensions will then be added that allow use of the extra features of the re-creation hardware (and future versions of the hardware), such as graphics commands and SD card support. SD card support will be added on a later hardware revision that has an SD card on it.

Tools
=====
To build these tools on Linux, use the <code>m</code> script.

newopl_objdump

This tool dumps object files, which are files containg the QCode byte stream. 

<code>newopl_objdump [objfile] </code>

