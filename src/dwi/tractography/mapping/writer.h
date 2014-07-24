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
#include "math/vector.h"
#include "thread/queue.h"

#include "dwi/tractography/mapping/buffer_scratch_dump.h"
#include "dwi/tractography/mapping/twi_stats.h"
#include "dwi/tractography/mapping/voxel.h"



#include <typeinfo>


namespace MR {
namespace DWI {
namespace Tractography {
namespace Mapping {



enum writer_dim { UNDEFINED, GREYSCALE, DEC, DIXEL, TOD };
extern const char* writer_dims[];



class MapWriterBase
{

  protected:
    // counts needs to be floating-point to cover possibility of weighted streamlines
    typedef Image::BufferScratch<float> counts_buffer_type;
    typedef Image::BufferScratch<float>::voxel_type counts_voxel_type;

  public:
    MapWriterBase (Image::Header& header, const std::string& name, const vox_stat_t s = V_SUM, const writer_dim t = GREYSCALE) :
        H (header),
        output_image_name (name),
        direct_dump (false),
        voxel_statistic (s),
        type (t)
    {
      assert (type != UNDEFINED);
    }

    MapWriterBase (const MapWriterBase& that) :
        H (that.H),
        output_image_name (that.output_image_name),
        direct_dump (that.direct_dump),
        voxel_statistic (that.voxel_statistic),
        type (that.type)
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
    virtual bool operator() (const SetVoxelTOD&) { return false; }


  protected:
    Image::Header& H;
    const std::string output_image_name;
    bool direct_dump;
    const vox_stat_t voxel_statistic;
    const writer_dim type;

    // This gets used with mean voxel statistic for some (but not all) writers
    Ptr<counts_buffer_type> counts;
    Ptr<counts_voxel_type > v_counts;

};






template <typename value_type>
class MapWriter : public MapWriterBase
{

  typedef typename Image::Buffer<value_type> image_type;
  typedef typename Image::Buffer<value_type>::voxel_type image_voxel_type;
  typedef typename Mapping::BufferScratchDump<value_type> buffer_type;
  typedef typename Mapping::BufferScratchDump<value_type>::voxel_type buffer_voxel_type;

  public:
    MapWriter (Image::Header& header, const std::string& name, const vox_stat_t voxel_statistic = V_SUM, const writer_dim type = GREYSCALE) :
        MapWriterBase (header, name, voxel_statistic, type),
        buffer (header, "TWI " + str(writer_dims[type]) + " buffer"),
        v_buffer (buffer)
    {
      Image::LoopInOrder loop (v_buffer);
      if (type == DEC || type == TOD) {

        if (voxel_statistic == V_MIN) {
          for (loop.start (v_buffer); loop.ok(); loop.next (v_buffer))
            v_buffer.value() = std::numeric_limits<value_type>::max();
        } else {
          buffer.zero();
        }

      } else { // Greyscale and dixel

        if (voxel_statistic == V_MIN) {
          for (loop.start (v_buffer); loop.ok(); loop.next (v_buffer))
            v_buffer.value() = std::numeric_limits<value_type>::max();
        } else if (voxel_statistic == V_MAX) {
          for (loop.start (v_buffer); loop.ok(); loop.next (v_buffer))
            v_buffer.value() = -std::numeric_limits<value_type>::max();
        } else {
          buffer.zero();
        }

      }

      // With TOD, hijack the counts buffer in voxel statistic min/max mode
      //   (use to store maximum / minimum factors and hence decide when to update the TOD)
      if (voxel_statistic == V_MEAN || (type == TOD && (voxel_statistic == V_MIN || voxel_statistic == V_MAX))) {
        Image::Header H_counts (header);
        if (type == DEC || type == TOD)
          H_counts.set_ndim (3);
        counts = new counts_buffer_type (H_counts, "TWI streamline count buffer");
        counts->zero();
        v_counts = new counts_voxel_type (*counts);
      }
    }

    MapWriter (const MapWriter& that) :
        MapWriterBase (that),
        buffer (H, ""),
        v_buffer (buffer)
    {
      throw Exception ("Do not instantiate copy constructor for MapWriter");
    }

    ~MapWriter ()
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
          if (type == DEC) {
            Image::LoopInOrder loop_dec (v_buffer, 0, 3);
            for (loop_dec.start (v_buffer, *v_counts); loop_dec.ok(); loop_dec.next (v_buffer, *v_counts)) {
              if (v_counts->value()) {
                Point<value_type> value (get_dec());
                value *= (1.0 / v_counts->value());
                set_dec (value);
              }
            }
          } else if (type == TOD) {
            Image::LoopInOrder loop_dec (v_buffer, 0, 3);
            for (loop_dec.start (v_buffer, *v_counts); loop_dec.ok(); loop_dec.next (v_buffer, *v_counts)) {
              if (v_counts->value()) {
                Math::Vector<float> value;
                get_tod (value);
                value *= (1.0 / v_counts->value());
                set_tod (value);
              }
            }
          } else { // Greyscale and dixel
            for (loop_buffer.start (v_buffer, *v_counts); loop_buffer.ok(); loop_buffer.next (v_buffer, *v_counts)) {
              if ((*v_counts).value())
                v_buffer.value() /= float(v_counts->value());
            }
          }
          break;

        case V_MAX:
          if (type == DEC || type == TOD)
            break;
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
        if (type == DEC) {
          Image::LoopInOrder loop_out (v_out, "writing image to file...", 0, 3);
          for (loop_out.start (v_out, v_buffer); loop_out.ok(); loop_out.next (v_out, v_buffer)) {
            Point<value_type> value (get_dec());
            v_out[3] = 0; v_out.value() = value[0];
            v_out[3] = 1; v_out.value() = value[1];
            v_out[3] = 2; v_out.value() = value[2];
          }
        } else if (type == TOD) {
          Image::LoopInOrder loop_out (v_out, "writing image to file...", 0, 3);
          for (loop_out.start (v_out, v_buffer); loop_out.ok(); loop_out.next (v_out, v_buffer)) {
            Math::Vector<float> value;
            get_tod (value);
            for (v_out[3] = 0; v_out[3] != v_out.dim(3); ++v_out[3])
              v_out.value() = value[size_t(v_out[3])];
          }
        } else { // Greyscale and Dixel
          Image::LoopInOrder loop_out (v_out, "writing image to file...");
          for (loop_out.start (v_out, v_buffer); loop_out.ok(); loop_out.next (v_out, v_buffer))
            v_out.value() = v_buffer.value();
        }

      }
    }


    bool operator() (const SetVoxel&);
    bool operator() (const SetVoxelDEC&);
    bool operator() (const SetDixel&);
    bool operator() (const SetVoxelTOD&);


  private:
    BufferScratchDump<value_type> buffer;
    buffer_voxel_type v_buffer;

    // Convenience functions for Directionally-Encoded Colour processing
    Point<value_type> get_dec ();
    void              set_dec (const Point<value_type>&);

    // Convenience functions for Track Orientation Distribution processing
    void get_tod (      Math::Vector<float>&);
    void set_tod (const Math::Vector<float>&);

};







template <typename value_type>
bool MapWriter<value_type>::operator() (const SetVoxel& in)
{
  assert (MapWriterBase::type == GREYSCALE);
  for (SetVoxel::const_iterator i = in.begin(); i != in.end(); ++i) {
    Image::Nav::set_pos (v_buffer, *i);
    const float factor = in.factor;
    const float weight = in.weight * i->get_length();
    switch (voxel_statistic) {
      case V_SUM:  v_buffer.value() += weight * factor;           break;
      case V_MIN:  v_buffer.value() = MIN(v_buffer.value(), factor); break;
      case V_MAX:  v_buffer.value() = MAX(v_buffer.value(), factor); break;
      case V_MEAN:
        v_buffer.value() += weight * factor;
        Image::Nav::set_pos (*v_counts, *i);
        (*v_counts).value() += weight;
        break;
      default:
        throw Exception ("Unknown / unhandled voxel statistic in MapWriter::operator() (SetVoxel)");
    }
  }
  return true;
}



template <typename value_type>
bool MapWriter<value_type>::operator() (const SetVoxelDEC& in)
{
  assert (type == DEC);
  for (SetVoxelDEC::const_iterator i = in.begin(); i != in.end(); ++i) {
    Image::Nav::set_pos (v_buffer, *i);
    const float factor = in.factor;
    const float weight = in.weight * i->get_length();
    Point<value_type> scaled_colour (i->get_colour());
    scaled_colour *= factor;
    const Point<value_type> current_value = get_dec();
    switch (voxel_statistic) {
      case V_SUM:
        set_dec (current_value + (scaled_colour * weight));
        break;
      case V_MIN:
        if (scaled_colour.norm2() < current_value.norm2())
          set_dec (scaled_colour);
        break;
      case V_MEAN:
        set_dec (current_value + (scaled_colour * weight));
        Image::Nav::set_pos (*v_counts, *i);
        (*v_counts).value() += weight;
        break;
      case V_MAX:
        if (scaled_colour.norm2() > current_value.norm2())
          set_dec (scaled_colour);
        break;
      default:
        throw Exception ("Unknown / unhandled voxel statistic in MapWriter::operator() (SetVoxelDEC)");
    }
  }
  return true;
}



template <typename value_type>
bool MapWriter<value_type>::operator() (const SetDixel& in)
{
  assert (type == DIXEL);
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
        v_buffer.value() += weight * factor;
        Image::Nav::set_pos (*v_counts, *i, 0, 3);
        (*v_counts)[3] = i->get_dir();
        (*v_counts).value() += weight;
        break;
      default:
        throw Exception ("Unknown / unhandled voxel statistic in MapWriter::operator() (SetDixel)");
    }
  }
  return true;
}



template <typename value_type>
bool MapWriter<value_type>::operator() (const SetVoxelTOD& in)
{
  assert (type == TOD);
  for (SetVoxelTOD::const_iterator i = in.begin(); i != in.end(); ++i) {
    Image::Nav::set_pos (v_buffer, *i, 0, 3);
    const float factor = in.factor;
    const float weight = in.weight * i->get_length();
    Math::Vector<float> sh_coefs;
    get_tod (sh_coefs);
    if (v_counts)
      Image::Nav::set_pos (*v_counts, *i, 0, 3);
    switch (voxel_statistic) {
      case V_SUM:
        for (size_t index = 0; index != sh_coefs.size(); ++index)
          sh_coefs[index] += (i->get_tod()[index] * weight * factor);
        set_tod (sh_coefs);
        break;
      // For TOD, need to store min/max factors - counts buffer is hijacked to do this
      case V_MIN:
        if (factor < (*v_counts).value()) {
          (*v_counts).value() = factor;
          set_tod (i->get_tod());
        }
        break;
      case V_MAX:
        if (factor > (*v_counts).value()) {
          (*v_counts).value() = factor;
          set_tod (i->get_tod());
        }
        break;
      case V_MEAN:
        for (size_t index = 0; index != sh_coefs.size(); ++index)
          sh_coefs[index] += (i->get_tod()[index] * weight * factor);
        set_tod (sh_coefs);
        (*v_counts).value() += weight;
        break;
      default:
        throw Exception ("Unknown / unhandled voxel statistic in MapWriter::operator() (SetVoxelTOD)");
    }
  }
  return true;
}





template <typename value_type>
Point<value_type> MapWriter<value_type>::get_dec ()
{
  assert (type == DEC);
  Point<float> value;
  v_buffer[3] = 0; value[0] = v_buffer.value();
  v_buffer[3] = 1; value[1] = v_buffer.value();
  v_buffer[3] = 2; value[2] = v_buffer.value();
  return value;
}

template <typename value_type>
void MapWriter<value_type>::set_dec (const Point<value_type>& value)
{
  assert (type == DEC);
  v_buffer[3] = 0; v_buffer.value() = value[0];
  v_buffer[3] = 1; v_buffer.value() = value[1];
  v_buffer[3] = 2; v_buffer.value() = value[2];
}





template <typename value_type>
void MapWriter<value_type>::get_tod (Math::Vector<float>& sh_coefs)
{
  assert (type == TOD);
  sh_coefs.allocate (v_buffer.dim(3));
  for (v_buffer[3] = 0; v_buffer[3] != v_buffer.dim(3); ++v_buffer[3])
    sh_coefs[size_t(v_buffer[3])] = v_buffer.value();
}

template <typename value_type>
void MapWriter<value_type>::set_tod (const Math::Vector<float>& sh_coefs)
{
  assert (type == TOD);
  assert (int(sh_coefs.size()) == v_buffer.dim(3));
  for (v_buffer[3] = 0; v_buffer[3] != v_buffer.dim(3); ++v_buffer[3])
    v_buffer.value() = sh_coefs[size_t(v_buffer[3])];
}





}
}
}
}

#endif



