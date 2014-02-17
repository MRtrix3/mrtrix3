/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


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

