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
#include "thread_queue.h"

#include "dwi/tractography/mapping/buffer_scratch_dump.h"
#include "dwi/tractography/mapping/twi_stats.h"
#include "dwi/tractography/mapping/voxel.h"
#include "dwi/tractography/mapping/gaussian/voxel.h"



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
      if (i && !Path::has_suffix (output_image_name, ".mih") && !Path::has_suffix (output_image_name, ".mif"))
        throw Exception ("Can only perform direct dump to file for .mih / .mif image format");
    }


    virtual bool operator() (const SetVoxel&)    { return false; }
    virtual bool operator() (const SetVoxelDEC&) { return false; }
    virtual bool operator() (const SetDixel&)    { return false; }
    virtual bool operator() (const SetVoxelTOD&) { return false; }

    virtual bool operator() (const Gaussian::SetVoxel&)    { return false; }
    virtual bool operator() (const Gaussian::SetVoxelDEC&) { return false; }
    virtual bool operator() (const Gaussian::SetDixel&)    { return false; }
    virtual bool operator() (const Gaussian::SetVoxelTOD&) { return false; }


  protected:
    Image::Header& H;
    const std::string output_image_name;
    bool direct_dump;
    const vox_stat_t voxel_statistic;
    const writer_dim type;

    // This gets used with mean voxel statistic for some (but not all) writers,
    //   or if the output is a voxel_summed DEC image.
    // It's also hijacked to store per-voxel min/max factors in the case of TOD
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
          for (auto l = loop (v_buffer); l; ++l )
            v_buffer.value() = std::numeric_limits<value_type>::max();
        } else {
          buffer.zero();
        }

      } else { // Greyscale and dixel

        if (voxel_statistic == V_MIN) {
          for (auto l = loop (v_buffer); l; ++l )
            v_buffer.value() = std::numeric_limits<value_type>::max();
        } else if (voxel_statistic == V_MAX) {
          for (auto l = loop (v_buffer); l; ++l )
            v_buffer.value() = std::numeric_limits<value_type>::lowest();
        } else {
          buffer.zero();
        }

      }

      // With TOD, hijack the counts buffer in voxel statistic min/max mode
      //   (use to store maximum / minimum factors and hence decide when to update the TOD)
      if ((voxel_statistic == V_MEAN) ||
          (type == TOD && (voxel_statistic == V_MIN || voxel_statistic == V_MAX)) ||
          (type == DEC && voxel_statistic == V_SUM))
      {
        Image::Header H_counts (header);
        if (type == DEC || type == TOD) {
          H_counts.set_ndim (3);
          H_counts.sanitise();
        }
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

      Image::LoopInOrder loop (v_buffer, 0, 3);
      switch (voxel_statistic) {

        case V_SUM:
          if (type == DEC) {
            for (auto l = loop (v_buffer, *v_counts); l; ++l) {
              if (v_counts->value()) {
                Point<value_type> value (get_dec());
                value *= v_counts->value() / value.norm();
                set_dec (value);
              }
            }
          }
          break;

        case V_MIN:
          for (auto l = loop (v_buffer); l; ++l ) {
            if (v_buffer.value() == std::numeric_limits<value_type>::max())
              v_buffer.value() = value_type(0);
          }
          break;

        case V_MEAN:
          if (type == GREYSCALE) {
            for (auto l = loop (v_buffer, *v_counts); l; ++l) {
              if (v_counts->value())
                v_buffer.value() /= float(v_counts->value());
            }
          } else if (type == DEC) {
            for (auto l = loop (v_buffer, *v_counts); l; ++l) {
              Point<value_type> value (get_dec());
              if (value.norm2()) {
                value /= v_counts->value();
                set_dec (value);
              }
            }
          } else if (type == TOD) {
            for (auto l = loop (v_buffer, *v_counts); l; ++l) {
              if (v_counts->value()) {
                Math::Vector<float> value;
                get_tod (value);
                value *= (1.0 / v_counts->value());
                set_tod (value);
              }
            }
          } else { // Dixel
            // TODO For dixels, should this be a voxel mean i.e. normalise each non-zero voxel to unit density,
            //   rather than a per-dixel mean?
            Image::LoopInOrder loop_dixel (v_buffer);
            for (auto l = loop_dixel (v_buffer, *v_counts); l; ++l) {
              if (v_counts->value())
                v_buffer.value() /= float(v_counts->value());
            }
          }
          break;

        case V_MAX:
          if (type == GREYSCALE) {
            for (auto l = loop (v_buffer); l; ++l) {
              if (v_buffer.value() == -std::numeric_limits<value_type>::max())
                v_buffer.value() = value_type(0);
            }
          } else if (type == DIXEL) {
            Image::LoopInOrder loop_dixel (v_buffer);
            for (auto l = loop_dixel (v_buffer); l; ++l) {
              if (v_buffer.value() == -std::numeric_limits<value_type>::max())
                v_buffer.value() = value_type(0);
            }
          }
          break;

        default:
          throw Exception ("Unknown / unhandled voxel statistic in ~MapWriter()");

      }

      if (direct_dump) {

        if (App::log_level)
          std::cerr << App::NAME << ": dumping image contents to file... ";
        buffer.dump_to_file (output_image_name, H);
        if (App::log_level)
          std::cerr << "done.\n";

      } else {

        image_type out (output_image_name, H);
        image_voxel_type v_out (out);
        if (type == DEC) {
          Image::LoopInOrder loop_out (v_out, "writing image to file...", 0, 3);
          for (auto l = loop_out (v_out, v_buffer); l; ++l) {
            Point<value_type> value (get_dec());
            v_out[3] = 0; v_out.value() = value[0];
            v_out[3] = 1; v_out.value() = value[1];
            v_out[3] = 2; v_out.value() = value[2];
          }
        } else if (type == TOD) {
          Image::LoopInOrder loop_out (v_out, "writing image to file...", 0, 3);
          for (auto l = loop_out (v_out, v_buffer); l; ++l) {
            Math::Vector<float> value;
            get_tod (value);
            for (v_out[3] = 0; v_out[3] != v_out.dim(3); ++v_out[3])
              v_out.value() = value[size_t(v_out[3])];
          }
        } else { // Greyscale and Dixel
          Image::LoopInOrder loop_out (v_out, "writing image to file...");
          for (auto l = loop_out (v_out, v_buffer); l; ++l) 
            v_out.value() = v_buffer.value();
        }

      }
    }


    bool operator() (const SetVoxel& in)    { receive_greyscale (in); return true; }
    bool operator() (const SetVoxelDEC& in) { receive_dec       (in); return true; }
    bool operator() (const SetDixel& in)    { receive_dixel     (in); return true; }
    bool operator() (const SetVoxelTOD& in) { receive_tod       (in); return true; }

    bool operator() (const Gaussian::SetVoxel& in)    { receive_greyscale (in); return true; }
    bool operator() (const Gaussian::SetVoxelDEC& in) { receive_dec       (in); return true; }
    bool operator() (const Gaussian::SetDixel& in)    { receive_dixel     (in); return true; }
    bool operator() (const Gaussian::SetVoxelTOD& in) { receive_tod       (in); return true; }


  private:
    BufferScratchDump<value_type> buffer;
    buffer_voxel_type v_buffer;

    // Template functions used so that the functors don't have to be written twice
    //   (once for standard TWI and one for Gaussian track-wise statistic)
    template <class Cont> void receive_greyscale (const Cont&);
    template <class Cont> void receive_dec       (const Cont&);
    template <class Cont> void receive_dixel     (const Cont&);
    template <class Cont> void receive_tod       (const Cont&);

    // These acquire the TWI factor at any point along the streamline;
    //   For the standard SetVoxel classes, this is a single value 'factor' for the set as
    //     stored in SetVoxelExtras
    //   For the Gaussian SetVoxel classes, there is a factor per mapped element
    float get_factor (const Voxel&    element, const SetVoxel&    set) const { return set.factor; }
    float get_factor (const VoxelDEC& element, const SetVoxelDEC& set) const { return set.factor; }
    float get_factor (const Dixel&    element, const SetDixel&    set) const { return set.factor; }
    float get_factor (const VoxelTOD& element, const SetVoxelTOD& set) const { return set.factor; }
    float get_factor (const Gaussian::Voxel&    element, const Gaussian::SetVoxel&    set) const { return element.get_factor(); }
    float get_factor (const Gaussian::VoxelDEC& element, const Gaussian::SetVoxelDEC& set) const { return element.get_factor(); }
    float get_factor (const Gaussian::Dixel&    element, const Gaussian::SetDixel&    set) const { return element.get_factor(); }
    float get_factor (const Gaussian::VoxelTOD& element, const Gaussian::SetVoxelTOD& set) const { return element.get_factor(); }


    // Convenience functions for Directionally-Encoded Colour processing
    Point<value_type> get_dec ();
    void              set_dec (const Point<value_type>&);

    // Convenience functions for Track Orientation Distribution processing
    void get_tod (      Math::Vector<float>&);
    void set_tod (const Math::Vector<float>&);

};







template <typename value_type>
template <class Cont>
void MapWriter<value_type>::receive_greyscale (const Cont& in)
{
  assert (MapWriterBase::type == GREYSCALE);
  for (typename Cont::const_iterator i = in.begin(); i != in.end(); ++i) {
    Image::Nav::set_pos (v_buffer, *i);
    const float factor = get_factor (*i, in);
    const float weight = in.weight * i->get_length();
    switch (voxel_statistic) {
      case V_SUM:  v_buffer.value() += weight * factor;              break;
      case V_MIN:  v_buffer.value() = MIN(v_buffer.value(), factor); break;
      case V_MAX:  v_buffer.value() = MAX(v_buffer.value(), factor); break;
      case V_MEAN:
        v_buffer.value() += weight * factor;
        assert (v_counts);
        Image::Nav::set_pos (*v_counts, *i);
        (*v_counts).value() += weight;
        break;
      default:
        throw Exception ("Unknown / unhandled voxel statistic in MapWriter::receive_greyscale()");
    }
  }
}



template <typename value_type>
template <class Cont>
void MapWriter<value_type>::receive_dec (const Cont& in)
{
  assert (type == DEC);
  for (typename Cont::const_iterator i = in.begin(); i != in.end(); ++i) {
    Image::Nav::set_pos (v_buffer, *i);
    const float factor = get_factor (*i, in);
    const float weight = in.weight * i->get_length();
    Point<value_type> scaled_colour (i->get_colour());
    scaled_colour *= factor;
    const Point<value_type> current_value = get_dec();
    switch (voxel_statistic) {
      case V_SUM:
        set_dec (current_value + (scaled_colour * weight));
        assert (v_counts);
        Image::Nav::set_pos (*v_counts, *i);
        (*v_counts).value() += weight;
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
        throw Exception ("Unknown / unhandled voxel statistic in MapWriter::receive_dec()");
    }
  }
}



template <typename value_type>
template <class Cont>
void MapWriter<value_type>::receive_dixel (const Cont& in)
{
  assert (type == DIXEL);
  for (typename Cont::const_iterator i = in.begin(); i != in.end(); ++i) {
    Image::Nav::set_pos (v_buffer, *i, 0, 3);
    v_buffer[3] = i->get_dir();
    const float factor = get_factor (*i, in);
    const float weight = in.weight * i->get_length();
    switch (voxel_statistic) {
      case V_SUM:  v_buffer.value() += weight * factor;              break;
      case V_MIN:  v_buffer.value() = MIN(v_buffer.value(), factor); break;
      case V_MAX:  v_buffer.value() = MAX(v_buffer.value(), factor); break;
      case V_MEAN:
        v_buffer.value() += weight * factor;
        assert (v_counts);
        Image::Nav::set_pos (*v_counts, *i, 0, 3);
        (*v_counts)[3] = i->get_dir();
        (*v_counts).value() += weight;
        break;
      default:
        throw Exception ("Unknown / unhandled voxel statistic in MapWriter::receive_dixel()");
    }
  }
}



template <typename value_type>
template <class Cont>
void MapWriter<value_type>::receive_tod (const Cont& in)
{
  assert (type == TOD);
  Math::Vector<float> sh_coefs;
  for (typename Cont::const_iterator i = in.begin(); i != in.end(); ++i) {
    Image::Nav::set_pos (v_buffer, *i, 0, 3);
    const float factor = get_factor (*i, in);
    const float weight = in.weight * i->get_length();
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
        assert (v_counts);
        if (factor < (*v_counts).value()) {
          (*v_counts).value() = factor;
          Math::Vector<float> tod (i->get_tod());
          tod *= factor;
          set_tod (tod);
        }
        break;
      case V_MAX:
        assert (v_counts);
        if (factor > (*v_counts).value()) {
          (*v_counts).value() = factor;
          Math::Vector<float> tod (i->get_tod());
          tod *= factor;
          set_tod (tod);
        }
        break;
      case V_MEAN:
        assert (v_counts);
        for (size_t index = 0; index != sh_coefs.size(); ++index)
          sh_coefs[index] += (i->get_tod()[index] * weight * factor);
        set_tod (sh_coefs);
        (*v_counts).value() += weight;
        break;
      default:
        throw Exception ("Unknown / unhandled voxel statistic in MapWriter::receive_tod()");
    }
  }
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



