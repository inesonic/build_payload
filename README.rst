=============
build_payload
=============
The build_payload tool is a small utility that converts a arbitrary file, in
binary format, to a C++ file that can be compiled into your own programs.  The
build_payload command can optionally compress the supplied file.

You can use the ``--help`` command line options for details on using this tool.


Build Dependencies
==================
The build_payload command requires the Qt libraries, version 5 or later.  The
build process on Linux and MacOS is:

    $ qmake
    $ make

The build process on Windows is:

    $ qmake
    $ nmake

