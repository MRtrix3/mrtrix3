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

#ifndef __dwi_tractography_mapping_voxel_h__
#define __dwi_tractography_mapping_voxel_h__



#include <set>

#include "point.h"

#include "image/header.h"


namespace MR {
namespace DWI {
namespace Tractography {
namespace Mapping {



class Voxel : public Point<int>
{
  public:
    Voxel (const int x, const int y, const int z) { p[0] = x; p[1] = y; p[2] = z; }
    Voxel () { memset (p, 0x00, 3 * sizeof(int)); }
    bool operator< (const Voxel& V) const { return ((p[2] == V.p[2]) ? ((p[1] == V.p[1]) ? (p[0] < V.p[0]) : (p[1] < V.p[1])) : (p[2] < V.p[2])); }
};


inline Voxel round (const Point<float>& p)
{ 
  assert (finite (p[0]) && finite (p[1]) && finite (p[2]));
  return (Voxel (Math::round<int> (p[0]), Math::round<int> (p[1]), Math::round<int> (p[2])));
}

inline bool check (const Voxel& V, const Image::Header& H)
{
  return (V[0] >= 0 && V[0] < H.dim(0) && V[1] >= 0 && V[1] < H.dim(1) && V[2] >= 0 && V[2] < H.dim(2));
}

inline Point<float> abs (const Point<float>& d)
{
  return (Point<float> (Math::abs(d[0]), Math::abs(d[1]), Math::abs(d[2])));
}

class VoxelDEC : public Voxel 
{

  public:
    VoxelDEC () :
      colour (Point<float> (0.0, 0.0, 0.0))
    {
      memset (p, 0x00, 3 * sizeof(int));
    }
    VoxelDEC (const Voxel& V) :
      colour (Point<float> (0.0, 0.0, 0.0))
    {
      memcpy (p, V, 3 * sizeof(int));
    }

    VoxelDEC& operator=  (const Voxel& V)          { Voxel::operator= (V); return (*this); }
    bool      operator== (const Voxel& V)    const { return Voxel::operator== (V); }
    bool      operator<  (const VoxelDEC& V) const { return Voxel::operator< (V); }

    void norm_dir() const { colour.normalise(); }
    void set_dir (const Point<float>& i) { colour[0] = Math::abs (i[0]); colour[1] = Math::abs (i[1]); colour[2] = Math::abs (i[2]); }
    void add_dir (const Point<float>& i) const { colour[0] += Math::abs (i[0]); colour[1] += Math::abs (i[1]); colour[2] += Math::abs (i[2]); }
    const Point<float>& get_colour() const { return colour; }

  private:
    mutable Point<float> colour;

};


class VoxelFactor : public Voxel
{

  public:
    VoxelFactor () :
      sum (0.0),
      contributions (0) { }

    VoxelFactor (const int x, const int y, const int z, const float factor) :
      Voxel (x, y, z),
      sum (factor),
      contributions (1) { }

    VoxelFactor (const Voxel& v) :
      Voxel (v),
      sum (0.0),
      contributions (0) { }

    VoxelFactor (const VoxelFactor& v) :
      Voxel (v),
      sum (v.sum),
      contributions (v.contributions) { }

    void add_contribution (const float factor) const {
      sum += factor;
      ++contributions;
    }

    void set_factor (const float i) const { sum = i; contributions = 1; }
    float get_factor() const { return (sum / float(contributions)); }
    size_t get_contribution_count() const { return contributions; }

    VoxelFactor& operator=  (const Voxel& V)             { Voxel::operator= (V); return (*this); }
    bool         operator== (const Voxel& V)       const { return Voxel::operator== (V); }
    bool         operator<  (const VoxelFactor& V) const { return Voxel::operator< (V); }


  protected:
    mutable float sum;
    mutable size_t contributions;

};


class VoxelDECFactor : public VoxelFactor
{

  public:
    VoxelDECFactor () :
      colour (Point<float> (0.0, 0.0, 0.0))
    {
      memset (p, 0x00, 3 * sizeof(int));
    }
    VoxelDECFactor (const Voxel& V) :
      colour (Point<float> (0.0, 0.0, 0.0))
    {
      memcpy (p, V, 3 * sizeof(int));
    }

    VoxelDECFactor& operator=  (const Voxel& V)                { Voxel::operator= (V); return (*this); }
    bool            operator== (const Voxel& V)          const { return Voxel::operator== (V); }
    bool            operator<  (const VoxelDECFactor& V) const { return Voxel::operator< (V); }

    void norm_dir() const { colour.normalise(); }
    void set_dir (const Point<float>& i) { colour[0] = Math::abs (i[0]); colour[1] = Math::abs (i[1]); colour[2] = Math::abs (i[2]); }
    void add_dir (const Point<float>& i) const { colour[0] += Math::abs (i[0]); colour[1] += Math::abs (i[1]); colour[2] += Math::abs (i[2]); }
    const Point<float>& get_colour() const { return colour; }

  private:
    mutable Point<float> colour;

};


// Unlike VoxelDEC, here the direction through the voxel is NOT constrained to the 3-axis positive octant
class VoxelDir : public Voxel
{

  public:
    VoxelDir () :
      dir (0.0, 0.0, 0.0) { }

    VoxelDir (const Voxel& V) :
      dir (0.0, 0.0, 0.0)
    {
      memcpy (p, V, 3 * sizeof(int));
    }

    void add_dir (const Point<float>& i) const { dir += i; }
    const Point<float>& get_dir() const { return dir; }

    VoxelDir& operator=  (const Voxel& V)          { Voxel::operator= (V); return (*this); }
    bool      operator== (const Voxel& V)    const { return Voxel::operator== (V); }
    bool      operator<  (const VoxelDir& V) const { return Voxel::operator< (V); }


  private:
    mutable Point<float> dir;

};



class SetVoxelExtras
{
  public:
    float factor; // For TWI, when contribution to the map is uniform along the length of the track
    size_t index; // Index of the track
};


class SetVoxel          : public std::set<Voxel>         , public SetVoxelExtras { };
class SetVoxelDEC       : public std::set<VoxelDEC>      , public SetVoxelExtras { };
class SetVoxelDir       : public std::set<VoxelDir>      , public SetVoxelExtras { };
class SetVoxelFactor    : public std::set<VoxelFactor>   , public SetVoxelExtras { };
class SetVoxelDECFactor : public std::set<VoxelDECFactor>, public SetVoxelExtras { };



}
}
}
}

#endif



