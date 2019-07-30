/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#ifndef __dwi_fixel_map_h__
#define __dwi_fixel_map_h__


#include "header.h"
#include "image.h"
#include "algo/loop.h"
#include "dwi/fmls.h"
#include "dwi/directions/mask.h"


namespace MR
{
  namespace DWI
  {



    template <class Fixel>
    class Fixel_map { MEMALIGN(Fixel_map<Fixel>)

      public:
        Fixel_map (const Header& H) :
            _header (H),
            _accessor (Image<MapVoxel*>::scratch (_header, "fixel map voxels"))
        {
          for (auto l = Loop() (_accessor); l; ++l)
            _accessor.value() = nullptr;
          // fixels[0] is an invalid fixel, as provided by the relevant empty constructor
          // This allows index 0 to be used as an error code, simplifying the implementation of MapVoxel and Iterator
          fixels.push_back (Fixel());
        }
        Fixel_map (const Fixel_map&) = delete;

        class MapVoxel;
        using VoxelAccessor = Image<MapVoxel*>;

        virtual ~Fixel_map()
        {
          for (auto l = Loop() (_accessor); l; ++l) {
            if (_accessor.value()) {
              delete _accessor.value();
              _accessor.value() = nullptr;
            }
          }
        }


        class Iterator;
        class ConstIterator;

        Iterator      begin (VoxelAccessor& v)       { return Iterator      (v.value(), *this); }
        ConstIterator begin (VoxelAccessor& v) const { return ConstIterator (v.value(), *this); }

        Fixel&       operator[] (const size_t i)       { return fixels[i]; }
        const Fixel& operator[] (const size_t i) const { return fixels[i]; }

        virtual bool operator() (const FMLS::FOD_lobes& in);

        // Functions can copy-construct their own voxel accessor from this and retain const-ness:
        const VoxelAccessor& accessor() const { return _accessor; }

        const ::MR::Header& header() const { return _header; }

      protected:
        vector<Fixel> fixels;

      private:
        const class HeaderHelper : public ::MR::Header { MEMALIGN(HeaderHelper)
          public:
            HeaderHelper (const ::MR::Header& H) :
                ::MR::Header (H)
            {
              ndim() = 3;
            }
            HeaderHelper() = default;
        } _header;

        VoxelAccessor _accessor;

    };




    template <class Fixel>
    class Fixel_map<Fixel>::MapVoxel
    { MEMALIGN(Fixel_map<Fixel>)
      public:
        MapVoxel (const FMLS::FOD_lobes& in, const size_t first) :
            first_fixel_index (first),
            count (in.size()),
            lookup_table (new uint8_t[in.lut.size()])
        {
          memcpy (lookup_table, &in.lut[0], in.lut.size() * sizeof (uint8_t));
        }

        MapVoxel (const size_t first, const size_t size) :
            first_fixel_index (first),
            count (size),
            lookup_table (nullptr) { }

        ~MapVoxel()
        {
          if (lookup_table) {
            delete[] lookup_table;
            lookup_table = nullptr;
          }
        }

        size_t first_index() const { return first_fixel_index; }
        size_t num_fixels()  const { return count; }
        bool   empty()       const { return !count; }

        // Direction must have been assigned to a histogram bin first
        size_t dir2fixel (const size_t dir) const
        {
          assert (lookup_table);
          const size_t offset = lookup_table[dir];
          return ((offset == count) ? 0 : (first_fixel_index + offset));
        }


      private:
        size_t first_fixel_index, count;
        uint8_t* lookup_table;

    };



    template <class Fixel>
    class Fixel_map<Fixel>::Iterator { NOMEMALIGN
        friend class Fixel_map<Fixel>::ConstIterator;
      public:
        Iterator (const MapVoxel* const voxel, Fixel_map<Fixel>& parent) :
            index     (voxel ? voxel->first_index() : 0),
            last      (voxel ? (index + voxel->num_fixels()) : 0),
            fixel_map (parent) { }
        Iterator (const Iterator&) = default;
        Iterator& operator++ ()       { ++index; return *this; }
        Fixel&    operator() () const { return fixel_map.fixels[index]; }
        operator  bool ()       const { return (index != last); }
        operator  size_t ()     const { return index; }
      private:
        size_t index, last;
        Fixel_map<Fixel>& fixel_map;
    };

    template <class Fixel>
    class Fixel_map<Fixel>::ConstIterator { NOMEMALIGN
      public:
        ConstIterator (const MapVoxel* const voxel, const Fixel_map& parent) :
            index     (voxel ? voxel->first_index() : 0),
            last      (voxel ? (index + voxel->num_fixels()) : 0),
            fixel_map (parent) { }
        ConstIterator (const ConstIterator&) = default;
        ConstIterator&  operator++ ()       { ++index; return *this; }
        const Fixel&    operator() () const { return fixel_map.fixels[index]; }
        operator        bool ()       const { return (index != last); }
        operator        size_t ()     const { return index; }
      private:
        size_t index, last;
        const Fixel_map<Fixel>& fixel_map;
    };




    template <class Fixel>
    bool Fixel_map<Fixel>::operator() (const FMLS::FOD_lobes& in)
    {
      if (in.empty())
        return true;
      auto v = accessor();
      assign_pos_of (in.vox).to (v);
      if (is_out_of_bounds (v))
        return false;
      if (v.value())
        throw Exception ("FIXME: FOD_map has received multiple segmentations for the same voxel!");
      v.value() = new MapVoxel (in, fixels.size());
      for (const auto& i : in)
        fixels.push_back (Fixel (i));
      return true;
    }




  }
}



#endif
