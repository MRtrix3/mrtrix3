/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */
#include "__mrtrix_plugin.h"

#include "command.h"
#include "datatype.h"
#include "image.h"
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
    + Argument ("image").type_image_in();
}



template <typename T, class StreamType>
void write (Image<T>& image, StreamType& out)
{
  TRACE;
  for (auto l = Loop(image) (image); l; ++l)
    out << image.value() << "\n";
}

template <typename T, class StreamType>
void write (Image<T> image, Image<bool>& mask, StreamType& out)
{
  if (!mask.valid()) {
    write (image, out);
    return;
  }
  Adapter::Replicate<Image<bool>> replicate (mask, image);
  for (auto l = Loop(image) (image, replicate); l; ++l) {
    if (replicate.value())
      out << image.value() << "\n";
  }
}



template <class StreamType>
void write (Header& header, Image<bool>& mask, StreamType& out)
{
  switch (uint8_t(DataType(header.datatype())()) & ~(DataType::BigEndian | DataType::LittleEndian | DataType::Complex)) {
    case DataType::Bit: case DataType::UInt8: case DataType::UInt16: case DataType::UInt32:
      write (header.get_image<uint32_t>(), mask, out);
      break;
    case DataType::Int8: case DataType::Int16: case DataType::Int32:
      write (header.get_image<int32_t>(), mask, out);
      break;
    case DataType::UInt64: write (header.get_image<uint64_t>(), mask, out); break;
    case DataType::Int64:  write (header.get_image<int64_t>(), mask, out); break;
    case DataType::Float32:
      if (header.datatype().is_complex())
        write (header.get_image<cfloat>(), mask, out);
      else
        write (header.get_image<float>(), mask, out);
      break;
    case DataType::Float64:
      if (header.datatype().is_complex())
        write (header.get_image<cdouble>(), mask, out);
      else
        write (header.get_image<double>(), mask, out);
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

  if (argument.size() == 2) {
    File::OFStream out (argument[1]);
    write (H, mask, out);
  } else {
    write (H, mask, std::cout);
  }
}
