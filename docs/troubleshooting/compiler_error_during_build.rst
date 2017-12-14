Compiler error during build
===========================

If you encounter an error during the build process that resembles the following::

    ERROR: (#/#) [CC] release/cmd/command.o

    /usr/bin/g++-4.8 -c -std=c++11 -pthread -fPIC -I/home/user/mrtrix3/eigen -Wall -O2 -DNDEBUG -Isrc -Icmd -I./lib -Icmd cmd/command.cpp -o release/cmd/command.o

    failed with output

    g++-4.8: internal compiler error: Killed (program cc1plus)
    Please submit a full bug report,
    with preprocessed source if appropriate.
    See for instructions.


This is most typically caused by the compiler running out of RAM. This
can be solved either through installing more RAM into your system, or
by restricting the number of threads to be used during compilation::

    NUMBER_OF_PROCESSORS=1 ./build
