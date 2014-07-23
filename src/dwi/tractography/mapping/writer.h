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




template <typename Cont>
class MapWriterBase
{

  // counts needs to be floating-point to cover possibility of weighted streamlines
  typedef Image::BufferScratch<float>::voxel_type counts_voxel_type;

  public:
    MapWriterBase (Image::Header& header, const std::string& name, const vox_stat_t s = V_SUM) :
        H (header),
        output_image_name (name),
        direct_dump (false),
        voxel_statistic (s),
        counts ((s == V_MEAN) ? (new Image::BufferScratch<float>(header, "counts")) : NULL),
        v_counts (counts ? new counts_voxel_type (*counts) : NULL) { }

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


    virtual bool operator() (const Cont&) { return false; }


  protected:
    Image::Header& H;
    const std::string output_image_name;
    bool direct_dump;
    const vox_stat_t voxel_statistic;
    Ptr< Image::BufferScratch<float> > counts;
    Ptr< counts_voxel_type > v_counts;

};



template <typename Cont> float get_factor (const Cont&, const typename Cont::const_iterator) { assert (0); return 0; }
template <> float get_factor<SetVoxel>          (const SetVoxel&         , const SetVoxel         ::const_iterator);
template <> float get_factor<SetVoxelDEC>       (const SetVoxelDEC&      , const SetVoxelDEC      ::const_iterator);
//template <> float get_factor<SetVoxelFactor>    (const SetVoxelFactor&   , const SetVoxelFactor   ::const_iterator);
//template <> float get_factor<SetVoxelDECFactor> (const SetVoxelDECFactor&, const SetVoxelDECFactor::const_iterator);



template <class value_type, typename Cont>
class MapWriter : public MapWriterBase<Cont>
{

  typedef typename Image::Buffer<value_type> image_type;
  typedef typename Image::Buffer<value_type>::voxel_type image_voxel_type;
  typedef typename Image::BufferScratch<value_type> buffer_type;
  typedef typename Image::BufferScratch<value_type>::voxel_type buffer_voxel_type;

  public:
    MapWriter (Image::Header& header, const std::string& name, const vox_stat_t voxel_statistic = V_SUM) :
      MapWriterBase<Cont> (header, name, voxel_statistic),
      buffer (header, "buffer"),
      v_buffer (buffer)
    {
      Image::Loop loop;
      if (voxel_statistic == V_MIN) {
        for (loop.start (v_buffer); loop.ok(); loop.next (v_buffer))
          v_buffer.value() = std::numeric_limits<value_type>::max();
      } else if (voxel_statistic == V_MAX) {
        for (loop.start (v_buffer); loop.ok(); loop.next (v_buffer))
          v_buffer.value() = -std::numeric_limits<value_type>::max();
      } else {
        for (loop.start (v_buffer); loop.ok(); loop.next (v_buffer))
          v_buffer.value() = value_type (0);
      }
    }

    MapWriter (const MapWriter& that) :
      MapWriterBase<Cont> (that),
      buffer (MapWriterBase<Cont>::H, ""),
      v_buffer (buffer)
    {
      throw Exception ("Do not instantiate copy constructor for MapWriter");
    }

    ~MapWriter ()
    {

      Image::Loop loop;
      switch (MapWriterBase<Cont>::voxel_statistic) {

        case V_SUM: break;

        case V_MIN:
          for (loop.start (v_buffer); loop.ok(); loop.next (v_buffer)) {
            if (v_buffer.value() == std::numeric_limits<value_type>::max())
              v_buffer.value() = value_type(0);
          }
          break;

        case V_MEAN:
          for (loop.start (v_buffer, *MapWriterBase<Cont>::v_counts); loop.ok(); loop.next (v_buffer, *MapWriterBase<Cont>::v_counts)) {
            if ((*MapWriterBase<Cont>::v_counts).value())
              v_buffer.value() /= float(MapWriterBase<Cont>::v_counts->value());
          }
          break;

        case V_MAX:
          for (loop.start (v_buffer); loop.ok(); loop.next (v_buffer)) {
            if (v_buffer.value() == -std::numeric_limits<value_type>::max())
              v_buffer.value() = value_type(0);
          }
          break;

        default:
          throw Exception ("Unknown / unhandled voxel statistic in ~MapWriter()");

      }

      if (MapWriterBase<Cont>::direct_dump) {
        fprintf (stderr, "%s: writing image to file... ", App::NAME.c_str());
        buffer.dump_to_file (MapWriterBase<Cont>::output_image_name, MapWriterBase<Cont>::H);
        fprintf (stderr, "done.\n");
      } else {
        image_type out (MapWriterBase<Cont>::output_image_name, MapWriterBase<Cont>::H);
        image_voxel_type v_out (out);
        Image::LoopInOrder loopinorder (v_out, "writing image to file...", 0, 3);
        for (loopinorder.start (v_out, v_buffer); loopinorder.ok(); loopinorder.next (v_out, v_buffer))
          v_out.value() = v_buffer.value();
      }
    }


    bool operator() (const Cont& in)
    {
      for (typename Cont::const_iterator i = in.begin(); i != in.end(); ++i) {
        Image::Nav::set_pos (v_buffer, *i);
        const value_type factor = get_factor (in, i);
        switch (MapWriterBase<Cont>::voxel_statistic) {
        case V_SUM:  v_buffer.value() += in.weight * factor;           break;
        case V_MIN:  v_buffer.value() = MIN(v_buffer.value(), factor); break;
        case V_MAX:  v_buffer.value() = MAX(v_buffer.value(), factor); break;
        case V_MEAN:
          // Only increment counts[] if it is necessary to do so given the chosen statistic
          v_buffer.value() += in.weight * factor;
          Image::Nav::set_pos (*MapWriterBase<Cont>::v_counts, *i);
          (*MapWriterBase<Cont>::v_counts).value() += in.weight;
          break;
        default:
          throw Exception ("Unknown / unhandled voxel statistic in MapWriter::operator()");
        }
      }
      return true;
    }


  private:
    BufferScratchDump<value_type> buffer;
    buffer_voxel_type v_buffer;

};





template <typename Cont>
class MapWriterColour : public MapWriterBase<Cont>
{

  typedef typename Image::Buffer<float> image_type;
  typedef typename Image::Buffer<float>::voxel_type image_voxel_type;
  typedef typename Image::BufferScratch<float> buffer_type;
  typedef typename Image::BufferScratch<float>::voxel_type buffer_voxel_type;

  public:
    MapWriterColour (Image::Header& header, const std::string& name, const vox_stat_t voxel_statistic = V_SUM) :
      MapWriterBase<Cont> (header, name, voxel_statistic),
      buffer (header, "buffer"),
      v_buffer (buffer)
    {
      Image::Loop loop;
      if (voxel_statistic == V_MIN) {
        for (loop.start (v_buffer); loop.ok(); loop.next (v_buffer))
          v_buffer.value() = std::numeric_limits<float>::max();
      } else {
        for (loop.start (v_buffer); loop.ok(); loop.next (v_buffer))
          v_buffer.value() = 0.0;
      }
    }

    MapWriterColour (const MapWriterColour& that) :
      MapWriterBase<Cont> (that),
      buffer (MapWriterBase<Cont>::H, "buffer"),
      v_buffer (buffer)
      {
        throw Exception ("Do not instantiate copy constructor for MapWriterColour");
      }

    ~MapWriterColour ()
    {
      Image::Loop loop (0, 3);
      switch (MapWriterBase<Cont>::voxel_statistic) {

        case V_SUM: break;

        case V_MIN:
          for (loop.start (v_buffer); loop.ok(); loop.next (v_buffer)) {
            const Point<float> value (get_value());
            if (value == Point<float> (std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()))
              set_value (Point<float> (0.0, 0.0, 0.0));
          }
          break;

        case V_MEAN:
          for (loop.start (v_buffer); loop.ok(); loop.next (v_buffer)) {
            Point<float> value (get_value());
            if (value.norm2()) {
              value.normalise();
              set_value (value);
            }
          }
          break;

        case V_MAX:
          break;

        default:
          throw Exception ("Unknown / unhandled voxel statistic in ~MapWriterColour()");

      }

      if (MapWriterBase<Cont>::direct_dump) {
        fprintf (stderr, "%s: writing image to file... ", App::NAME.c_str());
        buffer.dump_to_file (MapWriterBase<Cont>::output_image_name, MapWriterBase<Cont>::H);
        fprintf (stderr, "done.\n");
      } else {
        image_type out (MapWriterBase<Cont>::output_image_name, MapWriterBase<Cont>::H);
        image_voxel_type v_out (out);
        Image::LoopInOrder loopinorder (v_out, "writing image to file...", 0, 3);
        for (loopinorder.start (v_out, v_buffer); loopinorder.ok(); loopinorder.next (v_out, v_buffer)) {
          Point<float> value (get_value());
          v_out[3] = 0; v_out.value() = value[0];
          v_out[3] = 1; v_out.value() = value[1];
          v_out[3] = 2; v_out.value() = value[2];
        }
      }
    }


    bool operator() (const Cont& in)
    {
      for (typename Cont::const_iterator i = in.begin(); i != in.end(); ++i) {
        Image::Nav::set_pos (v_buffer, *i);
        const float factor = get_factor (in, i);
        Point<float> scaled_colour (i->get_colour());
        scaled_colour *= factor;
        const Point<float> current_value = get_value();
        switch (MapWriterBase<Cont>::voxel_statistic) {
          case V_SUM:
            set_value (current_value + (scaled_colour * in.weight));
            break;
          case V_MIN:
            if (scaled_colour.norm2() < current_value.norm2())
              set_value (scaled_colour);
            break;
          case V_MEAN:
            set_value (current_value + (scaled_colour * in.weight));
            Image::Nav::set_pos (*MapWriterBase<Cont>::v_counts, *i);
            (*MapWriterBase<Cont>::v_counts).value() += in.weight;
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


  private:
    BufferScratchDump<float> buffer;
    buffer_voxel_type v_buffer;


    Point<float> get_value ()
    {
      Point<float> value;
      v_buffer[3] = 0; value[0] = v_buffer.value();
      v_buffer[3] = 1; value[1] = v_buffer.value();
      v_buffer[3] = 2; value[2] = v_buffer.value();
      return value;
    }

    void set_value (const Point<float>& value)
    {
      v_buffer[3] = 0; v_buffer.value() = value[0];
      v_buffer[3] = 1; v_buffer.value() = value[1];
      v_buffer[3] = 2; v_buffer.value() = value[2];
    }



};




}
}
}
}

#endif



