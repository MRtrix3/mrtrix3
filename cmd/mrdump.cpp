/* Copyright (c) 2008-2023 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#include "command.h"
#include "datatype.h"
#include "image.h"
#include "transform.h"
#include "adapter/replicate.h"
#include "algo/loop.h"
#include "file/ofstream.h"

using namespace MR;
using namespace App;


void usage ()
{
  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Print out the values within an image";

  DESCRIPTION
  + "If no destination file is specified, the voxel locations will be "
    "printed to stdout.";

  ARGUMENTS
  + Argument ("input",  "the input image.").type_image_in()
  + Argument ("output", "the (optional) output text file.").type_file_out().optional();

  OPTIONS
  + Option ("mask", "only write the image values within voxels specified by a mask image")
    + Argument ("image").type_image_in()

  + Option ("indices", "output voxel indices as first 3 columns of output.")

  + Option ("positions", "output voxel real-space position as first 3 columns of output "
      "(after voxel indices if also requested)");
}



template <typename T, class StreamType>
void write (Image<T>&& image, Image<bool>& mask, StreamType& out, bool write_idx, bool write_pos)
{
  auto M = Transform(image).voxel2scanner;

  for (auto l = Loop(0,3) (image); l; ++l) {
    if (mask.valid()) {
      assign_pos_of (image, 0, 3).to (mask);
      if (!mask.value())
        continue;
    }

    if (write_idx)
      out << image.index(0) << " " << image.index(1) << " " << image.index(2) << " ";

    if (write_pos) {
      Eigen::Vector3d p = M * Eigen::Vector3d ({ double(image.index(0)), double(image.index(1)), double(image.index(2)) });
      out << p[0] << " " << p[1] << " " << p[2] << " ";
    }


    if (image.ndim() > 3) {
      for (auto il = Loop (3,image.ndim()) (image); il; ++il)
        out << image.value() << " ";
      out << "\n";
    }
    else
      out << image.value() << "\n";
  }
}


template <class StreamType>
void write (Header& header, Image<bool>& mask, StreamType& out, bool write_idx, bool write_pos)
{
  switch (uint8_t(DataType(header.datatype())()) & ~(DataType::BigEndian | DataType::LittleEndian | DataType::Complex)) {
    case DataType::Bit: case DataType::UInt8: case DataType::UInt16: case DataType::UInt32:
      write (header.get_image<uint32_t>(), mask, out, write_idx, write_pos);
      break;
    case DataType::Int8: case DataType::Int16: case DataType::Int32:
      write (header.get_image<int32_t>(), mask, out, write_idx, write_pos);
      break;
    case DataType::UInt64: write (header.get_image<uint64_t>(), mask, out, write_idx, write_pos); break;
    case DataType::Int64:  write (header.get_image<int64_t>(), mask, out, write_idx, write_pos); break;
    case DataType::Float32:
      if (header.datatype().is_complex())
        write (header.get_image<cfloat>(), mask, out, write_idx, write_pos);
      else
        write (header.get_image<float>(), mask, out, write_idx, write_pos);
      break;
    case DataType::Float64:
      if (header.datatype().is_complex())
        write (header.get_image<cdouble>(), mask, out, write_idx, write_pos);
      else
        write (header.get_image<double>(), mask, out, write_idx, write_pos);
      break;
    default:
      throw Exception ("Unknown data type: " + std::string(header.datatype().description()) + " (" + str(uint32_t(uint8_t(DataType(header.datatype())()))) + ")");
  }
}



void run ()
{
  auto H = Header::open (argument[0]);

  Image<bool> mask;
  auto opt = get_options ("mask");
  if (opt.size())
    mask = Image<bool>::open (opt[0][0]);

  bool write_idx = get_options("indices").size();
  bool write_pos = get_options("positions").size();

  if (argument.size() == 2) {
    File::OFStream out (argument[1]);
    write (H, mask, out, write_idx, write_pos);
  } else {
    write (H, mask, std::cout, write_idx, write_pos);
  }
}
