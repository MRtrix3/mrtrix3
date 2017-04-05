/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */

#ifndef __stride_h__
#define __stride_h__

#include "app.h"
#include "types.h"
#include "debug.h"
#include "datatype.h"
#include "math/math.h"

namespace MR
{

  //! Functions to handle the memory layout of images
  /*! Strides are typically supplied as a symbolic list of increments,
   * representing the layout of the data in memory. In this symbolic
   * representation, the actual magnitude of the strides is only important
   * in that it defines the ordering of the various axes.
   *
   * For example, the vector of strides [ 3 -1 -2 ] is valid as a symbolic
   * representation of a image stored as a stack of sagittal slices. Each
   * sagittal slice is stored as rows of voxels ordered from anterior to
   * posterior (i.e. negative y: -1), then stacked superior to inferior (i.e.
   * negative z: -2). These slices are then stacked from left to right (i.e.
   * positive x: 3).
   *
   * This representation is symbolic since it does not take into account the
   * size of the Image along each dimension. To be used in practice, these
   * strides must correspond to the number of intensity values to skip
   * between adjacent voxels along the respective axis. For the example
   * above, the image might consists of 128 sagittal slices, each with
   * dimensions 256x256. The dimensions of the image (as returned by size())
   * are therefore [ 128 256 256 ]. The actual strides needed to navigate
   * through the image, given the symbolic strides above, should therefore
   * be [ 65536 -256 -1 ] (since 256x256 = 65532).
   *
   * Note that a stride of zero is treated as undefined or invalid. This can
   * be used in the symbolic representation to specify that the ordering of
   * the corresponding axis is not important. A suitable stride will be
   * allocated to that axis when the image is initialised (this is done
   * with a call to sanitise()).
   *
   * The functions defined in this namespace provide an interface to
   * manipulate the strides and convert symbolic into actual strides. */
  namespace Stride
  {

    using List = std::vector<ssize_t>;

    extern const App::OptionGroup Options;




    //! \cond skip
    namespace
    {
      template <class HeaderType> 
        class Compare
        {
          public:
            Compare (const HeaderType& header) : S (header) { }
            bool operator() (const size_t a, const size_t b) const {
              if (S.stride(a) == 0)
                return false;
              if (S.stride(b) == 0)
                return true;
              return std::abs (S.stride (a)) < std::abs (S.stride (b));
            }
          private:
            const HeaderType& S;
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

      template <class HeaderType> 
        class InfoWrapper : public Wrapper
      {
        public:
          InfoWrapper (List& strides, const HeaderType& header) : Wrapper (strides), D (header) {
            assert (ndim() == D.ndim());
          }
          ssize_t size (size_t axis) const {
            return D.size (axis);
          }
        private:
          const HeaderType& D;
      };
    }
    //! \endcond




    //! return the strides of \a header as a std::vector<ssize_t>
    template <class HeaderType> 
      List get (const HeaderType& header)
      {
        List ret (header.ndim());
        for (size_t i = 0; i < header.ndim(); ++i)
          ret[i] = header.stride (i);
        return ret;
      }

    //! set the strides of \a header from a std::vector<ssize_t>
    template <class HeaderType>
      void set (HeaderType& header, const List& stride)
      {
        size_t n = 0;
        for (; n < std::min<size_t> (header.ndim(), stride.size()); ++n)
          header.stride (n) = stride[n];
        for (; n < stride.size(); ++n)
          header.stride (n) = 0;
      }

    //! set the strides of \a header from another HeaderType
    template <class HeaderType, class FromHeaderType>
      void set (HeaderType& header, const FromHeaderType& from)
      {
        set (header, get(from));
      }




    //! sort range of axes with respect to their absolute stride.
    /*! \return a vector of indices of the axes in order of increasing
     * absolute stride.
     * \note all strides should be valid (i.e. non-zero). */
    template <class HeaderType> 
      std::vector<size_t> order (const HeaderType& header, size_t from_axis = 0, size_t to_axis = std::numeric_limits<size_t>::max())
      {
        to_axis = std::min<size_t> (to_axis, header.ndim());
        assert (to_axis > from_axis);
        std::vector<size_t> ret (to_axis-from_axis);
        for (size_t i = 0; i < ret.size(); ++i) 
          ret[i] = from_axis+i;
        Compare<HeaderType> compare (header);
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
    /*! sanitise the strides of HeaderType \a header by identifying invalid (i.e.
     * zero) or duplicate (absolute) strides, and assigning to each a
     * suitable value. The value chosen for each sanitised stride is the
     * lowest number greater than any of the currently valid strides. */
    template <class HeaderType> 
      void sanitise (HeaderType& header)
      {
        // remove duplicates
        for (size_t i = 0; i < header.ndim()-1; ++i) {
          if (header.size (i) == 1) header.stride (i) = 0;
          if (!header.stride (i)) continue;
          for (size_t j = i+1; j < header.ndim(); ++j) {
            if (!header.stride (j)) continue;
            if (std::abs (header.stride (i)) == std::abs (header.stride (j))) header.stride (j) = 0;
          }
        }

        size_t max = 0;
        for (size_t i = 0; i < header.ndim(); ++i)
          if (size_t (std::abs (header.stride (i))) > max)
            max = std::abs (header.stride (i));

        for (size_t i = 0; i < header.ndim(); ++i) {
          if (header.stride (i)) continue;
          if (header.size (i) > 1) header.stride (i) = ++max;
        }
      }



    //! remove duplicate and invalid strides.
    /*! sanitise the strides of HeaderType \a header by identifying invalid (i.e.
     * zero) or duplicate (absolute) strides, and assigning to each a
     * suitable value. The value chosen for each sanitised stride is the
     * lowest number greater than any of the currently valid strides. */
    template <class HeaderType> 
      inline void sanitise (List& strides, const HeaderType& header)
      {
        InfoWrapper<HeaderType> wrapper (strides, header);
        sanitise (wrapper);
      }


    //! remove duplicate and invalid strides.
    /*! sanitise the strides in \a current by identifying invalid (i.e.
     * zero) or duplicate (absolute) strides, and assigning to each a
     * suitable value. The value chosen for each sanitised stride is the
     * lowest number greater than any of the currently valid strides. */
    List& sanitise (List& current, const List& desired, const std::vector<ssize_t>& header);


    //! convert strides from symbolic to actual strides
    template <class HeaderType> 
      void actualise (HeaderType& header)
      {
        sanitise (header);
        std::vector<size_t> x (order (header));
        ssize_t skip = 1;
        for (size_t i = 0; i < header.ndim(); ++i) {
          header.stride (x[i]) = header.stride (x[i]) < 0 ? -skip : skip;
          skip *= header.size (x[i]);
        }
      }
    //! convert strides from symbolic to actual strides
    /*! convert strides from symbolic to actual strides, assuming the strides
     * in \a strides and HeaderType dimensions of \a header. */
    template <class HeaderType> 
      inline void actualise (List& strides, const HeaderType& header)
      {
        InfoWrapper<HeaderType> wrapper (strides, header);
        actualise (wrapper);
      }

    //! get actual strides:
    template <class HeaderType> 
      inline List get_actual (HeaderType& header)
      {
        List strides (get (header));
        actualise (strides, header);
        return strides;
      }

    //! get actual strides:
    template <class HeaderType> 
      inline List get_actual (const List& strides, const HeaderType& header)
      {
        List out (strides);
        actualise (out, header);
        return out;
      }



    //! convert strides from actual to symbolic strides
    template <class HeaderType> 
      void symbolise (HeaderType& header)
      {
        std::vector<size_t> p (order (header));
        for (ssize_t i = 0; i < ssize_t (p.size()); ++i)
          if (header.stride (p[i]) != 0)
            header.stride (p[i]) = header.stride (p[i]) < 0 ? -(i+1) : i+1;
      }
    //! convert strides from actual to symbolic strides
    template <> 
      inline void symbolise (List& strides)
      {
        Wrapper wrapper (strides);
        symbolise (wrapper);
      }

    //! get symbolic strides:
    template <class HeaderType> 
      inline List get_symbolic (const HeaderType& header)
      {
        List strides (get (header));
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
    template <class HeaderType> 
      size_t offset (const HeaderType& header)
      {
        size_t offset = 0;
        for (size_t i = 0; i < header.ndim(); ++i)
          if (header.stride (i) < 0)
            offset += size_t (-header.stride (i)) * (header.size (i) - 1);
        return offset;
      }

    //! calculate offset to start of data
    /*! this function caculate the offset (in number of voxels) from the start of the data region
     * to the first voxel value (i.e. at voxel [ 0 0 0 ... ]), assuming the
     * strides in \a strides and HeaderType dimensions of \a header. */
    template <class HeaderType> 
      size_t offset (List& strides, const HeaderType& header)
      {
        InfoWrapper<HeaderType> wrapper (strides, header);
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
    template <class HeaderType> 
      List get_nearest_match (const HeaderType& current, const List& desired)
      {
        List in (get_symbolic (current)), out (desired);
        out.resize (in.size(), 0);

        std::vector<ssize_t> dims (current.ndim());
        for (size_t n = 0; n < dims.size(); ++n)
          dims[n] = current.size(n);

        for (size_t i = 0; i < out.size(); ++i) 
          if (out[i]) 
            if (std::abs (out[i]) != std::abs (in[i])) 
              return sanitise (in, out, dims);

        sanitise (in, current);
        return in;
      }




    //! convenience function to get volume-contiguous strides
    inline List contiguous_along_axis (size_t axis) 
    {
      List strides (axis+1,0);
      strides[axis] = 1;
      return strides;
    }

    //! convenience function to get volume-contiguous strides
    template <class HeaderType> 
      inline List contiguous_along_axis (size_t axis, const HeaderType& header) 
      {
        return get_nearest_match (header, contiguous_along_axis (axis));
      }

    //! convenience function to get spatially contiguous strides
    template <class HeaderType>
      inline List contiguous_along_spatial_axes (const HeaderType& header)
      {
        List strides = get (header);
        for (size_t n = 3; n < strides.size(); ++n)
          strides[n] = 0;
        return strides;
      }



    List __from_command_line (const List& current);


    template <class HeaderType> 
      inline void set_from_command_line (HeaderType& header, const List& default_strides = List())
      {
        auto cmdline_strides = __from_command_line (get (header));
        if (cmdline_strides.size())
          set (header, cmdline_strides);
        else if (default_strides.size()) 
          set (header, default_strides);
      }



  }
}

#endif

