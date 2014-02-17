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

