/*
    Copyright 2009 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 16/10/09.

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

#ifndef __dataset_loop_h__
#define __dataset_loop_h__

namespace MR {
  namespace DataSet {

    //! \addtogroup DataSet
    // @{

    namespace Loop {


      //! used to loop over all image coordinates.
      /*! This function increments the current position to the next voxel, incrementing to the next axis as required.
       * It is used to process all voxels in turn. For example:
       * \code
       * Image::Voxel position (image_object);
       * do {
       *   process (position.value());
       * } while (next (position));
       * \endcode
       * \return true once the last voxel has been reached (i.e. the next increment would bring the current position out of bounds), false otherwise. */
      template <class Set> inline bool next (Set& D)
      {
        size_t axis = 0;
        do {
          if (D.pos (axis)+1 < ssize_t (D.dim(axis))) { D.move (axis,1); return (true); }
          D.pos (axis, 0);
          axis++;
        } while (axis < D.ndim());
        return (false);
      }

      template <class Functor, class Set> inline void all (Functor& func, Set& D) {
        assert (voxel_count (D));
        D.reset();
        do func (D); while (next (D)); 
      }

      template <class Functor, class Set1, class Set2> 
        inline void all (Functor& func, Set1& D1, Set2& D2) {
        assert (voxel_count (D1));
        if (!dimensions_match (D1, D2)) 
          throw Exception ("dimensions mismatch between \"" + D1.name() + "\" and \"" + D2.name() + "\"");
        D1.reset(); D2.reset();
        do func (D1, D2); while (next (D1)); 
      }

      template <class Functor, class Set1, class Set2> 
        inline void all (const std::string& progress_message, Functor& func, Set1& D1, Set2& D2) {
        assert (voxel_count (D1));
        if (!dimensions_match (D1, D2)) 
          throw Exception ("dimensions mismatch between \"" + D1.name() + "\" and \"" + D2.name() + "\"");
        D1.reset(); D2.reset();
        ProgressBar::init (voxel_count (D1), progress_message);
        do { func (D1, D2); ProgressBar::inc(); next (D2); } while (next (D1)); 
        ProgressBar::done();
      }



      template <class Functor, class Set> inline void spatial (Functor& func, Set& D) {
        assert (voxel_count (D));
        D.reset();
        for (D.pos(2,0); D.pos(2) < D.dim(2); D.move(2,1)) 
          for (D.pos(1,0); D.pos(1) < D.dim(1); D.move(1,1)) 
            for (D.pos(0,0); D.pos(0) < D.dim(0); D.move(0,1)) 
              func (D); 
      }

      template <class Functor, class Set1, class Set2> inline void spatial (Functor& func, Set1& D1, Set2& D2) {
        assert (voxel_count (D1));
        if (!dimensions_match (D1, D2, 3)) 
          throw Exception ("dimensions mismatch between \"" + D1.name() + "\" and \"" + D2.name() + "\"");
        D1.reset(); D2.reset();
        for (D1.pos(2,0), D2.pos(2,0); D1.pos(2) < D1.dim(2); D1.move(2,1), D2.move(2,1)) 
          for (D1.pos(1,0), D2.pos(1,0); D1.pos(1) < D1.dim(1); D1.move(1,1), D2.move(1,1)) 
            for (D1.pos(0,0), D2.pos(0,0); D1.pos(0) < D1.dim(0); D1.move(0,1), D2.move(0,1)) 
              func (D1, D2); 
      }

    }

    //! @}
  }
}

#endif


