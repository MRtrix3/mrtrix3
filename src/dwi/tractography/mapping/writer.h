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


#include "dataset/buffer.h"
#include "dataset/loop.h"
#include "image/header.h"
#include "thread/queue.h"

#include "dwi/tractography/mapping/twi_stats.h"
#include "dwi/tractography/mapping/voxel.h"


namespace MR {
namespace DWI {
namespace Tractography {
namespace Mapping {


template <typename Cont>
class MapWriterBase
{

  public:
    MapWriterBase (Thread::Queue<Cont>& queue, Image::Header& header, const stat_t s) :
      reader (queue),
      H (header),
      voxel_statistic (s),
      counts ((s == MEAN) ? (new DataSet::Buffer<uint32_t>(header, 3, "counts")) : NULL)
    { }

    virtual void execute () = 0;

  protected:
    typename Thread::Queue<Cont>::Reader reader;
    Image::Header& H;
    const stat_t voxel_statistic;
    Ptr< DataSet::Buffer<uint32_t> > counts;

};


template <typename Cont>
class MapWriterScalar : public MapWriterBase<Cont>
{
  public:
    MapWriterScalar (Thread::Queue<Cont>& queue, Image::Header& header, const stat_t voxel_statistic) :
      MapWriterBase<Cont> (queue, header, voxel_statistic),
      buffer (MapWriterBase<Cont>::H, 3, "buffer")
    {
    	DataSet::Loop loop;
    	if (voxel_statistic == MIN) {
    		for (loop.start (buffer); loop.ok(); loop.next (buffer))
    			buffer.value() =  INFINITY;
    	} else if (voxel_statistic == MAX) {
    		for (loop.start (buffer); loop.ok(); loop.next (buffer))
    			buffer.value() = -INFINITY;
    	}
    }

    ~MapWriterScalar ()
    {
    	DataSet::Loop loop;
      switch (MapWriterBase<Cont>::voxel_statistic) {

        case SUM: break;

        case MIN:
        	for (loop.start (buffer); loop.ok(); loop.next (buffer)) {
        		if (buffer.value() ==  INFINITY)
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
        		if (buffer.value() == -INFINITY)
        			buffer.value() = 0.0;
        	}
        	break;

        default:
          throw Exception ("Unknown / unhandled voxel statistic in ~MapWriterScalar()");

      }
      Image::Voxel<float> vox (MapWriterBase<Cont>::H);
      DataSet::LoopInOrder loopinorder (vox, "writing image to file...", 0, 3);
      for (loopinorder.start (vox, buffer); loopinorder.ok(); loopinorder.next (vox, buffer))
        vox.value() = buffer.value();
    }


    void execute ()
    {
      typename Thread::Queue<Cont>::Reader::Item item (MapWriterBase<Cont>::reader);
      while (item.read()) {
        for (typename Cont::const_iterator i = item->begin(); i != item->end(); ++i) {
          buffer[0] = (*i)[0];
          buffer[1] = (*i)[1];
          buffer[2] = (*i)[2];
          const float factor = get_factor (*item, i);
          switch (MapWriterBase<Cont>::voxel_statistic) {
            case SUM:  buffer.value() += factor; break;
            case MIN:  buffer.value() = MIN(buffer.value(), factor); break;
            case MAX:  buffer.value() = MAX(buffer.value(), factor); break;
            case MEAN:
              // Only increment counts[] if it is necessary to do so given the chosen statistic
              buffer.value() += factor;
              (*MapWriterBase<Cont>::counts)[0] = (*i)[0];
              (*MapWriterBase<Cont>::counts)[1] = (*i)[1];
              (*MapWriterBase<Cont>::counts)[2] = (*i)[2];
              (*MapWriterBase<Cont>::counts).value() += 1;
              break;
            default:
              throw Exception ("Unknown / unhandled voxel statistic in MapWriterScalar::execute()");
          }
        }
      }
    }


  private:
    DataSet::Buffer<float> buffer;

    float get_factor (const Cont&, const typename Cont::const_iterator) const;


};

template <> float MapWriterScalar<SetVoxel>      ::get_factor (const SetVoxel&       set, const SetVoxel      ::const_iterator item) const { return set.factor; }
template <> float MapWriterScalar<SetVoxelFactor>::get_factor (const SetVoxelFactor& set, const SetVoxelFactor::const_iterator item) const { return item->get_factor(); }



template <typename Cont>
class MapWriterColour : public MapWriterBase<Cont>
{

  public:
    MapWriterColour (Thread::Queue<Cont>& queue, Image::Header& header, const stat_t voxel_statistic) :
      MapWriterBase<Cont>(queue, header, voxel_statistic),
      buffer (MapWriterBase<Cont>::H, 3, "directional_buffer")
    {
      if (voxel_statistic != SUM && voxel_statistic != MEAN)
        throw Exception ("Attempting to create MapWriterColour with a voxel statistic other than sum or mean");
    }

    ~MapWriterColour ()
    {
      Image::Voxel<float> vox (MapWriterBase<Cont>::H);
      DataSet::LoopInOrder loop (vox, "writing image to file...", 0, 3);
      for (loop.start (vox, buffer); loop.ok(); loop.next (vox, buffer)) {
        Point<float> temp = buffer.value();
        // Mean direction - no need to track the number of contribution
        if ((MapWriterBase<Cont>::voxel_statistic == MEAN) && (temp != Point<float> (0.0, 0.0, 0.0)))
          temp.normalise();
        vox[3] = 0; vox.value() = temp[0];
        vox[3] = 1; vox.value() = temp[1];
        vox[3] = 2; vox.value() = temp[2];
      }
    }

    void execute ()
    {
      typename Thread::Queue<Cont>::Reader::Item item (MapWriterBase<Cont>::reader);
      while (item.read()) {
        for (typename Cont::const_iterator i = item->begin(); i != item->end(); ++i) {
          buffer[0] = (*i)[0];
          buffer[1] = (*i)[1];
          buffer[2] = (*i)[2];
          // Voxel statistic for DECTDI is always sum
          // This also means that there is no need to increment counts[]
          // If this is being used for MEAN_DIR contrast, the direction can just be normalised; again, don't need counts
          buffer.value() += abs (i->dir);
        }
      }
    }


  private:
    DataSet::Buffer< Point<float> > buffer;

};



}
}
}
}

#endif



