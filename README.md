# MRtrix

[![Build Status](https://github.com/MRtrix3/mrtrix3/workflows/checks/badge.svg)](https://github.com/MRtrix3/mrtrix3/actions)
[![@MRtrix3](http://img.shields.io/twitter/follow/MRtrix3.svg?style=social)](https://twitter.com/MRtrix3)

*MRtrix3* can be installed / run through multiple avenues:
- [Direct download](https://www.mrtrix.org/download/) through mechanisms tailored for different OS platforms;
- Compiled from the source code in this repository, for which [comprehensive instructions](https://mrtrix.readthedocs.io/en/latest/installation/build_from_source.html) are provided in the [online documentation](https://mrtrix.readthedocs.io/en/);
- Via containerisation technology using Docker or Singularity; see [online documentation page](https://mrtrix.readthedocs.org/en/latest/installation/using_containers.html) for details.

## Getting help

Instructions on software setup and use are provided in the [online documentation](https://mrtrix.readthedocs.org).
Support and general discussion is hosted on the [*MRtrix3* Community Forum](http://community.mrtrix.org/).
Please also look through the Frequently Asked Questions on the [wiki section of the forum](http://community.mrtrix.org/c/wiki).
You can address all *MRtrix3*-related queries there, using your GitHub or Google login to post questions.

## Quick install

1. Install dependencies by whichever means your system uses. 
   These include: Python (>=2.6), a C++ compiler with full C++11 support (`g++` 4.9 or later, `clang++`), 
   Eigen (>=3.2.8), zlib, OpenGL (>=3.3), and Qt (>=4.8, or at least 5.1 on MacOSX).

2. Clone Git repository and compile:

        $ git clone https://github.com/MRtrix3/mrtrix3.git
        $ cd mrtrix3/
        $ ./configure
        $ ./build

3. Set the `PATH`:

    * Bash shell:

      run the `set_path` script provided:

            $ ./set_path

      or edit the startup `~/.bashrc` or `/etc/bash.bashrc` file manually by adding this line:

            $ export PATH=/<edit as appropriate>/mrtrix3/bin:$PATH

    * C shell:

      edit the startup `~/.cshrc` or `/etc/csh.cshrc` file manually by adding this line:

            $ setenv PATH /<edit as appropriate>/mrtrix3/bin:$PATH

4. Test installation:

    Command-line:

        $ mrconvert

    GUI:

        $ mrview

## Keeping MRtrix3 up to date

1. You can update your installation at any time by opening a terminal in the mrtrix3 folder, and typing:

        git pull
        ./build

2. If this doesn't work immediately, it may be that you need to re-run the configure script:

        ./configure

    and re-run step 1 again.

## Building a specific release of MRtrix3

You can build a particular release of MRtrix3 by checking out the corresponding _tag_, and using the same procedure as above to build it:

    git checkout 3.0_RC3
    ./configure
    ./build
    
## Contributing

Thank you for your interest in contributing to *MRtrix3*! Please read on [here](CONTRIBUTING.md) to find out how to report issues, request features and make direct contributions. 
