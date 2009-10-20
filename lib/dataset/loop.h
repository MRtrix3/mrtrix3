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

      class ContiguousAxis {
        public:
          ContiguousAxis (size_t axis_idx, size_t dim, ssize_t dir) : 
            axis (axis_idx), from (dir > 0 ? 0 : dim-1), to (dir > 0 ? dim-1 : 0), inc (dir) { }
          size_t axis;
          ssize_t from, to, inc;
      };



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



      class next_contiguous {
        public:
          template <class Set> next_contiguous (Set& D) { 
            assert (voxel_count (D));
            const Layout* layout = D.layout();
            for (size_t n = 0; n < D.ndim(); ++n) 
              A.push_back (ContiguousAxis (layout[n].axis, D.dim(layout[n].axis), layout[n].dir));
          }
          template <class Set> bool operator() (Set& D) {
            assert (voxel_count (D1));
            size_t n = 0;
            do {
              if (D.pos (A[n].axis) != A[n].to) { D.move (A[n].axis, A[n].inc); return (true); }
              D.pos (A[n].axis, A[n].from);
              ++n;
            } while (n < A.size());
            return (false);
          }
          template <class Set> void reset (Set& D) { for (size_t n = 0; n < A.size(); ++n) D.pos (A[n].axis, A[n].from); }
        private:
          std::vector<ContiguousAxis> A;
      };




      template <class Functor, class Set> 
        inline void all_contiguous (Functor& func, Set& D) {
        next_contiguous next (D);
        next.reset (D);
        do func (D); while (next (D)); 
      }


      template <class Functor, class Set1, class Set2> 
        inline void all_contiguous (Functor& func, Set1& D1, Set2& D2) {
        if (!dimensions_match (D1, D2)) 
          throw Exception ("dimensions mismatch between \"" + D1.name() + "\" and \"" + D2.name() + "\"");
        next_contiguous next (D1);
        next.reset (D1);
        next.reset (D2);
        do { func (D1, D2); next (D2); } while (next (D1)); 
      }


      template <class Functor, class Set1, class Set2> 
        inline void all_contiguous (const std::string& progress_message, Functor& func, Set1& D1, Set2& D2) {
        if (!dimensions_match (D1, D2)) 
          throw Exception ("dimensions mismatch between \"" + D1.name() + "\" and \"" + D2.name() + "\"");
        next_contiguous next (D1);
        next.reset (D1);
        next.reset (D2);
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


      template <class Functor, class Set> inline void spatial_contiguous (Functor& func, Set& D) {
        assert (voxel_count (D));
        const Layout* layout = D.layout();
        ContiguousAxis A[3];
        size_t a = 0;
        for (size_t n = 0; a < 3 && n < D.ndim(); ++n) {
          if (layout[n].axis < 3) {
            A[a] = ContiguousAxis (layout[n].axis, D.dim(layout[n].axis), layout[n].dir);
            ++a;
          }
        }
        assert (a == 3);
        D.reset();
        for (D.pos(A[2].axis,A[2].from); D.pos(A[2].axis) != A[2].to; D.move(A[2].axis,A[2].inc)) 
          for (D.pos(A[1].axis,A[1].from); D.pos(A[1].axis) != A[1].to; D.move(A[1].axis,A[1].inc)) 
            for (D.pos(A[0].axis,A[0].from); D.pos(A[0].axis) != A[0].to; D.move(A[0].axis,A[0].inc)) 
              func (D); 
      }

    }
    //! @}
  }
}

#endif


