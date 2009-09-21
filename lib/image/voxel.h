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
     * \todo add class ComplexVoxel to implement full complex data support. */
    class Voxel {
      public:
        //! construct a Voxel object to access the data in the Image::Header \p parent
        /*! All coordinates will be initialised to zero.
         * \note only one Image::Voxel object per Image::Header can be
         * constructed using this consructor. If more Image::Voxel objects are
         * required to access the same image (e.g. for multithreading), use the
         * copy constructor to create direct copies of the first Image::Voxel.
         * For example:
         * \code
         * Image::Header header = argument[0].get_image();
         * Image::Voxel vox1 (header);
         * Image::Voxel vox2 (vox1);
         * \endcode
       */
        Voxel (const Header& parent) : H (parent), handler (*H.handler), x (ndim(), 0) {
          assert (H.handler);
          H.handler->prepare();
          offset = handler.start();
        }
        
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

        size_t  ndim () const { return (H.ndim()); }
        ssize_t dim (size_t axis) const { return (H.dim(axis)); }
        float   vox (size_t axis) const { return (H.vox(axis)); }
        const std::string& name () const { return (H.name()); }

        template <class T> const Voxel& operator= (const T& V) {
          ssize_t shift = 0;
          for (size_t n = 0; n < ndim(); n++) {
            x[n] = V.pos(n);
            shift += handler.stride(n) * x[n];
          }
          offset = handler.start() + shift;
          return (*this);
        }

        //! reset all coordinates to zero. 
        void reset () { offset = handler.start(); for (size_t i = 0; i < ndim(); i++) x[i] = 0; }

        //! %get whether the image data are complex
        /*! \return always true, since this class can only handle real data */
        bool  is_complex () const      { return (false); }

        class Coordinate {
          public:
            operator ssize_t () const { return (V.x[a]); }

            ssize_t operator= (ssize_t pos)  { V.offset += V.handler.stride(a)*(pos-V.x[a]); V.x[a] = pos; return (pos); }
            ssize_t operator+= (ssize_t inc) { V.offset += V.handler.stride(a)*inc; V.x[a] += inc; return (V.x[a]); }
            ssize_t operator-= (ssize_t inc) { V.offset -= V.handler.stride(a)*inc; V.x[a] -= inc; return (V.x[a]); }
            ssize_t operator++ ()            { V.offset += V.handler.stride(a); return (++V.x[a]); }
            ssize_t operator-- ()            { V.offset -= V.handler.stride(a); return (--V.x[a]); }
            ssize_t operator++ (int unused)  { V.offset += V.handler.stride(a); return (V.x[a]++); }
            ssize_t operator-- (int unused)  { V.offset -= V.handler.stride(a); return (V.x[a]--); }

          private:
            Coordinate (Voxel& parent, size_t axis) : V (parent), a (axis) { }
            Voxel& V;
            size_t a;

            friend class Voxel;
        };

        ssize_t          operator[] (size_t axis) const { return (x[axis]); }
        Coordinate       operator[] (size_t axis)       { return (Coordinate (*this, axis)); }


        class Value {
          public:
            operator float () const { return (V.get()); }
            float operator= (float val) { V.set (val); return (val); }
            float operator+= (float val) { val += V.get(); V.set (val); return (val); }
            float operator-= (float val) { val = V.get() - val; V.set (val); return (val); }
            float operator*= (float val) { val *= V.get(); V.set (val); return (val); }
            float operator/= (float val) { val = V.get() / val; V.set (val); return (val); }
          private:
            Value (Voxel& parent) : V (parent) { }
            Voxel& V;
            friend class Voxel;
        };

        float value () const { return (get()); }
        Value value () { return (Value (*this)); }


        friend std::ostream& operator<< (std::ostream& stream, const Voxel& V) {
          stream << "position for image \"" << V.name() << "\" = [ ";
          for (size_t n = 0; n < V.ndim(); n++) stream << V[n] << " ";
          stream << "]\n  current offset = " << V.offset;
          return (stream);
        }

      protected:
        const Header&   H; //!< reference to the corresponding Image::Header
        const Handler::Base& handler;
        size_t   offset; //!< the offset in memory to the current voxel
        std::vector<ssize_t> x;

        float get () const { 
          ssize_t nseg (offset / handler.segment_size());
          return (H.scale_from_storage (handler.get (handler.segment(nseg), offset - nseg*handler.segment_size()))); 
        }
        void set (float val) {
          ssize_t nseg (offset / handler.segment_size());
          handler.put (H.scale_to_storage (val), handler.segment(nseg), offset - nseg*handler.segment_size()); 
        }

        friend class Coordinate;
    };

    //! @}

  }
}

#endif


