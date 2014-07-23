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

#ifndef __dwi_tractography_mapping_writer_h__
#define __dwi_tractography_mapping_writer_h__


#include "file/path.h"
#include "file/utils.h"
#include "image/buffer_scratch.h"
#include "image/buffer.h"
#include "image/loop.h"
#include "image/nav.h"
#include "image/header.h"
#include "thread/queue.h"

#include "dwi/tractography/mapping/buffer_scratch_dump.h"
#include "dwi/tractography/mapping/twi_stats.h"
#include "dwi/tractography/mapping/voxel.h"



#include <typeinfo>


namespace MR {
namespace DWI {
namespace Tractography {
namespace Mapping {



class MapWriterBase
{

  protected:
    // counts needs to be floating-point to cover possibility of weighted streamlines
    typedef Image::BufferScratch<float> counts_buffer_type;
    typedef Image::BufferScratch<float>::voxel_type counts_voxel_type;

  public:
    MapWriterBase (Image::Header& header, const std::string& name, const vox_stat_t s = V_SUM) :
        H (header),
        output_image_name (name),
        direct_dump (false),
        voxel_statistic (s) { }

    MapWriterBase (const MapWriterBase& that) :
        H (that.H),
        output_image_name (that.output_image_name),
        direct_dump (that.direct_dump),
        voxel_statistic (that.voxel_statistic)
    {
      throw Exception ("Do not instantiate copy constructor for MapWriterBase");
    }

    virtual ~MapWriterBase() { }


    virtual void set_direct_dump (const bool i)
    {
      direct_dump = i;
      if (i && !Path::has_suffix (output_image_name, ".mih"))
        throw Exception ("Can only perform direct dump to file for .mih image format");
    }


    virtual bool operator() (const SetVoxel&)    { return false; }
    virtual bool operator() (const SetVoxelDEC&) { return false; }
    virtual bool operator() (const SetDixel&)    { return false; }


  protected:
    Image::Header& H;
    const std::string output_image_name;
    bool direct_dump;
    const vox_stat_t voxel_statistic;

    // This gets used with mean voxel statistic for some (but not all) writers
    Ptr<counts_buffer_type> counts;
    Ptr<counts_voxel_type > v_counts;

};




// TODO It may not be necessary to have different types of map writers depending on the
//   output image dimensionality: just have an enum, and handle the data as it comes
//   in depending on type




template <typename value_type>
class MapWriterGreyscale : public MapWriterBase
{

  typedef typename Image::Buffer<value_type> image_type;
  typedef typename Image::Buffer<value_type>::voxel_type image_voxel_type;
  typedef typename Mapping::BufferScratchDump<value_type> buffer_type;
  typedef typename Mapping::BufferScratchDump<value_type>::voxel_type buffer_voxel_type;

  public:
    MapWriterGreyscale (Image::Header& header, const std::string& name, const vox_stat_t voxel_statistic = V_SUM) :
      MapWriterBase (header, name, voxel_statistic),
      buffer (header, "TWI greyscale buffer"),
      v_buffer (buffer)
    {
      Image::LoopInOrder loop (v_buffer);
      if (voxel_statistic == V_MIN) {
        for (loop.start (v_buffer); loop.ok(); loop.next (v_buffer))
          v_buffer.value() = std::numeric_limits<value_type>::max();
      } else if (voxel_statistic == V_MAX) {
        for (loop.start (v_buffer); loop.ok(); loop.next (v_buffer))
          v_buffer.value() = -std::numeric_limits<value_type>::max();
      } else {
        buffer.zero();
      }
      if (voxel_statistic == V_MEAN) {
        counts = new counts_buffer_type (header, "TWI streamline count buffer");
        counts->zero();
        v_counts = new counts_voxel_type (*counts);
      }
    }

    MapWriterGreyscale (const MapWriterGreyscale& that) :
      MapWriterBase (that),
      buffer (H, ""),
      v_buffer (buffer)
    {
      throw Exception ("Do not instantiate copy constructor for MapWriterGreyscale");
    }

    ~MapWriterGreyscale ()
    {

      Image::LoopInOrder loop_buffer (v_buffer);
      switch (voxel_statistic) {

        case V_SUM: break;

        case V_MIN:
          for (loop_buffer.start (v_buffer); loop_buffer.ok(); loop_buffer.next (v_buffer)) {
            if (v_buffer.value() == std::numeric_limits<value_type>::max())
              v_buffer.value() = value_type(0);
          }
          break;

        case V_MEAN:
          for (loop_buffer.start (v_buffer, *v_counts); loop_buffer.ok(); loop_buffer.next (v_buffer, *v_counts)) {
            if ((*v_counts).value())
              v_buffer.value() /= float(v_counts->value());
          }
          break;

        case V_MAX:
          for (loop_buffer.start (v_buffer); loop_buffer.ok(); loop_buffer.next (v_buffer)) {
            if (v_buffer.value() == -std::numeric_limits<value_type>::max())
              v_buffer.value() = value_type(0);
          }
          break;

        default:
          throw Exception ("Unknown / unhandled voxel statistic in ~MapWriter()");

      }

      if (direct_dump) {
        if (App::log_level)
          std::cerr << App::NAME << ": writing image to file... ";
        buffer.dump_to_file (output_image_name, H);
        if (App::log_level)
          std::cerr << "done.\n";
      } else {
        image_type out (output_image_name, H);
        image_voxel_type v_out (out);
        Image::LoopInOrder loop_out (v_out, "writing image to file...");
        for (loop_out.start (v_out, v_buffer); loop_out.ok(); loop_out.next (v_out, v_buffer))
          v_out.value() = v_buffer.value();
      }
    }


    bool operator() (const SetVoxel& in);


  private:
    BufferScratchDump<value_type> buffer;
    buffer_voxel_type v_buffer;


};



template <typename value_type>
bool MapWriterGreyscale<value_type>::operator() (const SetVoxel& in)
{
  for (SetVoxel::const_iterator i = in.begin(); i != in.end(); ++i) {
    Image::Nav::set_pos (v_buffer, *i);
    const float factor = in.factor;
    const float weight = in.weight * i->get_length();
    switch (voxel_statistic) {
      case V_SUM:  v_buffer.value() += weight * factor;           break;
      case V_MIN:  v_buffer.value() = MIN(v_buffer.value(), factor); break;
      case V_MAX:  v_buffer.value() = MAX(v_buffer.value(), factor); break;
      case V_MEAN:
        // Only increment counts[] if it is necessary to do so given the chosen statistic
        v_buffer.value() += weight * factor;
        Image::Nav::set_pos (*v_counts, *i);
        (*v_counts).value() += weight;
        break;
      default:
        throw Exception ("Unknown / unhandled voxel statistic in MapWriterGreyscale::operator()");
    }
  }
  return true;
}





class MapWriterDEC : public MapWriterBase
{

  typedef typename Image::Buffer<float> image_type;
  typedef typename Image::Buffer<float>::voxel_type image_voxel_type;
  typedef typename Image::BufferScratch<float> buffer_type;
  typedef typename Image::BufferScratch<float>::voxel_type buffer_voxel_type;

  public:
    MapWriterDEC (Image::Header&, const std::string&, const vox_stat_t);
    MapWriterDEC (const MapWriterDEC&);
    ~MapWriterDEC();


    bool operator() (const SetVoxelDEC& in);


  private:
    BufferScratchDump<float> buffer;
    buffer_voxel_type v_buffer;

    // Internal convenience functions
    Point<float> get_value();
    void         set_value (const Point<float>& value);

};






template <typename value_type>
class MapWriterDixel : public MapWriterBase
{

  typedef typename Image::Buffer<value_type> image_type;
  typedef typename Image::Buffer<value_type>::voxel_type image_voxel_type;
  typedef typename Mapping::BufferScratchDump<value_type> buffer_type;
  typedef typename Mapping::BufferScratchDump<value_type>::voxel_type buffer_voxel_type;

  public:
    MapWriterDixel (Image::Header& header, const std::string& name, const vox_stat_t voxel_statistic = V_SUM) :
      MapWriterBase (header, name, voxel_statistic),
      buffer (header, "TWI dixel buffer"),
      v_buffer (buffer)
    {
      Image::LoopInOrder loop (v_buffer);
      if (voxel_statistic == V_MIN) {
        for (loop.start (v_buffer); loop.ok(); loop.next (v_buffer))
          v_buffer.value() = std::numeric_limits<value_type>::max();
      } else if (voxel_statistic == V_MAX) {
        for (loop.start (v_buffer); loop.ok(); loop.next (v_buffer))
          v_buffer.value() = -std::numeric_limits<value_type>::max();
      } else {
        buffer.zero();
      }
      if (voxel_statistic == V_MEAN) {
        counts = new counts_buffer_type (header, "TWI streamline count buffer");
        counts->zero();
        v_counts = new counts_voxel_type (*counts);
      }
    }

    MapWriterDixel (const MapWriterDixel& that) :
      MapWriterBase (that),
      buffer (H, ""),
      v_buffer (buffer)
    {
      throw Exception ("Do not instantiate copy constructor for MapWriterGreyscale");
    }

    ~MapWriterDixel ()
    {

      Image::LoopInOrder loop_buffer (v_buffer);
      switch (voxel_statistic) {

        case V_SUM: break;

        case V_MIN:
          for (loop_buffer.start (v_buffer); loop_buffer.ok(); loop_buffer.next (v_buffer)) {
            if (v_buffer.value() == std::numeric_limits<value_type>::max())
              v_buffer.value() = value_type(0);
          }
          break;

        case V_MEAN:
          for (loop_buffer.start (v_buffer, *v_counts); loop_buffer.ok(); loop_buffer.next (v_buffer, *v_counts)) {
            if ((*v_counts).value())
              v_buffer.value() /= float(v_counts->value());
          }
          break;

        case V_MAX:
          for (loop_buffer.start (v_buffer); loop_buffer.ok(); loop_buffer.next (v_buffer)) {
            if (v_buffer.value() == -std::numeric_limits<value_type>::max())
              v_buffer.value() = value_type(0);
          }
          break;

        default:
          throw Exception ("Unknown / unhandled voxel statistic in ~MapWriter()");

      }

      if (direct_dump) {
        if (App::log_level)
          std::cerr << App::NAME << ": writing image to file... ";
        buffer.dump_to_file (output_image_name, H);
        if (App::log_level)
          std::cerr << "done.\n";
      } else {
        image_type out (output_image_name, H);
        image_voxel_type v_out (out);
        Image::LoopInOrder loop_out (v_out, "writing image to file...");
        for (loop_out.start (v_out, v_buffer); loop_out.ok(); loop_out.next (v_out, v_buffer))
          v_out.value() = v_buffer.value();
      }
    }


    bool operator() (const SetDixel& in);


  private:
    BufferScratchDump<value_type> buffer;
    buffer_voxel_type v_buffer;


};



template <typename value_type>
bool MapWriterDixel<value_type>::operator() (const SetDixel& in)
{
  for (SetDixel::const_iterator i = in.begin(); i != in.end(); ++i) {
    Image::Nav::set_pos (v_buffer, *i, 0, 3);
    v_buffer[3] = i->get_dir();
    const float factor = in.factor;
    const float weight = in.weight * i->get_length();
    switch (voxel_statistic) {
      case V_SUM:  v_buffer.value() += weight * factor;              break;
      case V_MIN:  v_buffer.value() = MIN(v_buffer.value(), factor); break;
      case V_MAX:  v_buffer.value() = MAX(v_buffer.value(), factor); break;
      case V_MEAN:
        // Only increment counts[] if it is necessary to do so given the chosen statistic
        v_buffer.value() += weight * factor;
        Image::Nav::set_pos (*v_counts, *i, 0, 3);
        (*v_counts)[3] = i->get_dir();
        (*v_counts).value() += weight;
        break;
      default:
        throw Exception ("Unknown / unhandled voxel statistic in MapWriterDixel::operator()");
    }
  }
  return true;
}






}
}
}
}

#endif



