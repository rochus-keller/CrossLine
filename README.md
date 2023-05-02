# Welcome to CrossLine

CrossLine is an outliner with sophisticated cross-link capabilities in the tradition of the well-respected ￼ Ecco Pro. It implements the concept of "Transclusion" proposed by Ted Nelson and - among others - implemented in the legendary Objectory SE tool by Ivar Jacobson. It is also a full text database with built-in search engine.

An outliner (or outline processor) is a specialized type of text editor (word processor) used to create and edit outlines, which are text files which have a tree structure, for organization. Textual information is contained in discrete sections called "nodes", which are arranged according to their topic–subtopic (parent–child) relationships, sort of like the members of a family tree. When loaded into an outliner, an outline may be collapsed or expanded to display as few or as many levels as desired (see https://en.wikipedia.org/wiki/Outliner).

![alt text](http://software.rochus-keller.ch/crosslinedemoscreenshot.png "CrossLine Screenshot")

## Download and Installation

You can either compile CrossLine yourself or download the pre-compiled version from here: 

- [Windows x86](http://software.rochus-keller.ch/CrossLine_win32.zip)
- [Windows x64](http://software.rochus-keller.ch/CrossLine_win64.zip)
- [Linux x86](http://software.rochus-keller.ch/CrossLine_linux_x86.tar.gz)
- [Linux x64](http://software.rochus-keller.ch/CrossLine_linux_x64.tar.gz)
- [Mac x64](http://software.rochus-keller.ch/CrossLine_macos_x64.zip)
- [Mac M1](http://software.rochus-keller.ch/CrossLine_macos_m1.zip)


This is a compressed single-file executable which was built using the source code from here. Of course you can build the executable yourself if you want (see below for instructions). Since it is a single executable, it can just be downloaded and unpacked. No installation is necessary. You therefore need no special privileges to run CrossLine on your machine. 

On Mac the terminal opens when CrossLine is run, and the menus are only active if the application was in the background one time; to avoid this the application can be included in an application bundle. Also note that the application on Mac must be started via the "open" command from the context menu; otherwise the system refuses to start the app.

Note that the Windows versions are built with MT flag using a statically linked C/C++ runtime, so no Microsoft runtime has to be installed. The executable runs even on Windows 7.

Here is a demo file with some instructions on how to use CrossLine: [CrossLineDemo.cldb](http://software.rochus-keller.ch/CrossLineDemo.cldb)

Here are the old Qt4 based versions if need be (use the new versions above if possile):

- [Windows x86](http://software.rochus-keller.ch/CrossLine_win32_qt4.zip)
- [Linux x86](http://software.rochus-keller.ch/CrossLine_linux_x86_qt4.tar.gz)


## How to Build CrossLine

This version of CrossLine now uses [LeanQt](https://github.com/rochus-keller/LeanQt) instead of the modified Qt4 toolkit, which makes things easier. 
Follow these steps if you want to build CrossLine yourself:

1. Create a new directory; we call it the root directory here.
1. Download https://github.com/rochus-keller/BUSY/archive/refs/heads/master.zip and unpack it to the root directory; rename the resulting directory to "build".
1. Download https://github.com/rochus-keller/LeanQt/archive/refs/heads/master.zip and unpack it to the root directory; rename the resulting directory to "LeanQt".
1. Download https://github.com/rochus-keller/GuiTools/archive/refs/heads/master.zip and unpack it to the root directory; rename the resulting directory to "GuiTools".
1. Download the CrossLine source code from https://github.com/rochus-keller/CrossLine/archive/master.zip and unpack it to the root directory; rename the resulting directory to "CrossLine".
1. Download https://github.com/rochus-keller/Fts/archive/refs/heads/leanqt.zip and unpack it to the root directory; rename the resulting directory to "Fts".
1. Download https://github.com/rochus-keller/Oln2/archive/refs/heads/leanqt.zip and unpack it to the root directory; rename the resulting directory to "Oln2".
1. Download https://github.com/rochus-keller/Stream/archive/refs/heads/leanqt.zip and unpack it to the root directory; rename the resulting directory to "Stream".
1. Download https://github.com/rochus-keller/Txt/archive/refs/heads/leanqt.zip and unpack it to the root directory; rename the resulting directory to "Txt".
1. Download https://github.com/rochus-keller/Udb/archive/refs/heads/leanqt.zip and unpack it to the root directory; rename the resulting directory to "GuiTools".
1. Create the subdirectory "Sqlite3" the root directory; download the Sqlite source from http://software.rochus-keller.ch/Sqlite3.tar.gz and unpack it to the subdirectory.
1. Open a command line in the build directory and type `cc *.c -O2 -lm -o lua` or `cl /O2 /MD /Fe:lua.exe *.c` depending on whether you are on a Unix or Windows machine; wait a few seconds until the Lua executable is built.
1. Now type `./lua build.lua ../CrossLine` (or `lua build.lua ../CrossLine` on Windows); wait until the CrossLine executable is built; you find it in the output subdirectory.

NOTE that if you build on Windows you have to first run vcvars32.bat or vcvars64.bat provided e.g. by VisualStudio (see e.g. [here](https://learn.microsoft.com/en-us/cpp/build/building-on-the-command-line?view=msvc-170) for more information) from the command line to set all required paths and environment variables.

If you already have a [LeanCreator](https://github.com/rochus-keller/LeanCreator/) executable on your machine, you can alternatively open the root_directory/CrossLine/BUSY file with LeanCreator and build it there using all available CPU cores (don't forget to switch to Release mode); this is simpler and faster than the command line build.

## Support
If you need support or would like to post issues or feature requests please use the Github issue list at https://github.com/rochus-keller/CrossLine/issues or send an email to the author.



