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

namespace MR {
  namespace Image {

    class Object;

    //! \addtogroup Image
    // @{

    //! This class provides access to the voxel intensities of an image.
    /*! This class keeps a reference to an existing Image::Header, and provides
     * access to the corresponding image intensities. It implements all the
     * features of the DataSet abstract class. */
    class Voxel {
      public:
        //! construct a Voxel object to access the data in the Image::Header \p parent
        /*! All coordinates will be initialised to zero. */
        Voxel (Header& parent);
        
        //! construct a Voxel object to access the same data as \a V
        /*! Useful for multi-threading applications. All coordinates will be initialised to
         * the same value as \a V. */
        Voxel (const Voxel& V) : H (V.header), offset (V.offset), files (V.files) {
          assert (files);
          x = new ssize_t [ndim()];
          memcpy (x, V.x, sizeof(ssize_t) * ndim());
        }
        ~Voxel () { delete [] x; }

        const Header& header (); 
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
            x[n] = V[n];
            shift += H.stride[n] * ssize_t(x[n]);
          }
          offset = H.start + shift;
          return (*this);
        }

        //! reset all coordinates to zero. 
        void reset () { offset = H.start; memset (x, 0, ndim()*sizeof(ssize_t)); }


        class Coordinate {
          public:
            operator ssize_t () const          { return (V.get (axis)); }
            ssize_t operator++ (int notused)   { return (V.inc (axis)); }
            ssize_t operator-- (int notused)   { return (V.dec (axis)); }
            ssize_t operator+= (ssize_t increment) { return (V.move (axis, increment)); }
            ssize_t operator-= (ssize_t increment) { return (V.move (axis, -increment)); }
            ssize_t operator= (ssize_t position)   { return (V.set (axis, position)); }
            ssize_t operator= (const Coordinate& C) { return (V.set (axis, C.V.get(axis))); }
          private:
            Coordinate (Voxel& parent, size_t corresponding_axis) : V (parent), axis (corresponding_axis) { }
            Voxel& V;
            size_t axis;
            friend class Voxel;
        };

        //! return the coordinate along the specified axis.
        const ssize_t   operator[] (const size_t axis) const     { return (x[axis]); }
        //! returns a reference to the coordinate along the specified axis.
        Coordinate operator[] (const size_t axis) { return (Coordinate (*this, axis)); }

        class RealValue {
          public:
            operator float () const { return (V.get_real()); }
            float operator= (const RealValue& C) { return (operator= (float(C))); }
            float operator= (float value) { V.set_real (value); return (value); }
            float operator+= (float value) { value += V.get_real(); V.set_real (value); return (value); }
            float operator-= (float value) { value = V.get_real() - value; V.set_real (value); return (value); }
            float operator*= (float value) { value *= V.get_real(); V.set_real (value); return (value); }
            float operator/= (float value) { value = V.get_real() / value; V.set_real (value); return (value); }
          private:
            RealValue (Voxel& parent) : V (parent) { }
            Voxel& V;
            friend class Voxel;
        };

        class ImagValue {
          public:
            operator float () const { return (V.get_imag()); }
            float operator= (const ImagValue& C) { return (operator= (float(C))); }
            float operator= (float value) { V.set_imag (value); return (value); }
            float operator+= (float value) { value += V.get_imag(); V.set_imag (value); return (value); }
            float operator-= (float value) { value = V.get_imag() - value; V.set_imag (value); return (value); }
            float operator*= (float value) { value *= V.get_imag(); V.set_imag (value); return (value); }
            float operator/= (float value) { value = V.get_imag() / value; V.set_imag (value); return (value); }
          private:
            ImagValue (Voxel& parent) : V (parent) { }
            Voxel& V;
            friend class Voxel;
        };

        class ComplexValue {
          public:
            operator cfloat () const { return (V.get_complex()); }
            cfloat operator= (const ComplexValue& C) { return (operator= (cfloat(C))); }
            cfloat operator= (cfloat value) { V.set_complex (value); return (value); }
            cfloat operator+= (cfloat value) { value += V.get_complex(); V.set_complex (value); return (value); }
            cfloat operator-= (cfloat value) { value = V.get_complex() - value; V.set_complex (value); return (value); }
            cfloat operator*= (cfloat value) { value *= V.get_complex(); V.set_complex (value); return (value); }
            cfloat operator/= (cfloat value) { value = V.get_complex() / value; V.set_complex (value); return (value); }
          private:
            ComplexValue (Voxel& parent) : V (parent) { }
            Voxel& V;
            friend class Voxel;
        };

        //! %get whether the image data is complex
        /*! \return true if the image data are complex */
        bool  is_complex () const      { return (header.is_complex()); }

        //! returns the value of the voxel at the current position
        float            value () const { return (get_real()); }
        //! returns a reference to the value of the voxel at the current position
        RealValue        value ()       { return (RealValue (*this)); }

        //! returns the real value of the voxel at the current position
        float            real () const { return (get_real()); }
        //! returns a reference to the real value of the voxel at the current position
        RealValue        real ()       { return (RealValue (*this)); }

        //! returns the imaginary value of the voxel at the current position
        float            imag () const { return (get_imag()); }
        //! returns a reference to the imaginary value of the voxel at the current position
        ImagValue        imag ()       { return (ImagValue (*this)); }

        //! returns the complex value stored at the current position
        /*! \note No check is performed to ensure the image is actually complex. Calling this function on real-valued data will produce undefined results */
        cfloat           Z () const    { return (get_complex()); }
        //! returns the complex value stored at the current position
        /*! \note No check is performed to ensure the image is actually complex. Calling this function on real-valued data will produce undefined results */
        ComplexValue     Z ()          { return (ComplexValue (*this)); }

        friend std::ostream& operator<< (std::ostream& stream, const Voxel& V);

      protected:
        Header& H; //!< reference to the corresponding Image::Header
        ssize_t* x; //!< the current image coordinates
        size_t   offset; //!< the offset in memory to the current voxel
        RefPtr<std::vector<RefPtr<File::MMap> > > files;

        void map ();
        bool is_mapped () const { return (files); }

        ssize_t set (size_t axis, ssize_t position)   { offset += stride[axis] * ssize_t(position - x[axis]); x[axis] = position; return (x[axis]); }
        ssize_t get (size_t axis) const               { return (x[axis]); }
        ssize_t inc (size_t axis)                     { offset += stride[axis]; x[axis]++; return (x[axis]); }
        ssize_t dec (size_t axis)                     { offset -= stride[axis]; x[axis]--; return (x[axis]); }
        ssize_t move (size_t axis, ssize_t increment) { offset += stride[axis] * ssize_t(increment); x[axis] += increment; return (x[axis]); } 

        float   get_real () const { assert (is_mapped()); return (image.real (offset)); }
        float   get_imag () const { assert (is_mapped()); return (image.imag (offset)); }
        cfloat  get_complex () const { assert (is_mapped()); return (cfloat (get_real(), get_imag())); }

        float   set_real (const float value)   { assert (is_mapped()); image.real (offset, value); return (value); }
        float   set_imag (const float value)   { assert (is_mapped()); image.imag (offset, value); return (value); }
        cfloat  set_complex (const cfloat value) { 
          assert (is_mapped());
          image.real (offset, value.real());
          image.imag (offset, value.imag());
          return (value);
        }

        friend class Entry;
        friend class Coordinate;
        friend class RealValue;
        friend class ImagValue;
        friend class ComplexValue;
    };

    //! @}











    inline std::ostream& operator<< (std::ostream& stream, const Voxel& V)
    {
      stream << "position for image \"" << V.name() << "\" = [ ";
      for (size_t n = 0; n < V.ndim(); n++) stream << V.x[n] << " ";
      stream << "]\n  current offset = " << V.offset;
      return (stream);
    }

  }
}

#endif


