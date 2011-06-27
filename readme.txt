===================================================================
                           MRtrix
===================================================================

The complete documentation for MRtrix is in doc/index.html, 
including a detailed decription of the installation procedure.


Quick install:

1. Unpack archive and compile: 

    tar xjf mrtrix-0.3.tar.bz2
    cd mrtrix-0.3/
    ./configure -nogui
    ./build

2. Set appropriate environment variables:

Bash shell:
    export PATH=/<edit as appropriate>/mrtrix-0.3/bin:$PATH
    export LD_LIBRARY_PATH=/<edit as appropriate>/mrtrix-0.3/lib

C shell:
    setenv PATH /<edit as appropriate>/mrtrix-0.3/bin:$PATH
    setenv LD_LIBRARY_PATH /<edit as appropriate>/mrtrix-0.3/lib

