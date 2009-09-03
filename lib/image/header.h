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
#include "image/handler/base.h"
#include "file/mmap.h"
#include "math/matrix.h"

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
          std::map<std::string, std::string> (H),
          format (NULL), axes (H.axes), offset (0.0), scale (1.0), 
          readwrite (false), DW_scheme (H.DW_scheme), comments (H.comments),
          dtype (H.dtype), transform_matrix (H.transform_matrix) { } 

        template <class DataSet> Header (const DataSet& ds) :
          format (NULL), offset (0.0), scale (1.0), readwrite (false), 
          transform_matrix (ds.transform()) { 
            axes.ndim() = ds.ndim();
            //axes.resize (ds.ndim());
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

        const std::string&         name () const { return (identifier); }
        std::string&               name ()       { return (identifier); }
        const char*                format;
        Axes                       axes;
        float                      offset, scale;
        bool                       readwrite;
        Math::Matrix<float>        DW_scheme;
        std::vector<File::Entry>   files;

        std::vector<std::string>   comments;

        Ptr<Handler::Base>         handler;

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

        void  apply_scaling (float scaling, float bias = 0.0) { scale *= scaling; offset = scaling * offset + bias; }

        static inline float scale_from_storage (float val, float scale_f, float offset_f) { return (offset_f + scale_f * val); }
        static inline float scale_to_storage (float val, float scale_f, float offset_f) { return ((val - offset_f) / scale_f); }

        float scale_from_storage (float val) const { return (scale_from_storage (val, scale, offset)); }
        float scale_to_storage (float val) const   { return (scale_to_storage (val, scale, offset)); }


        static const Header open (const std::string& image_name, bool read_write = false);
        static const Header create (const std::string& image_name, const Header& template_header);

        friend std::ostream& operator<< (std::ostream& stream, const Header& H);

      protected:
        std::string          identifier;
        DataType             dtype;
        Math::Matrix<float>  transform_matrix;

        void merge (const Header& H);
    };




    //! @}

  }
}

#endif

