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

#include "dwi/tractography/mapping/twi_stats.h"
#include "dwi/tractography/mapping/voxel.h"



#include <typeinfo>


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

  out_mih << "\nlayout: " << ((H.stride (0) > 0) ? "+" : "-") << Math::abs (H.stride (0))-1;
  for (size_t n = 1; n < H.ndim(); ++n)
    out_mih << "," << ((H.stride (n) > 0) ? "+" : "-") << Math::abs (H.stride (n))-1;

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




template <typename Cont>
class MapWriterBase
{

  typedef Image::BufferScratch<uint32_t>::voxel_type counts_voxel_type;

  public:
    MapWriterBase (Image::Header& header, const std::string& name, const bool dump, const stat_t s) :
      H (header),
      output_image_name (name),
      direct_dump (dump),
      voxel_statistic (s),
      counts ((s == MEAN) ? (new Image::BufferScratch<uint32_t>(header, "counts")) : NULL),
      v_counts (counts ? new counts_voxel_type (*counts) : NULL)
    { }

    MapWriterBase (const MapWriterBase& that) :
      H (that.H),
      output_image_name (that.output_image_name),
      direct_dump (that.direct_dump),
      voxel_statistic (that.voxel_statistic)
    {
      throw Exception ("Do not instantiate copy constructor for MapWriterBase");
    }


    virtual ~MapWriterBase() { }

    virtual bool operator() (const Cont&) { return false; }

  protected:
    Image::Header& H;
    const std::string output_image_name;
    const bool direct_dump;
    const stat_t voxel_statistic;
    Ptr< Image::BufferScratch<uint32_t> > counts;
    Ptr< counts_voxel_type > v_counts;

};



template <typename Cont> float get_factor (const Cont&, const typename Cont::const_iterator) { assert (0); return 0; }
template <> float get_factor<SetVoxel>          (const SetVoxel&         , const SetVoxel         ::const_iterator);
template <> float get_factor<SetVoxelDEC>       (const SetVoxelDEC&      , const SetVoxelDEC      ::const_iterator);
template <> float get_factor<SetVoxelDir>       (const SetVoxelDir&      , const SetVoxelDir      ::const_iterator);
template <> float get_factor<SetVoxelFactor>    (const SetVoxelFactor&   , const SetVoxelFactor   ::const_iterator);
template <> float get_factor<SetVoxelDECFactor> (const SetVoxelDECFactor&, const SetVoxelDECFactor::const_iterator);



template <class value_type, typename Cont>
class MapWriter : public MapWriterBase<Cont>
{

  typedef typename Image::Buffer<value_type> image_type;
  typedef typename Image::Buffer<value_type>::voxel_type image_voxel_type;
  typedef typename Image::BufferScratch<value_type> buffer_type;
  typedef typename Image::BufferScratch<value_type>::voxel_type buffer_voxel_type;

  public:
    MapWriter (Image::Header& header, const std::string& name, const bool direct_dump = false, const stat_t voxel_statistic = SUM) :
      MapWriterBase<Cont> (header, name, direct_dump, voxel_statistic),
      buffer (header, "buffer"),
      v_buffer (buffer)
    {
      Image::Loop loop;
      if (voxel_statistic == MIN) {
        for (loop.start (v_buffer); loop.ok(); loop.next (v_buffer))
          v_buffer.value() = std::numeric_limits<value_type>::max();
      } else if (voxel_statistic == MAX) {
        for (loop.start (v_buffer); loop.ok(); loop.next (v_buffer))
          v_buffer.value() = std::numeric_limits<value_type>::min();
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

        case SUM: break;

        case MIN:
          for (loop.start (v_buffer); loop.ok(); loop.next (v_buffer)) {
            if (v_buffer.value() == std::numeric_limits<value_type>::max())
              v_buffer.value() = 0.0;
          }
          break;

        case MEAN:
          for (loop.start (v_buffer, *MapWriterBase<Cont>::v_counts); loop.ok(); loop.next (v_buffer, *MapWriterBase<Cont>::v_counts)) {
            if ((*MapWriterBase<Cont>::v_counts).value())
              v_buffer.value() /= float(MapWriterBase<Cont>::v_counts->value());
          }
          break;

        case MAX:
          for (loop.start (v_buffer); loop.ok(); loop.next (v_buffer)) {
            if (v_buffer.value() == std::numeric_limits<value_type>::min())
              v_buffer.value() = 0.0;
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
        case SUM:  v_buffer.value() += factor;                     break;
        case MIN:  v_buffer.value() = MIN(v_buffer.value(), factor); break;
        case MAX:  v_buffer.value() = MAX(v_buffer.value(), factor); break;
        case MEAN:
          // Only increment counts[] if it is necessary to do so given the chosen statistic
          v_buffer.value() += factor;
          Image::Nav::set_pos (*MapWriterBase<Cont>::v_counts, *i);
          (*MapWriterBase<Cont>::v_counts).value() += 1;
          break;
        default:
          throw Exception ("Unknown / unhandled voxel statistic in MapWriter::execute()");
        }
      }
      return true;
    }


  private:
    BufferScratchDump<value_type> buffer;
    buffer_voxel_type v_buffer;

};


template <> bool MapWriter<float, SetVoxelDir>::operator () (const SetVoxelDir& in);



template <typename Cont>
class MapWriterColour : public MapWriterBase<Cont>
{

  typedef typename Image::Buffer<float> image_type;
  typedef typename Image::Buffer<float>::voxel_type image_voxel_type;
  typedef typename Image::BufferScratch<float> buffer_type;
  typedef typename Image::BufferScratch<float>::voxel_type buffer_voxel_type;

  public:
    MapWriterColour (Image::Header& header, const std::string& name, const bool direct_dump = false, const stat_t voxel_statistic = SUM) :
      MapWriterBase<Cont> (header, name, direct_dump, voxel_statistic),
      buffer (header, "buffer"),
      v_buffer (buffer)
    {
      Image::Loop loop;
      if (voxel_statistic == MIN) {
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

        case SUM: break;

        case MIN:
          for (loop.start (v_buffer); loop.ok(); loop.next (v_buffer)) {
            const Point<float> value (get_value());
            if (value == Point<float> (std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()))
              set_value (Point<float> (0.0, 0.0, 0.0));
          }
          break;

        case MEAN:
          for (loop.start (v_buffer); loop.ok(); loop.next (v_buffer)) {
            Point<float> value (get_value());
            if (value.norm2()) {
              value.normalise();
              set_value (value);
            }
          }
          break;

        case MAX:
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
        case SUM:
          set_value (current_value + scaled_colour);
          break;
        case MIN:
          if (scaled_colour.norm2() < current_value.norm2())
            set_value (scaled_colour);
          break;
        case MAX:
          if (scaled_colour.norm2() > current_value.norm2())
            set_value (scaled_colour);
          break;
        case MEAN:
          set_value (current_value + scaled_colour);
          Image::Nav::set_pos (*MapWriterBase<Cont>::v_counts, *i);
          (*MapWriterBase<Cont>::v_counts).value() += 1;
          break;
        default:
          throw Exception ("Unknown / unhandled voxel statistic in MapWriter::execute()");
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


// If using precise mapping, need to explicitly map to positive octant
// Only sum is supported, no factor provided
template <> bool MapWriterColour<SetVoxelDir>::operator () (const SetVoxelDir&);



}
}
}
}

#endif



