/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 23/05/09.

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

#ifndef __image_voxel_h__
#define __image_voxel_h__

#include "get_set.h"
#include "image/header.h"
#include "math/complex.h"

#define MAX_FILES_PER_IMAGE 256U
#define MAX_NDIM 16

namespace MR {
  namespace Image {

    class Object;

    //! \addtogroup Image
    // @{

    //! This class provides access to the voxel intensities of an image.
    /*! This class keeps a reference to an existing Image::Header, and provides
     * access to the corresponding image intensities. It implements all the
     * features of the DataSet abstract class. 
     * \todo implement full complex data support. */
    class Voxel {
      public:
        //! construct a Voxel object to access the data in the Image::Header \p parent
        /*! All coordinates will be initialised to zero. */
        Voxel (const Header& parent);
        
        //! construct a Voxel object to access the same data as \a V
        /*! Useful for multi-threading applications. All coordinates will be initialised to
         * the same value as \a V. */
        Voxel (const Voxel& V);

        const Header& header () const { return (H); }
        DataType datatype () const { return (H.datatype()); }
        const Math::Matrix<float>& transform () const { return (H.transform()); }

        //! test whether the current position is within bounds.
        /*! \return true if the current position is out of bounds, false otherwise */
        bool operator! () const { 
          for (size_t n = 0; n < ndim(); n++)
            if (x[n] < 0 || x[n] >= ssize_t(dim(n))) return (true);
          return (false);
        }

        //size_t  ndim () const { return (S->H.ndim()); }
        //ssize_t dim (size_t axis) const { return (S->H.dim(axis)); }
        size_t  ndim () const { return (num_dim); }
        ssize_t dim (size_t axis) const { return (dims[axis]); }
        float   vox (size_t axis) const { return (H.vox(axis)); }
        const std::string& name () const { return (H.name()); }

        template <class T> const Voxel& operator= (const T& V) {
          ssize_t shift = 0;
          for (size_t n = 0; n < ndim(); n++) {
            x[n] = V[n];
            shift += stride[n] * ssize_t(x[n]);
          }
          offset = start + shift;
          return (*this);
        }

        //! reset all coordinates to zero. 
        void reset () { offset = start; memset (x, 0, ndim()*sizeof(ssize_t)); }

        ssize_t pos (size_t axis) const { return (x[axis]); }
        void    pos (size_t axis, ssize_t newpos) { offset += stride[axis]*(newpos-x[axis]); x[axis] = newpos; }
        void    inc (size_t axis) { offset += stride[axis]; x[axis]++; }

        float   get () const { 
          ssize_t nseg (offset / segsize);
          return (H.scale_from_storage (get_func (segment[nseg], offset - nseg*segsize))); 
        }
        void    set (float val) const {
          ssize_t nseg (offset / segsize);
          put_func (H.scale_to_storage (val), segment[nseg], offset - nseg*segsize); 
        }

        //! %get whether the image data are complex
        /*! \return true if the image data are complex */
        bool  is_complex () const      { return (H.is_complex()); }

        friend std::ostream& operator<< (std::ostream& stream, const Voxel& V) {
          stream << "position for image \"" << V.name() << "\" = [ ";
          for (size_t n = 0; n < V.ndim(); n++) stream << V.x[n] << " ";
          stream << "]\n  current offset = " << V.offset;
          return (stream);
        }

      protected:
        const Header&   H; //!< reference to the corresponding Image::Header
        size_t   start; //!< the offset to the first (logical) voxel in the dataset
        size_t   offset; //!< the offset in memory to the current voxel
        size_t   segsize;
        ssize_t  stride[MAX_NDIM]; //!< the offsets between adjacent voxels along each respective axis
        ssize_t  x[MAX_NDIM]; //!< the current image coordinates
        size_t   num_dim;
        size_t   dims[MAX_NDIM];
        uint8_t* segment[MAX_FILES_PER_IMAGE];

        float  (*get_func) (const void* data, size_t i);
        void   (*put_func) (float val, void* data, size_t i);

        cfloat  (*getZ_func) (const void* data, size_t i);
        void    (*putZ_func) (cfloat val, void* data, size_t i);

        template <typename T> static float __get   (const void* data, size_t i) { return (MR::get<T> (data, i)); }
        template <typename T> static float __getLE (const void* data, size_t i) { return (MR::getLE<T> (data, i)); }
        template <typename T> static float __getBE (const void* data, size_t i) { return (MR::getBE<T> (data, i)); }

        template <typename T> static void __put   (float val, void* data, size_t i) { return (MR::put<T> (val, data, i)); }
        template <typename T> static void __putLE (float val, void* data, size_t i) { return (MR::putLE<T> (val, data, i)); }
        template <typename T> static void __putBE (float val, void* data, size_t i) { return (MR::putBE<T> (val, data, i)); }
    };

    //! @}


  }
}

#endif


