===================================================================
                           MRtrix
===================================================================

The complete documentation for MRtrix is in doc/index.html, 
including a detailed decription of the installation procedure.


Quick install:

1. Unpack archive and compile: 

    tar xjf mrtrix-0.2.1.tar.bz2
    cd mrtrix-0.2.1/
    ./build

2. Set appropriate environment variables:

Bash shell:
    export PATH=/<edit as appropriate>/mrtrix-0.2.1/bin:$PATH
    export LD_LIBRARY_PATH=/<edit as appropriate>/mrtrix-0.2.1/lib

C shell:
    setenv PATH /<edit as appropriate>/mrtrix-0.2.1/bin:$PATH
    setenv LD_LIBRARY_PATH /<edit as appropriate>/mrtrix-0.2.1/lib

