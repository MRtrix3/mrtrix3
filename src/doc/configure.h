/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 16/08/09.

   This file is part of MRtrix.

   MRtrix is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   MRtrix is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

 */

#error - this file is for documentation purposes only!
#error - It should NOT be included in other code files.

namespace MR
{

  /*! \page configure_page The configure script

 The first step in compiling MRtrix is to invoke the configure script. This
 will run a series of tests to ensure the required dependencies are installed,
 and figure out the appropriate compilation settings. By default, this
 information is stored in the \c config.default file, ready for use with the
 \ref build_page "build script".

 The configure script accepts a number of options, and is influenced by a
 number of environment variables. These are all described in detail in
 configure script's online help, produced by invoking it with the \c -help
option:
 \code
 $ ./configure -help
 \endcode
 These options should be self-explanatory, and will not be described any
 further here. This section instead focuses on describing the multi-config
 feature of MRtrix.

 \section multiconfig Using multiple co-existing configurations

 By default, the configure script will generate a file called \c
 config.default, with the relevant settings requested by the options supplied
 to the configure script. This is a text file consisting of Python commands,
 which you're encouraged to look through. It is possible to request that the
 information be stored in a different file, so that non-standard settings can
 be used to build a different set of binaries, without affecting the existing
 set. This is best illustrated with an example:
 

 If we want to generate a configuration for a debug build, the \c -debug option
 can be passed to the configure script to generate a suitable configuration for
 it. If we supply a suitable name as an argument, the settings will be stored
 in a file called \c config.&lt;name&gt; instead of the default \c
 config.default. In our case, we chose the name \c dbg:
\code
$ ./configure -debug dbg
\endcode
This generates a file called \c config.dbg that co-exists with the default \c
config.default file. We can then ask the build script to compile the debug
version of the binaries by passing the name \c dbg as the target:
\code 
$ ./build dbg
\endcode
To avoid naming conflicts with the existing binaries, all intermediate files
generated with this command will have the suffix \c __dbg appended to the
filename, before the file extension. The debug version of the command can then
be invoked as, for example: 
\code
$ mrconvert__dbg in.mif out.nii
\endcode

This allows any number of configurations to co-exist without interferring with
each other. Other potentially interesting configurations include:
-# one that does not include the MRtrix shared library, configured with the
-noshared option. This makes it easier to distribute the binaries without
having to set the linker path for the MRtrix shared library on the target
systems.
-# one that does not include the GUI components, configured with the -nogui
option.
-# one that is statically linked, configured with the -static option. This
makes it easy to install on target systems where the required dependencies
cannot be installed.

If required, it is also possible to create a configuration by hand. This may be
useful in cases where the configuration produced by the configure script does
not work as-is, and requires manual editing. 

 */
 
}

