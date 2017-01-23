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

#ifndef __dwi_tractography_mapping_buffer_scratch_dump_h__
#define __dwi_tractography_mapping_buffer_scratch_dump_h__


#include <map>
#include <vector>

#include "file/ofstream.h"
#include "image.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Mapping {


        template <typename value_type>
          class BufferScratchDump : public Image<value_type>
        { MEMALIGN(BufferScratchDump)

          public:
            template <class Template>
              BufferScratchDump (const Template& info) :
                Image::BufferScratch<value_type> (info) { }

            template <class Template>
              BufferScratchDump (const Template& info, const std::string& label) :
                Image::BufferScratch<value_type> (info, label) { }

            void dump_to_file (const std::string&, const Image::Header&) const;

          private:
            // Helper function to get the underlying data pointer
            inline const char* get_data_ptr() const { return reinterpret_cast<const char*> (Image::BufferScratch<value_type>::address(0)); }

        };

        template <>
          inline const char* BufferScratchDump<bool>::get_data_ptr() const
          {
            return reinterpret_cast<const char*> (Image::BufferScratch<bool>::address());
          }



        template <typename value_type>
          void BufferScratchDump<value_type>::dump_to_file (const std::string& path, const Image::Header& H) const
          {

            if (!Path::has_suffix (path, ".mih") && !Path::has_suffix (path, ".mif"))
              throw Exception ("Can only perform direct dump to file for .mih / .mif files");

            const bool single_file = Path::has_suffix (path, ".mif");

            std::string dat_path;
            if (!single_file)
              dat_path = Path::basename (path.substr (0, path.size()-4) + ".dat");
            const int64_t dat_size = Image::footprint (*this);

            File::OFStream out_header (path, std::ios::out | std::ios::binary);

            out_header << "mrtrix image\n";
            out_header << "dim: " << H.dim (0);
            for (size_t n = 1; n < H.ndim(); ++n)
              out_header << "," << H.dim (n);

            out_header << "\nvox: " << H.vox (0);
            for (size_t n = 1; n < H.ndim(); ++n)
              out_header << "," << H.vox (n);

            Image::Stride::List stride = Image::Stride::get (H);
            Image::Stride::symbolise (stride);

            out_header << "\nlayout: " << (stride[0] >0 ? "+" : "-") << std::abs (stride[0])-1;
            for (size_t n = 1; n < H.ndim(); ++n)
              out_header << "," << (stride[n] >0 ? "+" : "-") << std::abs (stride[n])-1;

            out_header << "\ndatatype: " << H.datatype().specifier();

            for (std::map<std::string, std::string>::const_iterator i = H.begin(); i != H.end(); ++i)
              out_header << "\n" << i->first << ": " << i->second;

            for (vector<std::string>::const_iterator i = H.comments().begin(); i != H.comments().end(); i++)
              out_header << "\ncomments: " << *i;


            if (H.transform().is_set()) {
              out_header << "\ntransform: " << H.transform() (0,0) << "," <<  H.transform() (0,1) << "," << H.transform() (0,2) << "," << H.transform() (0,3);
              out_header << "\ntransform: " << H.transform() (1,0) << "," <<  H.transform() (1,1) << "," << H.transform() (1,2) << "," << H.transform() (1,3);
              out_header << "\ntransform: " << H.transform() (2,0) << "," <<  H.transform() (2,1) << "," << H.transform() (2,2) << "," << H.transform() (2,3);
            }

            if (H.intensity_offset() != 0.0 || H.intensity_scale() != 1.0)
              out_header << "\nscaling: " << H.intensity_offset() << "," << H.intensity_scale();

            if (H.DW_scheme().is_set()) {
              for (size_t i = 0; i < H.DW_scheme().rows(); i++)
                out_header << "\ndw_scheme: " << H.DW_scheme() (i,0) << "," << H.DW_scheme() (i,1) << "," << H.DW_scheme() (i,2) << "," << H.DW_scheme() (i,3);
            }

            out_header << "\nfile: ";
            int64_t offset = 0;
            if (single_file) {
              offset = out_header.tellp() + int64_t(18);
              offset += ((4 - (offset % 4)) % 4);
              out_header << ". " << offset << "\nEND\n";
            } else {
              out_header << dat_path << "\n";
            }
            out_header.close();

            File::OFStream out_dat;
            if (single_file) {
              File::resize (path, offset);
              out_dat.open (path, std::ios_base::out | std::ios_base::binary | std::ios_base::app);
            } else {
              out_dat.open (dat_path, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
            }

            const char* data_ptr = get_data_ptr();
            out_dat.write (data_ptr, dat_size);
            out_dat.close();

            // If dat_size exceeds some threshold, ostream artificially increases the file size beyond that required at close()
            if (single_file)
              File::resize (path, offset + dat_size);
            else
              File::resize (dat_path, dat_size);

          }




      }
    }
  }
}

#endif



