Brief installation instructions:

- This module relies on the MRtrix 0.3 core source code, as available from the
mrtrix_dev repository. Make sure you've got that downloaded and built before proceeding.

- Checkout this repository into a folder alongside the MRtrix core folder.

- cd into this folder (the MRView source), and from there issue the build
command from the core folder. In other words, if you've checked out the
mrtrix_dev repository into the folder ~/src/mrtrix/core, and the mrtrix_mrview
repository into the folder ~/src/mrtrix/mrview, then type:
  $ cd ~/src/mrtrix/mrview
  $ ../core/build

- If everything has gone according to plan, all the final executables will be
located in the 'bin' folder. You need to make sure that executables are in your search
path. There are many ways to do this, but the simplest is probably: 
  $ export PATH=~/src/mrtrix/mrview/bin:$PATH 
Note that this assumes you already have the paths set for the core (mrtrix_dev)
code, in particular that the shared library is in the linker search path.



- for routine development, it is much easier to set up a symbolic link pointing
to the build script, like so:
  $ cd ~/src/mrtrix/mrview
  $ ln -s ../core/build
at which point you can build by typing:
  $ ./build



- note that you can daisy-chain modules if they rely on each other. For
instance, at some point soon some tractography-specific code will make it into
MRView. You would then need to check out that repository too, say into the
folder ~/src/mrtrix/tractography. You'd need to have the tractography module
build using the core build script: 
  $ cd ~/src/mrtrix/tractography
  $ ln -s ../core/build
and modify the mrview module's symbolic link to point to the tractography module's build link:
  $ cd ~/src/mrtrix/mrview
  $ rm build
  $ ln -s ../tractography/build
at which point you can compile as before, but this time all the code from both
the tractography and core modules will also be included in the search path: 
  $ ./build


