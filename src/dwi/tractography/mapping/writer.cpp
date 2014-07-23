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


#include "dwi/tractography/mapping/writer.h"


namespace MR {
namespace DWI {
namespace Tractography {
namespace Mapping {




MapWriterDEC::MapWriterDEC (Image::Header& header, const std::string& name, const vox_stat_t voxel_statistic = V_SUM) :
    MapWriterBase (header, name, voxel_statistic),
    buffer (header, "TWI DEC buffer"),
    v_buffer (buffer)
{
  if (voxel_statistic == V_MIN) {
    Image::LoopInOrder loop (v_buffer);
    for (loop.start (v_buffer); loop.ok(); loop.next (v_buffer))
      v_buffer.value() = std::numeric_limits<float>::max();
  } else {
    buffer.zero();
  }
  if (voxel_statistic == V_MEAN) {
    Image::Header H_counts (header);
    H_counts.set_ndim (3);
    counts = new counts_buffer_type (H_counts, "TWI streamline count buffer");
    counts->zero();
    v_counts = new counts_voxel_type (*counts);
  }
}



MapWriterDEC::MapWriterDEC (const MapWriterDEC& that) :
    MapWriterBase (that),
    buffer (H, ""),
    v_buffer (buffer)
{
  throw Exception ("Do not instantiate copy constructor for MapWriterDEC");
}



MapWriterDEC::~MapWriterDEC ()
{
  Image::LoopInOrder loop_buffer (v_buffer, 0, 3);
  switch (voxel_statistic) {

    case V_SUM: break;

    case V_MIN:
      for (loop_buffer.start (v_buffer); loop_buffer.ok(); loop_buffer.next (v_buffer)) {
        const Point<float> value (get_value());
        if (value == Point<float> (std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()))
          set_value (Point<float> (0.0, 0.0, 0.0));
      }
      break;

    case V_MEAN:
      for (loop_buffer.start (v_buffer, *v_counts); loop_buffer.ok(); loop_buffer.next (v_buffer, *v_counts)) {
        if (v_counts->value()) {
          Point<float> value (get_value());
          value *= (1.0 / v_counts->value());
          set_value (value);
        }
      }
      break;

    case V_MAX:
      break;

    default:
      throw Exception ("Unknown / unhandled voxel statistic in ~MapWriterColour()");

  }

  if (direct_dump) {
    if (App::log_level)
      std::cerr << App::NAME.c_str() << ": writing image to file... ";
    buffer.dump_to_file (output_image_name, H);
    if (App::log_level)
      std::cerr << "done.\n";
  } else {
    image_type out (output_image_name, H);
    image_voxel_type v_out (out);
    Image::LoopInOrder loop_out (v_out, "writing image to file...", 0, 3);
    for (loop_out.start (v_out, v_buffer); loop_out.ok(); loop_out.next (v_out, v_buffer)) {
      Point<float> value (get_value());
      v_out[3] = 0; v_out.value() = value[0];
      v_out[3] = 1; v_out.value() = value[1];
      v_out[3] = 2; v_out.value() = value[2];
    }
  }
}




bool MapWriterDEC::operator() (const SetVoxelDEC& in)
{
  for (SetVoxelDEC::const_iterator i = in.begin(); i != in.end(); ++i) {
    Image::Nav::set_pos (v_buffer, *i);
    const float factor = in.factor;
    const float weight = in.weight * i->get_length();
    Point<float> scaled_colour (i->get_colour());
    scaled_colour *= factor;
    const Point<float> current_value = get_value();
    switch (voxel_statistic) {
      case V_SUM:
        set_value (current_value + (scaled_colour * weight));
        break;
      case V_MIN:
        if (scaled_colour.norm2() < current_value.norm2())
          set_value (scaled_colour);
        break;
      case V_MEAN:
        set_value (current_value + (scaled_colour * weight));
        Image::Nav::set_pos (*v_counts, *i);
        (*v_counts).value() += weight;
        break;
      case V_MAX:
        if (scaled_colour.norm2() > current_value.norm2())
          set_value (scaled_colour);
        break;
      default:
        throw Exception ("Unknown / unhandled voxel statistic in MapWriterColour::operator()");
    }
  }
  return true;
}




Point<float> MapWriterDEC::get_value ()
{
  Point<float> value;
  v_buffer[3] = 0; value[0] = v_buffer.value();
  v_buffer[3] = 1; value[1] = v_buffer.value();
  v_buffer[3] = 2; value[2] = v_buffer.value();
  return value;
}

void MapWriterDEC::set_value (const Point<float>& value)
{
  v_buffer[3] = 0; v_buffer.value() = value[0];
  v_buffer[3] = 1; v_buffer.value() = value[1];
  v_buffer[3] = 2; v_buffer.value() = value[2];
}





}
}
}
}




