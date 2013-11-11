===================================================================
                           MRtrix
===================================================================

The complete documentation for MRtrix is in doc/index.html, 
including a detailed decription of the installation procedure.


Quick install:

1. Install dependencies by whichever means your system uses. 
   These include: Python (>=2.6), a C++ compiler, POSIX threads, 
   GNU Scientific Library (GSL, >= 1.1), zlib, OpenGL (>=3), and Qt4.

2. Unpack archive and compile: 

    tar xjf mrtrix-0.3.tar.bz2
    cd mrtrix-0.3/
    ./configure 
    ./build

3. Set appropriate environment variables:

Bash shell:
    export PATH=/<edit as appropriate>/mrtrix-0.3/bin:$PATH

C shell:
    setenv PATH /<edit as appropriate>/mrtrix-0.3/bin:$PATH


