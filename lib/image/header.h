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
#include "file/mmap.h"
#include "math/matrix.h"

namespace MR {
  namespace Image {

    //! \addtogroup Image 
    // @{
    
    class Header : public std::map<std::string, std::string> {
      public:
        Header () : 
          format (NULL), offset (0.0), scale (1.0), use_existing_files (false), 
          read_only (true), num_voxel_per_file (0), first_voxel_offset (0) { }
        Header (const Header& H) :
          std::map<std::string, std::string> (H),
          format (NULL), axes (H.axes), offset (0.0), scale (1.0), 
          use_existing_files (false), read_only (true), DW_scheme (H.DW_scheme), comments (H.comments),
          num_voxel_per_file (0), first_voxel_offset (0), 
          dtype (H.dtype), transform_matrix (H.transform_matrix) { } 

        template <class DataSet> Header (const DataSet& ds) :
          format (NULL), offset (0.0), scale (1.0), use_existing_files (false), read_only (true),
          num_voxel_per_file (0), first_voxel_offset (0), transform_matrix (ds.transform()) { 
            axes.ndim() = ds.ndim();
            //axes.resize (ds.ndim());
            for (size_t i = 0; i < ds.ndim(); i++) {
              axes.dim(i) = ds.dim(i);
              axes.vox(i) = ds.vox(i);
            }
          } 

        template <class DataSet> Header& operator= (const DataSet& ds) {
          format = NULL; offset = 0.0; scale = 1.0; use_existing_files = false; read_only = true;
          num_voxel_per_file = 0; first_voxel_offset = 0; transform_matrix = ds.transform(); 
          axes.ndim() = ds.ndim();
          //axes.resize (ds.ndim());
          for (size_t i = 0; i < ds.ndim(); i++) {
            axes.dim(i) = ds.dim(i);
            axes.vox(i) = ds.vox(i);
          }
          return (*this);
        } 

        const std::string&         name () const { return (identifer); }
        std::string&               name ()       { return (identifer); }
        const char*                format;
        Axes                       axes;
        float                      offset, scale;
        bool                       use_existing_files;
        bool                       read_only;
        Math::Matrix<float>        DW_scheme;
        std::vector<File::Entry>   files;

        std::vector<std::string>   comments;

        size_t num_voxel_per_file;
        size_t first_voxel_offset;

        // DataSet interface:
        int     dim (size_t index) const { return (axes.dim (index)); } 
        size_t  ndim () const            { return (axes.ndim()); }
        float   vox (size_t index) const { return (axes.vox (index)); }

        const DataType& datatype () const { return (dtype); }
        DataType&       datatype ()       { return (dtype); }

        const Math::Matrix<float>& transform () const { return (transform_matrix); }
        Math::Matrix<float>&       transform ()       { return (transform_matrix); }

        void clear () {
          std::map<std::string, std::string>::clear(); 
          identifer.clear(); axes.clear(); comments.clear(); dtype = DataType();
          offset = 0.0; scale = 1.0; use_existing_files = false; read_only = true; format = NULL;
          transform_matrix.clear(); DW_scheme.clear();
        }

        std::string  description () const;
        void         sanitise ();

        float scale_from_storage (float val) const { return (offset + scale * val); }
        float scale_to_storage (float val) const   { return ((val - offset) / scale); }

        friend std::ostream& operator<< (std::ostream& stream, const Header& H);

      protected:
        std::string          identifer;
        DataType             dtype;
        Math::Matrix<float>  transform_matrix;
    };

    //! @}

  }
}

#endif

