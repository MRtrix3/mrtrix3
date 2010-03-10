/*
    Copyright 2010 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 05/02/10.

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

#ifndef __dataset_stride_h__
#define __dataset_stride_h__

#include "data_type.h"

namespace MR {
  namespace DataSet {

    //! \addtogroup DataSet
    // @{

    //! Functions to handle the memory layout of DataSet classes
    /*! Strides are typically supplied as a symbolic list of increments,
     * representing the layout of the data in memory. In this symbolic
     * representation, the actual magnitude of the strides is only important
     * in that it defines the ordering of the various axes. 
     *
     * For example, the vector of strides [ 3 -1 -2 ] is valid as a symbolic
     * representation of a DataSet stored as a stack of sagittal slices. Each
     * sagittal slice is stored as rows of voxels ordered from anterior to
     * posterior (i.e. negative y: -1), then stacked superior to inferior (i.e.
     * negative z: -2). These slices are then stacked from left to right (i.e.
     * positive x: 3). 
     *
     * This representation is symbolic since it does not take into account the
     * size of the DataSet along each dimension. To be used in practice, these
     * strides must correspond to the number of intensity values to skip
     * between adjacent voxels along the respective axis. For the example
     * above, the DataSet might consists of 128 sagittal slices, each with
     * dimensions 256x256. The dimensions of the DataSet (as returned by dim())
     * are therefore [ 128 256 256 ]. The actual strides needed to navigate
     * through the DataSet, given the symbolic strides above, should therefore
     * be [ 65536 -256 -1 ] (since 256x256 = 65532). 
     *
     * Note that a stride of zero is treated as undefined or invalid. This can
     * be used in the symbolic representation to specify that the ordering of
     * the corresponding axis is not important. A suitable stride will be
     * allocated to that axis when the DataSet is initialised (this is done
     * with a call to sanitise()).
     *
     * The functions defined in this namespace provide an interface to
     * manipulate the strides and convert symbolic into actual strides. */
    namespace Stride {

      typedef std::vector<ssize_t> List;

      //! \cond skip
      namespace {
        template <class Set> class Compare {
          public:
            Compare (const Set& set) : S (set) { }
            bool operator() (const size_t a, const size_t b) const { return (abs(S.stride(a)) < abs(S.stride(b))); }
          private:
            const Set& S;
        };

        class Wrapper {
          public:
            Wrapper (List& strides) : S (strides) { }
            size_t ndim () const { return (S.size()); }
            const ssize_t& stride (size_t axis) const { return (S[axis]); }
            ssize_t& stride (size_t axis) { return (S[axis]); }
          private:
            List& S;
        };

        template <class Set> class WrapperSet : public Wrapper {
          public:
            WrapperSet (List& strides, const Set& set) : Wrapper (strides), D (set) { assert (ndim() == D.ndim()); }
            ssize_t dim (size_t axis) const { return (D.dim(axis)); }
          private:
            const Set& D;
        };
      }
      //! \endcond




      //! return the strides of \a set as a std::vector<ssize_t>
      template <class Set> List get (const Set& set)
      {
        List ret (set.ndim());
        for (size_t i = 0; i < set.ndim(); ++i) 
          ret[i] = set.stride(i);
        return (ret);
      }




      //! sort axes with respect to their absolute stride.
      /*! \return a vector of indices of the axes in order of increasing
       * absolute stride. 
       * \note all strides should be valid (i.e. non-zero). */
      template <class Set> std::vector<size_t> order (const Set& set) 
      {
        std::vector<size_t> ret (set.ndim());
        for (size_t i = 0; i < ret.size(); ++i) ret[i] = i;
        Compare<Set> compare (set);
        std::sort (ret.begin(), ret.end(), compare);
        return (ret);
      }

      //! sort axes with respect to their absolute stride.
      /*! \return a vector of indices of the axes in order of increasing
       * absolute stride. 
       * \note all strides should be valid (i.e. non-zero). */
      template <> inline std::vector<size_t> order<List> (const List& strides) {
        const Wrapper wrapper (const_cast<List&> (strides)); 
        return (order (wrapper)); 
      }

      //! sort range of axes with respect to their absolute stride.
      /*! \return a vector of indices of the axes in order of increasing
       * absolute stride. 
       * \note all strides should be valid (i.e. non-zero). */
      template <class Set> std::vector<size_t> order (const Set& set, size_t from_axis, size_t to_axis) 
      {
        to_axis = std::min (to_axis, set.ndim());
        assert (to_axis > from_axis);
        std::vector<size_t> ret (to_axis-from_axis);
        for (size_t i = 0; i < ret.size(); ++i) ret[i] = from_axis+i;
        Compare<Set> compare (set);
        std::sort (ret.begin(), ret.end(), compare);
        return (ret);
      }




      //! remove duplicate and invalid strides.
      /*! sanitise the strides of DataSet \a set by identifying invalid (i.e.
       * zero) or duplicate (absolute) strides, and assigning to each a
       * suitable value. The value chosen for each sanitised stride is the
       * lowest number greater than any of the currently valid strides. */
      template <class Set> void sanitise (Set& set) 
      {
        size_t max = 0;
        for (size_t i = 0; i < set.ndim()-1; ++i) {
          if (!set.stride(i)) continue;
          if (size_t (abs(set.stride(i))) > max) max = abs (set.stride(i));
          for (size_t j = i+1; j < set.ndim(); ++j) {
            if (!set.stride(j)) continue;
            if (abs(set.stride(i)) == abs(set.stride(j))) set.stride(j) = 0;
          }
        }

        for (size_t i = 0; i < set.ndim(); ++i) {
          if (set.stride(i)) continue;
          set.stride(i) = max++;
        }
      }
      //! remove duplicate and invalid strides.
      /*! sanitise the strides of DataSet \a set by identifying invalid (i.e.
       * zero) or duplicate (absolute) strides, and assigning to each a
       * suitable value. The value chosen for each sanitised stride is the
       * lowest number greater than any of the currently valid strides. */
      template <> inline void sanitise<List> (List& strides) { Wrapper wrapper (strides); sanitise (wrapper); }




      //! convert strides from symbolic to actual strides
      template <class Set> void actualise (Set& set) 
      {
        std::vector<size_t> x (order (set));
        ssize_t skip = 1;
        for (size_t i = 0; i < set.ndim(); ++i) {
          set.stride(x[i]) = set.stride(x[i]) > 0 ? skip : -skip;
          skip *= set.dim(x[i]);
        }
      }
      //! convert strides from symbolic to actual strides
      /*! convert strides from symbolic to actual strides, assuming the strides
       * in \a strides and DataSet dimensions of \a set. */
      template <class Set> void actualise (List& strides, const Set& set) 
      {
        WrapperSet<Set> wrapper (strides, set);
        actualise (wrapper); 
      }




      //! convert strides from actual to symbolic strides
      template <class Set> void symbolise (Set& set) 
      {
        std::vector<size_t> p (order (set));
        for (ssize_t i = 0; i < ssize_t(p.size()); ++i) 
          set.stride(p[i]) = set.stride(p[i]) > 0 ? i+1 : -(i+1);
      }
      //! convert strides from actual to symbolic strides
      template <> inline void symbolise<List> (List& strides) { Wrapper wrapper (strides); symbolise (wrapper); }


      //! calculate offset to start of data
      /*! this function caculate the offset (in number of voxels) from the start of the data region
       * to the first voxel value (i.e. at voxel [ 0 0 0 ... ]). */
      template <class Set> size_t offset (const Set& set) 
      {
        size_t offset = 0;
        for (size_t i = 0; i < set.ndim(); ++i) 
          if (set.stride(i) < 0) 
            offset += size_t(-set.stride(i)) * (set.dim(i) - 1);
        return (offset);
      }

      //! calculate offset to start of data
      /*! this function caculate the offset (in number of voxels) from the start of the data region
       * to the first voxel value (i.e. at voxel [ 0 0 0 ... ]), assuming the
       * strides in \a strides and DataSet dimensions of \a set. */
      template <class Set> size_t offset (List& strides, const Set& set) { WrapperSet<Set> wrapper (strides, set); return (offset (wrapper)); }

    }

    //! @}
  }
}

#endif

