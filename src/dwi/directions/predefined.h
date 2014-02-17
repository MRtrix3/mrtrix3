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


#ifndef __dwi_directions_predefined_h__
#define __dwi_directions_predefined_h__


#include "math/matrix.h"


namespace MR {
  namespace DWI {
    namespace Directions {

      extern const float electrostatic_repulsion_60_data[];
      extern const float electrostatic_repulsion_300_data[];
      extern const float tesselation_129_data[];
      extern const float tesselation_321_data[];
      extern const float tesselation_469_data[];
      extern const float tesselation_513_data[];
      extern const float tesselation_1281_data[];



      template <typename T> 
        inline Math::Matrix<T>& electrostatic_repulsion_60 (Math::Matrix<T>& dirs)
        {
          dirs = Math::Matrix<T> (const_cast<float*> (electrostatic_repulsion_60_data), 60, 2);
          return dirs;
        }


      template <typename T> 
        inline Math::Matrix<T>& electrostatic_repulsion_300 (Math::Matrix<T>& dirs)
        {
          dirs = Math::Matrix<T> (const_cast<float*> (electrostatic_repulsion_300_data), 300, 2);
          return dirs;
        }



      //! 3rd-order tessellation of an octahedron
      template <typename T> 
        inline Math::Matrix<T>& tesselation_129 (Math::Matrix<T>& dirs)
        {
          dirs = Math::Matrix<T> (const_cast<float*> (tesselation_129_data), 129, 2);
          return dirs;
        }



      //! 3rd-order tessellation of an icosahedron
      template <typename T> 
        inline Math::Matrix<T>& tesselation_321 (Math::Matrix<T>& dirs)
        {
          dirs = Math::Matrix<T> (const_cast<float*> (tesselation_321_data), 321, 2);
          return dirs;
        }


      //! 4th-order tessellation of a tetrahedron
      template <typename T> 
        inline Math::Matrix<T>& tesselation_469 (Math::Matrix<T>& dirs)
        {
          dirs = Math::Matrix<T> (const_cast<float*> (tesselation_469_data), 469, 2);
          return dirs;
        }


      //! 4th-order tessellation of an octahedron
      template <typename T> 
        inline Math::Matrix<T>& tesselation_513 (Math::Matrix<T>& dirs)
        {
          dirs = Math::Matrix<T> (const_cast<float*> (tesselation_513_data), 513, 2);
          return dirs;
        }


      //! 4th-order tessellation of an icosahedron
      template <typename T> 
        inline Math::Matrix<T>& tesselation_1281 (Math::Matrix<T>& dirs)
        {
          dirs = Math::Matrix<T> (const_cast<float*> (tesselation_1281_data), 1281, 2);
          return dirs;
        }




    }
  }
}

#endif

