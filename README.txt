Brief installation instructions:

- Checkout this repository into its own folder, say ~/src/mrtrix/core

- cd into this folder and launch the configure script: $ cd ~/src/mrtrix/core $
./configure For help using the configure script, use the -help option: $
./configure -help

- If successful, you can start the compilation proper using the build script: $
./build

- If everything has gone according to plan, all the final executables will be
located in the 'bin' folder, and by default the shared library will be located
in the 'lib' folder. You need to make sure that executables are in your search
path, and the shared library (if you have one) is in the linker's search path.
There are many ways to do this, but the simplest is probably: $ export
PATH=~/src/mrtrix/core/bin:$PATH
LD_LIBRARY_PATH=~/src/mrtrix/core/lib:$LD_LIBRARY_PATH



- If you plan on contributing to MRtrix, use Doxygen to gerenate documentation
for the code, and take a look through some of the overview pages to get a feel
for how things work. In particular, read the section on setting up your own
module, so you can create your own commands without modifying the MRtrix core, 
which allows you to keep your code separate and if needed, private.

