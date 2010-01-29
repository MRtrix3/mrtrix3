/*
   Copyright 2008 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 27/06/08.

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

#ifndef __image_header_h__
#define __image_header_h__

#include <map>

#include "ptr.h"
#include "data_type.h"
#include "image/axis.h"
#include "dataset/misc.h"
#include "image/handler/base.h"
#include "file/mmap.h"
#include "math/matrix.h"

 /*! \defgroup Image Image access
  * \brief Classes and functions providing access to image data. */

namespace MR {
  namespace Image {

    //! \addtogroup Image 
    // @{
    
    /*! A container for all the information related to an image
     * This class contains all the information available about an image as it
     * is (or will be) stored on disk. It does not provide access to the voxel
     * data themselves. For this, please use the Image::Voxel class. 
     *
     * The Header class provides a limited implementation of the DataSet
     * interface. As such, it can already be used with a number of the relevant
     * algorithms. */
    class Header : public std::map<std::string, std::string> {
      public:
        Header () : format (NULL), offset (0.0), scale (1.0), readwrite (false) { }
        Header (const Header& H) :
          std::map<std::string, std::string> (H), dtype (H.dtype), 
          transform_matrix (H.transform_matrix), format (NULL),
          axes (H.axes), offset (0.0), scale (1.0), readwrite (false), 
          DW_scheme (H.DW_scheme), comments (H.comments) { } 

        template <class DataSet> Header (const DataSet& ds) :
          format (NULL), offset (0.0), scale (1.0), readwrite (false), 
          transform_matrix (ds.transform()) { 
            axes.ndim() = ds.ndim();
            for (size_t i = 0; i < ds.ndim(); i++) {
              axes.dim(i) = ds.dim(i);
              axes.vox(i) = ds.vox(i);
            }
          } 

        template <class DataSet> Header& operator= (const DataSet& ds) {
          format = NULL; offset = 0.0; scale = 1.0; readwrite = false;
          transform_matrix = ds.transform(); 
          axes.ndim() = ds.ndim();
          for (size_t i = 0; i < ds.ndim(); i++) {
            axes.dim(i) = ds.dim(i);
            axes.vox(i) = ds.vox(i);
          }
          return (*this);
        } 

        bool operator! () const { return (handler); }

        const std::string&         name () const { return (identifier); }
        std::string&               name ()       { return (identifier); }

        // DataSet interface:
        int     dim (size_t index) const { return (axes.dim (index)); } 
        size_t  ndim () const            { return (axes.ndim()); }
        float   vox (size_t index) const { return (axes.vox (index)); }

        const DataType& datatype () const   { return (dtype); }
        DataType&       datatype ()         { return (dtype); }
        bool            is_complex () const { return (dtype.is_complex()); }

        const Math::Matrix<float>& transform () const { return (transform_matrix); }
        Math::Matrix<float>&       transform ()       { return (transform_matrix); }

        void clear () {
          std::map<std::string, std::string>::clear(); 
          identifier.clear(); axes.clear(); comments.clear(); dtype = DataType();
          offset = 0.0; scale = 1.0; readwrite = false; format = NULL;
          transform_matrix.clear(); DW_scheme.clear();
        }

        std::string  description () const;
        void         sanitise ();

        void  set_scaling (float scaling = 1.0, float bias = 0.0) { offset = bias; scale = scaling; }
        void  reset_scaling () { set_scaling(); }
        void  apply_scaling (float scaling, float bias = 0.0) { scale *= scaling; offset = scaling * offset + bias; }

        template <typename T> static inline T scale_from_storage (T val, float scale_f, float offset_f) { return (offset_f + scale_f * val); }
        template <typename T> static inline T scale_to_storage (T val, float scale_f, float offset_f) { return ((val - offset_f) / scale_f); }

        template <typename T> float scale_from_storage (T val) const { return (scale_from_storage (val, scale, offset)); }
        template <typename T> float scale_to_storage (T val) const   { return (scale_to_storage (val, scale, offset)); }

        //! returns the memory footprint of the Image
        off64_t footprint (size_t up_to_dim = SIZE_MAX) { return (footprint_for_count (DataSet::voxel_count (*this, up_to_dim))); }

        //! returns the memory footprint of a DataSet
        off64_t footprint (const char* specifier) { return (footprint_for_count (DataSet::voxel_count (*this, specifier))); }

        static const Header open (const std::string& image_name, bool read_write = false);
        static const Header create (const std::string& image_name, const Header& template_header);

        friend std::ostream& operator<< (std::ostream& stream, const Header& H);

      private:
        std::string          identifier;
        DataType             dtype;
        Math::Matrix<float>  transform_matrix;
        mutable Array<size_t>::Ptr axes_layout; 

        void merge (const Header& H);

        off64_t footprint_for_count (off64_t count) { return (dtype == DataType::Bit ? (count+7)/8 : count * dtype.bytes()); }

      public:
        const char*                format;
        Axes                       axes;
        float                      offset, scale;
        bool                       readwrite;
        Math::Matrix<float>        DW_scheme;
        std::vector<File::Entry>   files;

        std::vector<std::string>   comments;

        Ptr<Handler::Base>         handler;
    };




    //! @}

  }
}

#endif

