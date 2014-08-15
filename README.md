# MRtrix

The complete documentation for MRtrix will eventually be found on the [wiki](https://github.com/jdtournier/mrtrix3/wiki),
including a detailed decription of the installation procedure. The official website for MRtrix is hosted on <a href="https://plus.google.com/110975114527807720518" rel="publisher">Google+</a>.


## Quick install:

1. Install dependencies by whichever means your system uses. 
   These include: Python (>=2.6), a C++ compiler, POSIX threads, 
   GNU Scientific Library (GSL, >= 1.1), zlib, OpenGL (>=3), and Qt (>=4.8).

2. Clone Git repository and compile: 

        $ git clone https://github.com/jdtournier/mrtrix3.git
        $ cd mrtrix3/
        $ ./configure 
        $ ./build

3. Set appropriate environment variables:

    * Bash shell:

            $ export PATH=/<edit as appropriate>/mrtrix-0.3/bin:$PATH
 
    * C shell:

            $ setenv PATH /<edit as appropriate>/mrtrix-0.3/bin:$PATH

4. Test installation: 

        $ mrview

