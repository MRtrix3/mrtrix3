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

  /*! \page module_howto Writing separate modules

    The MRtrix \ref build_page "build process" allows for the easy development
    of separate modules, compiled against the MRtrix core (or indeed against
    any other MRtrix module). This allows developers to maintain their own
    repository without affecting the MRtrix core. The obvious benefit is that
    developers can keep their own developments private if they wish to, and the
    MRtrix core can be kept as lean as possible. 

    A module simply consists of a separate folder, with its own \c cmd/ folder,
    and potentially its own \c src/ folder if required. The \ref build_page
    "build process" is then almost identical to that for the MRtrix core, with
    a few differences. 
    
    The most relevant difference is how the build script is invoked. For a
    module, compilation is started by invoking the MRtrix core's build script,
    from the module's top-level folder. For example, if the MRtrix core resides
    in the folder \c ~/src/mrtrix/core, and the module resides in \c
    ~/src/mrtrix/mymodule, then the module can be compiled by typing:
    \code
    $ cd ~/src/mrtrix/mymodule
    $ ../core/build
    \endcode
    For routine use, it is more convenient to set up a symbolic link pointing to the MRtrix core's build script, and invoke that:
    \code
    $ cd ~/src/mrtrix/mymodule
    $ ln -s ../core/build
    $ ./build
    \endcode

    Using a symbolic link as above has the added benefit that modules can be
    daisy-chained together. If a new module is created that uses code from
    another module's \c src folder, the dependencies can be met simply by
    creating a symbolic link (called \c build) from the new module's folder and
    pointing to the required module's build script (which will itself be a
    symbolic link to the MRtrix core's build script). For example, if another
    module is created in the folder ~/src/mrtrix/experimental, which builds on
    code in the module in \c ~/src/mrtrix/mymodule (which has already been set
    up as described above), then these commands will allow it to be compiled
    directly: 
    \code
    $ cd ~/src/mrtrix/experimental
    $ ln -s ../mymodule/build
    $ ./build
    \endcode

   */
}
