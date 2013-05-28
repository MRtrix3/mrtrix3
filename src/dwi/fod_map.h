/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert Smith, 2012.

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



#ifndef __dwi_fod_map_h__
#define __dwi_fod_map_h__


#include "image/buffer_scratch.h"
#include "image/loop.h"
#include "image/nav.h"
#include "image/voxel.h"

#include "dwi/fmls.h"
#include "dwi/directions/mask.h"


namespace MR
{
  namespace DWI
  {



    template <class Lobe>
    class FOD_map
    {

      public:
        template <typename Set>
        FOD_map (const Set& i) :
        info_ (i),
        data  (info_, "FOD map voxels"),
        accessor (data)
        {
          Image::Voxel< Image::BufferScratch<MapVoxel*> > v (data);
          Image::Loop loop;
          for (loop.start (v); loop.ok(); loop.next (v))
            v.value() = NULL;
          // Lobes[0] is an invaid lobe, as provided by the relevant empty constructor
          // This allows index 0 to be used as an error code, simplifying the implementation of MapVoxel and Iterator
          lobes.push_back (Lobe());
        }

        class MapVoxel;
        typedef Image::Voxel< Image::BufferScratch<MapVoxel*> > VoxelAccessor;

        virtual ~FOD_map()
        {
          Image::Loop loop;
          VoxelAccessor v (accessor);
          for (loop.start (v); loop.ok(); loop.next (v)) {
            if (v.value()) {
              delete v.value();
              v.value() = NULL;
            }
          }
        }


        class Iterator;
        class ConstIterator;

        Iterator      begin (VoxelAccessor& v)       { return Iterator      (v.value(), *this); }
        ConstIterator begin (VoxelAccessor& v) const { return ConstIterator (v.value(), *this); }

        Lobe&       operator[] (const size_t i)       { return lobes[i]; }
        const Lobe& operator[] (const size_t i) const { return lobes[i]; }

        bool operator() (const FMLS::FOD_lobes& in);

        const Image::Info& info() const { return info_; }


      protected:

        const class Info : public Image::Info {
          public:
          template <typename T>
          Info (const T& i) :
          Image::Info (i.info())
          {
            set_ndim (3);
          }
          Info () : Image::Info () { }
        } info_;

        Image::BufferScratch<MapVoxel*> data;
        const VoxelAccessor accessor; // Functions can copy-construct their own voxel accessor from this and retain const-ness
        std::vector<Lobe> lobes;


        FOD_map (const FOD_map& that) : info_ (that.data), data (info_) { assert (0); }


    };




    template <class Lobe>
    class FOD_map<Lobe>::MapVoxel
    {
      public:
        MapVoxel (const FMLS::FOD_lobes& in, const size_t first) :
          first_lobe_index (first),
          count (in.size()),
          lookup_table (new uint8_t[in.lut.size()])
        {
          memcpy (lookup_table, &in.lut[0], in.lut.size() * sizeof (uint8_t));
        }

        MapVoxel (const size_t first, const size_t size) :
          first_lobe_index (first),
          count (size),
          lookup_table (NULL) { }

        ~MapVoxel()
        {
          if (lookup_table) {
            delete lookup_table;
            lookup_table = NULL;
          }
        }

        size_t first_index() const { return first_lobe_index; }
        size_t num_lobes()   const { return count; }
        bool   empty()       const { return !count; }

        // Direction must have been assigned to a histogram bin first
        size_t dir2lobe (const size_t dir) const
        {
          assert (lookup_table);
          const size_t offset = lookup_table[dir];
          return ((offset == count) ? 0 : (first_lobe_index + offset));
        }


      private:
        size_t first_lobe_index, count;
        uint8_t* lookup_table;

    };



    template <class Lobe>
    class FOD_map<Lobe>::Iterator
    {
        friend class FOD_map<Lobe>::ConstIterator;
      public:
        Iterator (const MapVoxel* const voxel, FOD_map<Lobe>& parent) :
          index (voxel ? voxel->first_index() : 0),
          last  (voxel ? (index + voxel->num_lobes()) : 0),
          fod_map (parent) { }
        Iterator& operator++ ()       { ++index; return *this; }
        Lobe&     operator() () const { return fod_map.lobes[index]; }
        operator  bool ()       const { return (index != last); }
        operator  size_t ()     const { return index; }
      private:
        size_t index, last;
        FOD_map<Lobe>& fod_map;
    };

    template <class Lobe>
    class FOD_map<Lobe>::ConstIterator
    {
      public:
        ConstIterator (const MapVoxel* const voxel, const FOD_map& parent) :
          index   (voxel ? voxel->first_index() : 0),
          last    (voxel ? (index + voxel->num_lobes()) : 0),
          fod_map (parent) { }
        ConstIterator (const Iterator& that) :
          index   (that.index),
          last    (that.last),
          fod_map (that.fod_map) { }
        ConstIterator&  operator++ ()       { ++index; return *this; }
        const Lobe&     operator() () const { return fod_map.lobes[index]; }
        operator        bool ()       const { return (index != last); }
        operator        size_t ()     const { return index; }
      private:
        size_t index, last;
        const FOD_map<Lobe>& fod_map;
    };




    template <class Lobe>
    bool FOD_map<Lobe>::operator() (const FMLS::FOD_lobes& in)
    {
        if (in.empty())
          return true;
        if (!Image::Nav::within_bounds (data, in.vox))
          return false;
        VoxelAccessor v (accessor);
        Image::Nav::set_pos (v, in.vox);
        if (v.value())
          throw Exception ("FIXME: FOD_map has received multiple segmentations for the same voxel!");
        v.value() = new MapVoxel (in, lobes.size());
        for (FMLS::FOD_lobes::const_iterator i = in.begin(); i != in.end(); ++i)
          lobes.push_back (Lobe (*i));
        return true;
    }




  }
}



#endif
