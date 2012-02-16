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


#include "image/buffer_scratch.h"
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


template <typename Cont>
class MapWriterBase
{

  typedef Image::BufferScratch<uint32_t> CountType;

  public:
    MapWriterBase (Thread::Queue<Cont>& queue, Image::Header& header, const stat_t s) :
      reader (queue),
      H (header),
      voxel_statistic (s),
      counts_data ((s == MEAN) ? (new CountType (header, "counts")) : NULL),
      counts (counts_data ? (new CountType::voxel_type (*counts_data)) : NULL)
    { }

    virtual ~MapWriterBase() { }

    virtual void execute () = 0;

  protected:
    typename Thread::Queue<Cont>::Reader reader;
    Image::Header& H;
    const stat_t voxel_statistic;
    Ptr<CountType> counts_data;
    Ptr<CountType::voxel_type> counts;

};



template <typename Cont> float get_factor (const Cont&, const typename Cont::const_iterator) { assert (0); return 0; }
template <> float get_factor<SetVoxel>          (const SetVoxel&          set, const SetVoxel         ::const_iterator item) { return set.factor; }
template <> float get_factor<SetVoxelDir>       (const SetVoxelDir&       set, const SetVoxelDir      ::const_iterator item) { return set.factor; }
template <> float get_factor<SetVoxelFactor>    (const SetVoxelFactor&    set, const SetVoxelFactor   ::const_iterator item) { return item->get_factor(); }
template <> float get_factor<SetVoxelDirFactor> (const SetVoxelDirFactor& set, const SetVoxelDirFactor::const_iterator item) { return item->get_factor(); }



template <class value_type, typename Cont>
class MapWriter : public MapWriterBase<Cont>
{
  public:
    MapWriter (Thread::Queue<Cont>& queue, Image::Header& header, const stat_t voxel_statistic) :
      MapWriterBase<Cont> (queue, header, voxel_statistic),
      buffer_data (MapWriterBase<Cont>::H, "buffer"),
      buffer (buffer_data)
    {
      Image::Loop loop;
      if (voxel_statistic == MIN) {
        for (loop.start (buffer); loop.ok(); loop.next (buffer))
          buffer.value() = std::numeric_limits<value_type>::max();
      } else if (voxel_statistic == MAX) {
        for (loop.start (buffer); loop.ok(); loop.next (buffer))
          buffer.value() = std::numeric_limits<value_type>::min();
      }
    }

    ~MapWriter ()
    {
      Image::Loop loop;
      switch (MapWriterBase<Cont>::voxel_statistic) {

        case SUM: break;

        case MIN:
          for (loop.start (buffer); loop.ok(); loop.next (buffer)) {
            if (buffer.value() == std::numeric_limits<value_type>::max())
              buffer.value() = 0.0;
          }
          break;

        case MEAN:
          for (loop.start (buffer, *MapWriterBase<Cont>::counts); loop.ok(); loop.next (buffer, *MapWriterBase<Cont>::counts)) {
            if ((*MapWriterBase<Cont>::counts).value())
              buffer.value() /= float(MapWriterBase<Cont>::counts->value());
          }
          break;

        case MAX:
          for (loop.start (buffer); loop.ok(); loop.next (buffer)) {
            if (buffer.value() == std::numeric_limits<value_type>::min())
              buffer.value() = 0.0;
          }
          break;

        default:
          throw Exception ("Unknown / unhandled voxel statistic in ~MapWriter()");

      }
      Image::Buffer<value_type> data (MapWriterBase<Cont>::H);
      typename Image::Buffer<value_type>::voxel_type vox (data);
      Image::LoopInOrder loopinorder (vox, "writing image to file...", 0, 3);
      for (loopinorder.start (vox, buffer); loopinorder.ok(); loopinorder.next (vox, buffer))
        vox.value() = buffer.value();
    }


    void execute ()
    {
      typename Thread::Queue<Cont>::Reader::Item item (MapWriterBase<Cont>::reader);
      while (item.read()) {
        for (typename Cont::const_iterator i = item->begin(); i != item->end(); ++i) {
          Image::Nav::set_pos (buffer, *i);
          const value_type factor = get_factor (*item, i);
          switch (MapWriterBase<Cont>::voxel_statistic) {
            case SUM:  buffer.value() += factor;                     break;
            case MIN:  buffer.value() = MIN(buffer.value(), factor); break;
            case MAX:  buffer.value() = MAX(buffer.value(), factor); break;
            case MEAN:
              // Only increment counts[] if it is necessary to do so given the chosen statistic
              buffer.value() += factor;
              Image::Nav::set_pos (*MapWriterBase<Cont>::counts, *i);
              (*MapWriterBase<Cont>::counts).value() += 1;
              break;
            default:
              throw Exception ("Unknown / unhandled voxel statistic in MapWriter::execute()");
          }
        }
      }
    }


  private:
    Image::BufferScratch<value_type> buffer_data;
    typename Image::BufferScratch<value_type>::voxel_type buffer;

};



template <>
MapWriter<Point<float>, SetVoxelDir>::MapWriter (Thread::Queue<SetVoxelDir>& queue, Image::Header& header, const stat_t voxel_statistic) :
    MapWriterBase<SetVoxelDir> (queue, header, voxel_statistic),
    buffer_data (MapWriterBase<SetVoxelDir>::H, "buffer"),
    buffer (buffer_data)
{
  Image::Loop loop;
  if (voxel_statistic == MIN) {
    for (loop.start (buffer); loop.ok(); loop.next (buffer))
      buffer.value() = Point<float> (std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
  } else if (voxel_statistic == MAX) {
    for (loop.start (buffer); loop.ok(); loop.next (buffer))
      buffer.value() = Point<float> (0.0, 0.0, 0.0);
  }
}

template <>
MapWriter<Point<float>, SetVoxelDir>::~MapWriter ()
{
  Image::Loop loop;
  switch (MapWriterBase<SetVoxelDir>::voxel_statistic) {

    case SUM: break;

    case MIN:
      for (loop.start (buffer); loop.ok(); loop.next (buffer)) {
        const Point<float>& value (buffer.value());
        if (value == Point<float> (std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()))
          buffer.value() = Point<float> (0.0, 0.0, 0.0);
      }
      break;

    case MEAN:
      for (loop.start (buffer); loop.ok(); loop.next (buffer)) {
        const Point<float>& value (buffer.value());
        if (value.norm2()) {
          Point<float> norm_value (value);
          norm_value.normalise();
          buffer.value() = norm_value;
        }
      }
      break;

    case MAX: break;

    default:
      throw Exception ("Unknown / unhandled voxel statistic in ~MapWriter()");

  }
  Image::Buffer<float> data (MapWriterBase<SetVoxelDir>::H);
  Image::Buffer<float>::voxel_type vox (data);
  Image::LoopInOrder loopinorder (vox, "writing image to file...", 0, 3);
  for (loopinorder.start (vox, buffer); loopinorder.ok(); loopinorder.next (vox, buffer)) {
    const Point<float>& value (buffer.value());
    vox[3] = 0; vox.value() = value[0];
    vox[3] = 1; vox.value() = value[1];
    vox[3] = 2; vox.value() = value[2];
  }

}



template <>
void MapWriter<Point<float>, SetVoxelDir>::execute ()
{
  Thread::Queue<SetVoxelDir>::Reader::Item item (MapWriterBase<SetVoxelDir>::reader);
  while (item.read()) {
    for (SetVoxelDir::const_iterator i = item->begin(); i != item->end(); ++i) {
      Image::Nav::set_pos (buffer, *i);
      const float factor = get_factor (*item, i);
      const Point<float> scaled_dir = i->dir * factor;
      const Point<float>& current_value = buffer.value();
      switch (MapWriterBase<SetVoxelDir>::voxel_statistic) {
        case SUM:
          buffer.value() += scaled_dir;
          break;
        case MIN:
          if (scaled_dir.norm2() < current_value.norm2())
            buffer.value() = scaled_dir;
          break;
        case MAX:
          if (scaled_dir.norm2() > current_value.norm2())
            buffer.value() = scaled_dir;
          break;
        case MEAN:
          buffer.value() += scaled_dir;
          Image::Nav::set_pos (*MapWriterBase<SetVoxelDir>::counts, *i);
          (*MapWriterBase<SetVoxelDir>::counts).value() += 1;
          break;
        default:
          throw Exception ("Unknown / unhandled voxel statistic in MapWriter::execute()");
      }
    }
  }
}



template <>
MapWriter<Point<float>, SetVoxelDirFactor>::MapWriter (Thread::Queue<SetVoxelDirFactor>& queue, Image::Header& header, const stat_t voxel_statistic) :
    MapWriterBase<SetVoxelDirFactor> (queue, header, voxel_statistic),
    buffer_data (MapWriterBase<SetVoxelDirFactor>::H, "buffer"),
    buffer (buffer_data)
{
  Image::Loop loop;
  if (voxel_statistic == MIN) {
    for (loop.start (buffer); loop.ok(); loop.next (buffer))
      buffer.value() = Point<float> (std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
  } else if (voxel_statistic == MAX) {
    for (loop.start (buffer); loop.ok(); loop.next (buffer))
      buffer.value() = Point<float> (0.0, 0.0, 0.0);
  }
}

template <>
MapWriter<Point<float>, SetVoxelDirFactor>::~MapWriter ()
{
  Image::Loop loop;
  switch (MapWriterBase<SetVoxelDirFactor>::voxel_statistic) {

    case SUM: break;

    case MIN:
      for (loop.start (buffer); loop.ok(); loop.next (buffer)) {
        const Point<float>& value (buffer.value());
        if (value == Point<float> (std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()))
          buffer.value() = Point<float> (0.0, 0.0, 0.0);
      }
      break;

    case MEAN:
      for (loop.start (buffer); loop.ok(); loop.next (buffer)) {
        const Point<float>& value (buffer.value());
        if (value.norm2()) {
          Point<float> norm_value (value);
          norm_value.normalise();
          buffer.value() = norm_value;
        }
      }
      break;

    case MAX: break;

    default:
      throw Exception ("Unknown / unhandled voxel statistic in ~MapWriter()");

  }
  Image::Buffer<float> data (MapWriterBase<SetVoxelDirFactor>::H);
  Image::Buffer<float>::voxel_type vox (data);
  Image::LoopInOrder loopinorder (vox, "writing image to file...", 0, 3);
  for (loopinorder.start (vox, buffer); loopinorder.ok(); loopinorder.next (vox, buffer)) {
    const Point<float>& value (buffer.value());
    vox[3] = 0; vox.value() = value[0];
    vox[3] = 1; vox.value() = value[1];
    vox[3] = 2; vox.value() = value[2];
  }

}


template <>
void MapWriter<Point<float>, SetVoxelDirFactor>::execute ()
{
  Thread::Queue<SetVoxelDirFactor>::Reader::Item item (MapWriterBase<SetVoxelDirFactor>::reader);
  while (item.read()) {
    for (SetVoxelDirFactor::const_iterator i = item->begin(); i != item->end(); ++i) {
      Image::Nav::set_pos (buffer, *i);
      const float factor = get_factor (*item, i);
      const Point<float> scaled_dir = i->dir * factor;
      const Point<float>& current_value = buffer.value();
      switch (MapWriterBase<SetVoxelDirFactor>::voxel_statistic) {
        case SUM:
          buffer.value() += scaled_dir;
          break;
        case MIN:
          if (scaled_dir.norm2() < current_value.norm2())
            buffer.value() = scaled_dir;
          break;
        case MAX:
          if (scaled_dir.norm2() > current_value.norm2())
            buffer.value() = scaled_dir;
          break;
        case MEAN:
          buffer.value() += scaled_dir;
          Image::Nav::set_pos (*MapWriterBase<SetVoxelDirFactor>::counts, *i);
          (*MapWriterBase<SetVoxelDirFactor>::counts).value() += 1;
          break;
        default:
          throw Exception ("Unknown / unhandled voxel statistic in MapWriter::execute()");
      }
    }
  }
}

template <>
MapWriter<Point<float>, SetVoxel>::MapWriter (Thread::Queue<SetVoxel>& queue, Image::Header& header, const stat_t voxel_statistic) :
  MapWriterBase<SetVoxel> (queue, header, voxel_statistic),
  buffer_data (MapWriterBase<SetVoxel>::H, "buffer"),
  buffer (buffer_data)
{
  throw Exception ("Invalid type requested: MR::DWI::Tractography::Mapping::MapWriter<Point<float>, SetVoxel>");
}
template <>
MapWriter<Point<float>, SetVoxelFactor>::MapWriter (Thread::Queue<SetVoxelFactor>& queue, Image::Header& header, const stat_t voxel_statistic) :
  MapWriterBase<SetVoxelFactor> (queue, header, voxel_statistic),
  buffer_data (MapWriterBase<SetVoxelFactor>::H, "buffer"),
  buffer (buffer_data)
{
  throw Exception ("Invalid type requested: MR::DWI::Tractography::Mapping::MapWriter<Point<float>, SetVoxelFactor>");
}
template <>
MapWriter<Point<float>, SetVoxel>::~MapWriter ()
{
  throw Exception ("Invalid type requested: MR::DWI::Tractography::Mapping::MapWriter<Point<float>, SetVoxel>");
}
template <>
MapWriter<Point<float>, SetVoxelFactor>::~MapWriter ()
{
  throw Exception ("Invalid type requested: MR::DWI::Tractography::Mapping::MapWriter<Point<float>, SetVoxelFactor>");
}
template <>
void MapWriter<Point<float>, SetVoxel>::execute ()
{
  throw Exception ("Invalid type requested: MR::DWI::Tractography::Mapping::MapWriter<Point<float>, SetVoxel>");
}
template <>
void MapWriter<Point<float>, SetVoxelFactor>::execute ()
{
  throw Exception ("Invalid type requested: MR::DWI::Tractography::Mapping::MapWriter<Point<float>, SetVoxelFactor>");
}



}
}
}
}

#endif



