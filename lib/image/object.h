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

#ifndef __image_object_h__
#define __image_object_h__

#include "ptr.h"
#include "image/header.h"
#include "image/mapper.h"
#include "image/format/base.h"
#include "math/complex.h"

namespace MR {
  namespace Dialog { class File; }
  namespace Image {

    //! \addtogroup Image 
    // @{
    
    class Object {
      public:
        Object () { } //: start (0) { memset (stride, 0, MRTRIX_MAX_NDIMS*sizeof(ssize_t)); }
        ~Object ()
        { 
          info ("closing image \"" + H.name + "\"...");
          M.unmap (H); 
        }

        const Header&        header () const         { return (H); }

        void                 open (const std::string& imagename, bool is_read_only = true);
        void                 create (const std::string& imagename, Header &template_header);
        void                 concatenate (std::vector<RefPtr<Object> >& images);

        void                 map ()                  { if (!is_mapped()) M.map (H); }
        void                 unmap ()                { if (is_mapped()) M.unmap (H); }
        bool                 is_mapped () const      { return (M.is_mapped()); }

        int                  dim (size_t index) const { return (H.dim (index)); } 
        size_t               ndim () const            { return (H.ndim()); }
        float                vox (size_t index) const { return (H.vox (index)); }
        const std::string&   name () const           { return (H.name); }
        bool                 is_complex () const     { return (H.data_type.is_complex()); }

        const std::vector<std::string>&  comments () const   { return (H.comments); }
        const Math::Matrix<float>&       transform () const  { return (H.transform()); }
        const Math::Matrix<float>&       DW_scheme () const  { return (H.DW_scheme); }

        DataType             data_type () const       { return (H.data_type); }
        float                offset () const          { return (H.offset); }
        float                scale () const           { return (H.scale); }
        
        bool                 output_name () const     { return (M.output_name.size()); }
        void                 no_output_name ()        { M.output_name.clear(); }

        void set_temporary (bool yesno = true) {
          M.temporary = yesno; 
          if (M.temporary) {
            for (size_t n = 0; n < M.list.size(); n++) 
              M.list[n].fmap.mark_for_deletion(); 
          }
        }

        bool                 read_only () const   { return (H.read_only); }
        void                 set_read_only (bool read_only) { M.set_read_only (read_only); H.read_only = read_only; }
        const char*          format () const      { return (H.format); }
        const Axes&          axes () const;
        std::string          description () const { return (H.description()); }
        void                 apply_scaling (float scale, float bias = 0.0) { H.scale *= scale; H.offset = scale * H.offset + bias; }

        friend std::ostream& operator<< (std::ostream& stream, const Object& obj);

      protected:
        static const Format::Base* handlers[];

        Header  H;
        Mapper  M;
        size_t  start;
        ssize_t stride[MRTRIX_MAX_NDIMS];

        void                 setup ();

        float32              scale_from_storage (float32 val) const { return (H.offset + H.scale * val); }
        float32              scale_to_storage (float32 val) const   { return ((val - H.offset) / H.scale); }

        float                real (size_t offset) const                { return (scale_from_storage (M.real (offset))); }
        void                 real (size_t offset, float val)           { M.real (scale_to_storage (val), offset); }

        float                imag (size_t offset) const                { return (scale_from_storage (M.imag (offset))); }
        void                 imag (size_t offset, float val)           { M.imag (scale_to_storage (val), offset); }

        friend class Dialog::File;
        friend class Position;
        friend class Voxel;
        friend class Header;
    };

    //! @}

    inline Header::Header (const Object& H) { *this = H.H; }

  }
}

#endif

