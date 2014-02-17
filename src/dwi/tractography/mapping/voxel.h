/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


#ifndef __dwi_tractography_mapping_voxel_h__
#define __dwi_tractography_mapping_voxel_h__



#include <set>

#include "point.h"

#include "image/info.h"


namespace MR {
namespace DWI {
namespace Tractography {
namespace Mapping {



class Voxel : public Point<int>
{
  public:
    Voxel (const int x, const int y, const int z) { p[0] = x; p[1] = y; p[2] = z; }
    Voxel (const Point<int>& that) : Point<int> (that) { }
    Voxel () { memset (p, 0x00, 3 * sizeof(int)); }
    bool operator< (const Voxel& V) const { return ((p[2] == V.p[2]) ? ((p[1] == V.p[1]) ? (p[0] < V.p[0]) : (p[1] < V.p[1])) : (p[2] < V.p[2])); }
};


inline Voxel round (const Point<float>& p)
{ 
  assert (std::isfinite (p[0]) && std::isfinite (p[1]) && std::isfinite (p[2]));
  return (Voxel (Math::round<int> (p[0]), Math::round<int> (p[1]), Math::round<int> (p[2])));
}

inline bool check (const Voxel& V, const Image::Info& H)
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
        Voxel (V),
        colour (Point<float> (0.0, 0.0, 0.0)) { }

    VoxelDEC& operator=  (const Voxel& V)          { Voxel::operator= (V); return (*this); }
    bool      operator== (const Voxel& V)    const { return Voxel::operator== (V); }
    bool      operator<  (const VoxelDEC& V) const { return Voxel::operator< (V); }

    void norm_dir() const { colour.normalise(); }
    void set_dir (const Point<float>& i)       { colour[0] =  Math::abs (i[0]); colour[1] =  Math::abs (i[1]); colour[2] =  Math::abs (i[2]); }
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

    template <class Init>
    VoxelFactor (const Init& v, const float f) :
        Voxel (v),
        sum (f),
        contributions (1) { }

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
        VoxelFactor (V),
        colour (Point<float> (0.0, 0.0, 0.0)) { }

    VoxelDECFactor& operator=  (const Voxel& V)                { Voxel::operator= (V); return (*this); }
    bool            operator== (const Voxel& V)          const { return Voxel::operator== (V); }
    bool            operator<  (const VoxelDECFactor& V) const { return Voxel::operator< (V); }

    void norm_dir() const { colour.normalise(); }
    void set_dir (const Point<float>& i)       { colour[0] =  Math::abs (i[0]); colour[1] =  Math::abs (i[1]); colour[2] =  Math::abs (i[2]); }
    void add_dir (const Point<float>& i) const { colour[0] += Math::abs (i[0]); colour[1] += Math::abs (i[1]); colour[2] += Math::abs (i[2]); }
    const Point<float>& get_colour() const { return colour; }

  private:
    mutable Point<float> colour;

};


// Unlike VoxelDEC, here the direction through the voxel is NOT constrained to the 3-axis positive octant
// Reworked VoxelDir:
//   * Direction should be set to a unit vector
//   * VoxelDirs should NOT be added
//   * Separate member variable for length
class VoxelDir : public Voxel
{

  public:
    VoxelDir () :
        dir (0.0, 0.0, 0.0),
        length (0.0) { }

    VoxelDir (const Voxel& V) :
        Voxel (V),
        dir (0.0, 0.0, 0.0),
        length (0.0) { }

    VoxelDir (const Voxel& V, const Point<float>& d, const float l) :
        Voxel (V),
        dir (d),
        length (l) { }

    const Point<float>& get_dir() const { return dir; }
    float get_length() const { return length; }

    VoxelDir& operator=  (const Voxel& V)          { Voxel::operator= (V); return (*this); }
    bool      operator== (const Voxel& V)    const { return Voxel::operator== (V); }
    bool      operator<  (const VoxelDir& V) const { return Voxel::operator== (V) ? true : Voxel::operator< (V); }
    bool      operator== (const VoxelDir& V) const { return false; }


  private:
    Point<float> dir;
    float length;

};



// New Voxel-derived class; assumes tangent has been mapped to a hemisphere basis direction set
class Dixel : public Voxel
{

  public:
    Dixel () :
        dir (invalid),
        value (0.0) { }

    Dixel (const Voxel& V) :
        Voxel (V),
        dir (invalid),
        value (0.0) { }

    Dixel (const Voxel& V, const size_t b) :
        Voxel (V),
        dir (b),
        value (0.0) { }

    Dixel (const Voxel& V, const size_t b, const float v) :
        Voxel (V),
        dir (b),
        value (v) { }

    void set_dir   (const size_t b) { dir = b; }
    void set_value (const float v) const { value = v; }

    bool   valid()     const { return (dir != invalid); }
    size_t get_dir()   const { return dir; }
    float  get_value() const { return value; }

    bool operator== (const Dixel& V) const { return (Voxel::operator== (V) ? (dir == V.dir) : false); }
    bool operator<  (const Dixel& V) const { return (Voxel::operator== (V) ? (dir <  V.dir) : Voxel::operator< (V)); }

  private:
    size_t dir;
    mutable float value;

    static const size_t invalid;

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
class SetDixel          : public std::set<Dixel>         , public SetVoxelExtras { };



}
}
}
}

#endif



