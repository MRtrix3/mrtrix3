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

 /*! \mainpage Overview

    MRtrix was developed with simplicity, performance, flexibility and 
    consistency in mind, which has led to a number of fundamental design 
    decisions. The main concepts are explained in the following pages:

    - The build process is based on a Python script rather than Makefiles,
    and all dependencies are resolved at build-time. This is explained in
    \ref build_page.
    - The configure script allows you to create different co-existing 
    configurations, for example to easily switch from a release build to a 
    debug build.  This is explained in \ref configure_page.
    - You are encouraged to set up your own, separate module on top of the MRtrix
    codebase. This allows you to write your own code, stored on your own 
    (potentially private) repository, without affecting the MRtrix core 
    repository. This is explained in the section \ref module_howto.
    - The basic steps for writing applications based on MRtrix are explained
    in the section \ref command_howto.
    - The concepts behind accessing and processing %Image data are outlined in \ref 
    image_access.
    - There are a number of convenience classes to simplify development of
    multi-threaded applications, in particular the Image::ThreadedLoop and
    Thread::Queue classes. These are outlined in the section \ref
    multithreading

   */

}

