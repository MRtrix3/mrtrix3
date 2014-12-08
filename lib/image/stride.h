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

#ifndef __image_stride_h__
#define __image_stride_h__

#include "app.h"
#include "debug.h"
#include "datatype.h"
#include "math/math.h"

namespace MR
{
  namespace Image
  {

    //! Functions to handle the memory layout of InfoType classes
    /*! Strides are typically supplied as a symbolic list of increments,
     * representing the layout of the data in memory. In this symbolic
     * representation, the actual magnitude of the strides is only important
     * in that it defines the ordering of the various axes.
     *
     * For example, the vector of strides [ 3 -1 -2 ] is valid as a symbolic
     * representation of a InfoType stored as a stack of sagittal slices. Each
     * sagittal slice is stored as rows of voxels ordered from anterior to
     * posterior (i.e. negative y: -1), then stacked superior to inferior (i.e.
     * negative z: -2). These slices are then stacked from left to right (i.e.
     * positive x: 3).
     *
     * This representation is symbolic since it does not take into account the
     * size of the InfoType along each dimension. To be used in practice, these
     * strides must correspond to the number of intensity values to skip
     * between adjacent voxels along the respective axis. For the example
     * above, the InfoType might consists of 128 sagittal slices, each with
     * dimensions 256x256. The dimensions of the InfoType (as returned by dim())
     * are therefore [ 128 256 256 ]. The actual strides needed to navigate
     * through the InfoType, given the symbolic strides above, should therefore
     * be [ 65536 -256 -1 ] (since 256x256 = 65532).
     *
     * Note that a stride of zero is treated as undefined or invalid. This can
     * be used in the symbolic representation to specify that the ordering of
     * the corresponding axis is not important. A suitable stride will be
     * allocated to that axis when the InfoType is initialised (this is done
     * with a call to sanitise()).
     *
     * The functions defined in this namespace provide an interface to
     * manipulate the strides and convert symbolic into actual strides. */
    namespace Stride
    {

      typedef std::vector<ssize_t> List;

      extern const App::OptionGroup StrideOption;

      template <class InfoType> 
        void set_from_command_line (InfoType& info, const List& default_strides = List())
        {
          App::Options opt = App::get_options ("stride");
          size_t n;
          if (opt.size()) {
            std::vector<int> strides = opt[0][0];
            if (strides.size() > info.ndim())
              WARN ("too many axes supplied to -stride option - ignoring remaining strides");
            for (n = 0; n < std::min (info.ndim(), strides.size()); ++n)
              info.stride(n) = strides[n];
            for (; n < info.ndim(); ++n)
              info.stride(n) = 0;
          } 
          else if (default_strides.size()) {
            for (n = 0; n < std::min (info.ndim(), default_strides.size()); ++n) 
              info.stride(n) = default_strides[n];
            for (; n < info.ndim(); ++n)
              info.stride(n) = 0;
          }
        }





      //! \cond skip
      namespace
      {
        template <class InfoType> 
          class Compare
          {
            public:
              Compare (const InfoType& info) : S (info) { }
              bool operator() (const size_t a, const size_t b) const {
                if (S.stride(a) == 0)
                  return false;
                if (S.stride(b) == 0)
                  return true;
                return std::abs (S.stride (a)) < std::abs (S.stride (b));
              }
            private:
              const InfoType& S;
          };

        class Wrapper
        {
          public:
            Wrapper (List& strides) : S (strides) { }
            size_t ndim () const {
              return S.size();
            }
            const ssize_t& stride (size_t axis) const {
              return S[axis];
            }
            ssize_t& stride (size_t axis) {
              return S[axis];
            }
          private:
            List& S;
        };

        template <class InfoType> 
          class InfoWrapper : public Wrapper
        {
          public:
            InfoWrapper (List& strides, const InfoType& info) : Wrapper (strides), D (info) {
              assert (ndim() == D.ndim());
            }
            ssize_t dim (size_t axis) const {
              return D.dim (axis);
            }
          private:
            const InfoType& D;
        };
      }
      //! \endcond




      //! return the strides of \a info as a std::vector<ssize_t>
      template <class InfoType> 
        List get (const InfoType& info)
        {
          List ret (info.ndim());
          for (size_t i = 0; i < info.ndim(); ++i)
            ret[i] = info.stride (i);
          return ret;
        }

      //! set the strides of \a info from a std::vector<ssize_t>
      template <class InfoType>
        void set (InfoType& info, const List& stride)
        {
          size_t n = 0;
          for (; n < std::min (info.ndim(), stride.size()); ++n)
            info.stride (n) = stride[n];
          for (; n < stride.size(); ++n)
            info.stride (n) = 0;
        }

      //! set the strides of \a info from another Infotype
      template <class InfoType, class FromInfoType>
        void set (InfoType& info, const FromInfoType& from)
        {
          set (info, get(from));
        }




      //! sort range of axes with respect to their absolute stride.
      /*! \return a vector of indices of the axes in order of increasing
       * absolute stride.
       * \note all strides should be valid (i.e. non-zero). */
      template <class InfoType> 
        std::vector<size_t> order (const InfoType& info, size_t from_axis = 0, size_t to_axis = std::numeric_limits<size_t>::max())
        {
          to_axis = std::min (to_axis, info.ndim());
          assert (to_axis > from_axis);
          std::vector<size_t> ret (to_axis-from_axis);
          for (size_t i = 0; i < ret.size(); ++i) 
            ret[i] = from_axis+i;
          Compare<InfoType> compare (info);
          std::sort (ret.begin(), ret.end(), compare);
          return ret;
        }

      //! sort axes with respect to their absolute stride.
      /*! \return a vector of indices of the axes in order of increasing
       * absolute stride.
       * \note all strides should be valid (i.e. non-zero). */
      template <> inline 
        std::vector<size_t> order<List> (const List& strides, size_t from_axis, size_t to_axis)
        {
          const Wrapper wrapper (const_cast<List&> (strides));
          return order (wrapper, from_axis, to_axis);
        }




      //! remove duplicate and invalid strides.
      /*! sanitise the strides of InfoType \a info by identifying invalid (i.e.
       * zero) or duplicate (absolute) strides, and assigning to each a
       * suitable value. The value chosen for each sanitised stride is the
       * lowest number greater than any of the currently valid strides. */
      template <class InfoType> 
        void sanitise (InfoType& info)
        {
          // remove duplicates
          for (size_t i = 0; i < info.ndim()-1; ++i) {
            if (!info.stride (i)) continue;
            for (size_t j = i+1; j < info.ndim(); ++j) {
              if (!info.stride (j)) continue;
              if (std::abs (info.stride (i)) == std::abs (info.stride (j))) info.stride (j) = 0;
            }
          }

          size_t max = 0;
          for (size_t i = 0; i < info.ndim(); ++i)
            if (size_t (std::abs (info.stride (i))) > max)
              max = std::abs (info.stride (i));

          for (size_t i = 0; i < info.ndim(); ++i) {
            if (info.stride (i)) continue;
            info.stride (i) = ++max;
          }
        }



      //! remove duplicate and invalid strides.
      /*! sanitise the strides of InfoType \a info by identifying invalid (i.e.
       * zero) or duplicate (absolute) strides, and assigning to each a
       * suitable value. The value chosen for each sanitised stride is the
       * lowest number greater than any of the currently valid strides. */
      template <> 
        inline void sanitise<List> (List& strides)
        {
          Wrapper wrapper (strides);
          sanitise (wrapper);
        }


      //! remove duplicate and invalid strides.
      /*! sanitise the strides in \a current by identifying invalid (i.e.
       * zero) or duplicate (absolute) strides, and assigning to each a
       * suitable value. The value chosen for each sanitised stride is the
       * lowest number greater than any of the currently valid strides. */
      List& sanitise (List& current, const List& desired);


      //! convert strides from symbolic to actual strides
      template <class InfoType> 
        void actualise (InfoType& info)
        {
          sanitise (info);
          std::vector<size_t> x (order (info));
          ssize_t skip = 1;
          for (size_t i = 0; i < info.ndim(); ++i) {
            info.stride (x[i]) = info.stride (x[i]) < 0 ? -skip : skip;
            skip *= info.dim (x[i]);
          }
        }
      //! convert strides from symbolic to actual strides
      /*! convert strides from symbolic to actual strides, assuming the strides
       * in \a strides and InfoType dimensions of \a info. */
      template <class InfoType> 
        inline void actualise (List& strides, const InfoType& info)
        {
          InfoWrapper<InfoType> wrapper (strides, info);
          actualise (wrapper);
        }

      //! get actual strides:
      template <class InfoType> 
        inline List get_actual (InfoType& info)
        {
          List strides (get (info));
          actualise (strides, info);
          return strides;
        }

      //! get actual strides:
      template <class InfoType> 
        inline List get_actual (const List& strides, const InfoType& info)
        {
          List out (strides);
          actualise (out, info);
          return out;
        }



      //! convert strides from actual to symbolic strides
      template <class InfoType> 
        void symbolise (InfoType& info)
        {
          std::vector<size_t> p (order (info));
          for (ssize_t i = 0; i < ssize_t (p.size()); ++i)
            if (info.stride (p[i]) != 0)
              info.stride (p[i]) = info.stride (p[i]) < 0 ? -(i+1) : i+1;
        }
      //! convert strides from actual to symbolic strides
      template <> 
        inline void symbolise (List& strides)
        {
          Wrapper wrapper (strides);
          symbolise (wrapper);
        }

      //! get symbolic strides:
      template <class InfoType> 
        inline List get_symbolic (const InfoType& info)
        {
          List strides (get (info));
          symbolise (strides);
          return strides;
        }

      //! get symbolic strides:
      template <> 
        inline List get_symbolic (const List& list)
        {
          List strides (list);
          symbolise (strides);
          return strides;
        }


      //! calculate offset to start of data
      /*! this function caculate the offset (in number of voxels) from the start of the data region
       * to the first voxel value (i.e. at voxel [ 0 0 0 ... ]). */
      template <class InfoType> 
        size_t offset (const InfoType& info)
        {
          size_t offset = 0;
          for (size_t i = 0; i < info.ndim(); ++i)
            if (info.stride (i) < 0)
              offset += size_t (-info.stride (i)) * (info.dim (i) - 1);
          return offset;
        }

      //! calculate offset to start of data
      /*! this function caculate the offset (in number of voxels) from the start of the data region
       * to the first voxel value (i.e. at voxel [ 0 0 0 ... ]), assuming the
       * strides in \a strides and InfoType dimensions of \a info. */
      template <class InfoType> 
        size_t offset (List& strides, const InfoType& info)
        {
          InfoWrapper<InfoType> wrapper (strides, info);
          return offset (wrapper);
        }



      //! produce strides from \c current that match those specified in \c desired
      /*! The strides in \c desired should be specified as symbolic strides,
       * and any zero strides will be ignored and replaced with sensible values
       * if needed.  Essentially, this function checks whether the symbolic
       * strides in \c current already match those specified in \c desired. If so,
       * these will be used as-is, otherwise a new set of strides based on \c
       * desired will be produced, as follows. First, non-zero strides in \c
       * desired are used as-is, then the remaining strides are taken from \c
       * current where specified and used with higher values, followed by those
       * strides not specified in either.
       *
       * Note that strides are considered matching even if the differ in their
       * sign - this purpose of this function is to ensure contiguity in RAM
       * along the desired axes, and a reversal in the direction of traversal
       * is not considered to affect this.
       *
       * Examples:
       * - \c current: [ 1 2 3 4 ], \c desired: [ 0 0 0 1 ] => [ 2 3 4 1 ]
       * - \c current: [ 3 -2 4 1 ], \c desired: [ 0 0 0 1 ] => [ 3 -2 4 1 ]
       * - \c current: [ -2 4 -3 1 ], \c desired: [ 1 2 3 0 ] => [ 1 2 3 4 ]
       * - \c current: [ -1 2 -3 4 ], \c desired: [ 1 2 3 0 ] => [ -1 2 -3 4 ]
       *   */
      template <class InfoType> 
        List get_nearest_match (const InfoType& current, const List& desired)
        {
          List in (get_symbolic (current)), out (desired);
          out.resize (in.size(), 0);

          for (size_t i = 0; i < out.size(); ++i) 
            if (out[i]) 
              if (std::abs (out[i]) != std::abs (in[i])) 
                return sanitise (in, out);

          sanitise (in);
          return in;
        }


      template <> 
        inline List get_nearest_match<List> (const List& strides, const List& desired) 
        {
          Wrapper wrapper (const_cast<List&> (strides));
          return get_nearest_match (wrapper, desired);
        }



      //! convenience function for use with Image::BufferPreload
      /*! when passed as the second argument to the Image::BufferPreload
       * constructor, ensures the specified axis will be contiguous in RAM. */
      inline List contiguous_along_axis (size_t axis) 
      {
        List strides (axis+1,0);
        strides[axis] = 1;
        return strides;
      }

      //! convenience function for use with Image::BufferPreload
      /*! when passed as the second argument to the Image::BufferPreload
       * constructor, ensures the specified axis will be contiguous in RAM,
       * while matching the strides in \a info as closely as possible. */
      template <class InfoType> 
        inline List contiguous_along_axis (size_t axis, const InfoType& info) 
        {
          return get_nearest_match (Image::Stride::get (info), contiguous_along_axis (axis));
        }

      //! convenience function for use with Image::BufferPreload
      /*! when passed as the second argument to the Image::BufferPreload
       * constructor, ensures the spatial axes will be contiguous in RAM,
       * preserving the original order as on file as closely as possible. */
      template <class InfoType>
        inline List contiguous_along_spatial_axes (const InfoType& info)
        {
          List strides = get (info);
          for (size_t n = 3; n < strides.size(); ++n)
            strides[n] = 0;
          return strides;
        }



    }
  }
}

#endif

