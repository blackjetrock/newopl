Those files marked with * are not normally part of the Developer kit,
and not necessarily written by Psion.

ORG2    EXE    3.20  Developer.
HELP    TXT          Help text for ordinary developer kit.
MANUAL  TXT          Manual for ordinary developer kit.

MAKE    EXE    3.20  PC program to write OPK image to datapaks.
MAKE    BIN    3.3   Psion side of the above program.

AMAKE   BAT          PC batch program to append OPK image to datapaks. Uses MAKE.EXE.
AMAKE   BIN    3.3   Psion side of the above program.

UNMAKE  EXE    3.20  PC program to read OPK image from paks, and splits it up.
UNMAKE  BIN    3.3   Psion side of the above program.
UNMAKEX BIN *  1.0   As above, but ignores copy protection. To use, rename to UNMAKE.BIN

BLDPACK EXE    3.20  Builds a pack image from constituent files.
BOOT    BIN    1.0   Boot code for packs. Automatically runs BOOT procedure.
BUILD   BLD *        Example build file for above program.
MENU    BIN *  1.0   Boot code for packs. Inserts/deletes MENU in main menu.

OPLTRAN EXE    3.20  Translates OPL procedure to OB3 file.

OLDER   ZIP *        Various older versions of the programs above.
