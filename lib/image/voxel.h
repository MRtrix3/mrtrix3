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
        
        const Header& header () const { return (H); }
        DataType datatype () const { return (H.datatype()); }
        const Math::Matrix<float>& transform () const { return (H.transform()); }

        //! test whether the current position is within bounds.
        /*! \return true if the current position is out of bounds, false otherwise */
        bool operator! () const { 
          for (size_t n = 0; n < ndim(); n++)
            if (ax[n].x < 0 || ax[n].x >= ssize_t(dim(n))) return (true);
          return (false);
        }

        //size_t  ndim () const { return (S->H.ndim()); }
        //ssize_t dim (size_t axis) const { return (S->H.dim(axis)); }
        size_t  ndim () const { return (num_dim); }
        ssize_t dim (size_t axis) const { return (ax[axis].dim); }
        float   vox (size_t axis) const { return (ax[axis].vox); }
        const std::string& name () const { return (H.name()); }

        template <class T> const Voxel& operator= (const T& V) {
          ssize_t shift = 0;
          for (size_t n = 0; n < ndim(); n++) {
            ax[n].x = V.pos(n);
            shift += ax[n].stride * ssize_t(ax[n].x);
          }
          offset = start + shift;
          return (*this);
        }

        //! reset all coordinates to zero. 
        void reset () { offset = start; for (size_t i = 0; i < ndim(); i++) ax[i].x = 0; }

        ssize_t pos (size_t axis) const { return (ax[axis].x); }
        void    pos (size_t axis, ssize_t newpos) { offset += ax[axis].stride*(newpos-ax[axis].x); ax[axis].x = newpos; }
        void    inc (size_t axis) { offset += ax[axis].stride; ax[axis].x++; }

        float get () const { 
          ssize_t nseg (offset / segsize);
          return (H.scale_from_storage (get_func (segment[nseg], offset - nseg*segsize))); 
        }
        void set (float val) const {
          ssize_t nseg (offset / segsize);
          put_func (H.scale_to_storage (val), segment[nseg], offset - nseg*segsize); 
        }

        //! %get whether the image data are complex
        /*! \return always true, since this class can only handle real data */
        bool  is_complex () const      { return (false); }

        friend std::ostream& operator<< (std::ostream& stream, const Voxel& V) {
          stream << "position for image \"" << V.name() << "\" = [ ";
          for (size_t n = 0; n < V.ndim(); n++) stream << V.pos(n) << " ";
          stream << "]\n  current offset = " << V.offset;
          return (stream);
        }

      protected:
        const Header&   H; //!< reference to the corresponding Image::Header

        size_t   offset; //!< the offset in memory to the current voxel
        float    scale, bias;
        size_t   segsize;
        size_t   start; //!< the offset to the first (logical) voxel in the dataset
        size_t   num_dim;

        float  (*get_func) (const void* data, size_t i);
        void   (*put_func) (float val, void* data, size_t i);

        class Ax {
          public:
            ssize_t x, stride;
            size_t  dim;
            float   vox;
        };

        Ax ax[MAX_NDIM];

        uint8_t* segment[MAX_FILES_PER_IMAGE];

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


