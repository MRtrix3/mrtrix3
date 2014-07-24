/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 2011.

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

#ifndef __dwi_tractography_mapping_buffer_scratch_dump_h__
#define __dwi_tractography_mapping_buffer_scratch_dump_h__


#include <fstream>
#include <map>
#include <vector>

#include "image/buffer_scratch.h"
#include "image/header.h"
#include "image/stride.h"


namespace MR {
namespace DWI {
namespace Tractography {
namespace Mapping {



template <typename value_type>
class BufferScratchDump : public Image::BufferScratch<value_type>
{

  public:
    template <class Template>
    BufferScratchDump (const Template& info) :
        Image::BufferScratch<value_type> (info) { }

    template <class Template>
    BufferScratchDump (const Template& info, const std::string& label) :
        Image::BufferScratch<value_type> (info, label) { }

    void dump_to_file (const std::string&, const Image::Header&) const;

};



template <typename value_type>
void BufferScratchDump<value_type>::dump_to_file (const std::string& path, const Image::Header& H) const
{

  if (!Path::has_suffix (path, ".mih"))
    throw Exception ("Can only perform direct dump to file for .mih files");

  // TODO Should be possible to do this also for .mif files
  const std::string dat_path = Path::basename (path.substr (0, path.size()-4) + ".dat");
  const int64_t dat_size = Image::footprint (*this);

  std::ofstream out_mih (path.c_str(), std::ios::out | std::ios::binary);
  if (!out_mih)
    throw Exception ("error creating file \"" + H.name() + "\":" + strerror (errno));

  out_mih << "mrtrix image\n";
  out_mih << "dim: " << H.dim (0);
  for (size_t n = 1; n < H.ndim(); ++n)
    out_mih << "," << H.dim (n);

  out_mih << "\nvox: " << H.vox (0);
  for (size_t n = 1; n < H.ndim(); ++n)
    out_mih << "," << H.vox (n);

  Image::Stride::List stride = Image::Stride::get (H);
  Image::Stride::symbolise (stride);

  out_mih << "\nlayout: " << (stride[0] >0 ? "+" : "-") << abs (stride[0])-1;
  for (size_t n = 1; n < H.ndim(); ++n)
    out_mih << "," << (stride[n] >0 ? "+" : "-") << abs (stride[n])-1;

  out_mih << "\ndatatype: " << H.datatype().specifier();

  for (std::map<std::string, std::string>::const_iterator i = H.begin(); i != H.end(); ++i)
    out_mih << "\n" << i->first << ": " << i->second;

  for (std::vector<std::string>::const_iterator i = H.comments().begin(); i != H.comments().end(); i++)
    out_mih << "\ncomments: " << *i;


  if (H.transform().is_set()) {
    out_mih << "\ntransform: " << H.transform() (0,0) << "," <<  H.transform() (0,1) << "," << H.transform() (0,2) << "," << H.transform() (0,3);
    out_mih << "\ntransform: " << H.transform() (1,0) << "," <<  H.transform() (1,1) << "," << H.transform() (1,2) << "," << H.transform() (1,3);
    out_mih << "\ntransform: " << H.transform() (2,0) << "," <<  H.transform() (2,1) << "," << H.transform() (2,2) << "," << H.transform() (2,3);
  }

  if (H.intensity_offset() != 0.0 || H.intensity_scale() != 1.0)
    out_mih << "\nscaling: " << H.intensity_offset() << "," << H.intensity_scale();

  if (H.DW_scheme().is_set()) {
    for (size_t i = 0; i < H.DW_scheme().rows(); i++)
      out_mih << "\ndw_scheme: " << H.DW_scheme() (i,0) << "," << H.DW_scheme() (i,1) << "," << H.DW_scheme() (i,2) << "," << H.DW_scheme() (i,3);
  }

  out_mih << "\nfile: " << dat_path << "\n";
  out_mih.close();

  std::ofstream out_dat;
  out_dat.open (dat_path.c_str(), std::ios_base::out | std::ios_base::binary);
  const value_type* data_ptr = Image::BufferScratch<value_type>::data_;
  out_dat.write (reinterpret_cast<const char*>(data_ptr), dat_size);
  out_dat.close();

  // If dat_size exceeds some threshold, ostream artificially increases the file size beyond that required at close()
  File::resize (dat_path, dat_size);

}




}
}
}
}

#endif



