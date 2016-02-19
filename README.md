# MRtrix

[![Build Status](https://travis-ci.org/MRtrix3/mrtrix3.svg?branch=master)](https://travis-ci.org/MRtrix3/mrtrix3)

The complete documentation for MRtrix will eventually be found on the [wiki](https://github.com/MRtrix3/mrtrix3/wiki),
including a detailed decription of the installation procedure. The official website for MRtrix is www.mrtrix.org.

## Getting help 

Support and general discussion is hosted on the [MRtrix3 Google+ Community
page](https://plus.google.com/u/0/communities/111072048088633408015). Please
address all MRtrix3-related queries there. You will need to create a Google+
account if you don't already have one.

## Quick install

1. Install dependencies by whichever means your system uses. 
   These include: Python (>=2.6), a C++ compiler, POSIX threads, 
   GNU Scientific Library (GSL, >= 1.1), zlib, OpenGL (>=3), and Qt (>=4.8).
   
   * CentOS users:
   ```bash
      yum install gcc-c++ zlib-devel gsl-devel qt-devel
      export PATH=$PATH:/usr/lib64/qt4/bin
   ```
   
2. Clone Git repository and compile: 

        $ git clone https://github.com/MRtrix3/mrtrix3.git
        $ cd mrtrix3/
        $ ./configure 
        $ ./build

3. Set appropriate environment variables:

    * Bash shell:

            $ export PATH=/<edit as appropriate>/mrtrix3/bin:$PATH
 
    * C shell:

            $ setenv PATH /<edit as appropriate>/mrtrix3/bin:$PATH

4. Test installation: 

        $ mrview

## Keeping MRtrix3 up to date

1. You can update your installation at any time by opening a terminal in the mrtrix3 folder, and typing:

        git pull
		./build
		
2. If this doesn't work immediately, it may be that you need to re-run the configure script:

        ./configure

    and re-run step 1 again.
