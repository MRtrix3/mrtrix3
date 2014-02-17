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


#ifndef __dwi_tractography_resample_h__
#define __dwi_tractography_resample_h__


#include <vector>

#include "point.h"

#include "math/hermite.h"
#include "math/matrix.h"

#include "dwi/tractography/tracking/generated_track.h"


namespace MR {
namespace DWI {
namespace Tractography {


template <typename T = float>
class Upsampler
{

  public:
    Upsampler () :
        data (4, 3) { }

    Upsampler (const size_t os_ratio) :
        data (4, 3)
    {
      set_ratio (os_ratio);
    }

    Upsampler (const Upsampler<T>& that) :
        M    (that.M),
        temp (M.rows(), 3),
        data (4, 3) { }

    ~Upsampler() { }


    void set_ratio (const size_t);
    bool operator() (std::vector< Point<T> >&);

    size_t get_ratio() const { return (M.rows() + 1); }
    bool   valid ()    const { return (M.is_set()); }


  private:
    Math::Matrix<T> M, temp, data;

    bool interp_prepare (std::vector< Point<T> >&);
    void increment (const Point<T>&);

};






template <typename T>
bool Upsampler<T>::operator() (std::vector< Point<T> >& in)
{
  if (!interp_prepare (in))
    return false;
  std::vector< Point<T> > out;
  for (size_t i = 3; i < in.size(); ++i) {
    out.push_back (in[i-2]);
    increment (in[i]);
    Math::mult (temp, M, data);
    for (size_t row = 0; row != temp.rows(); ++row)
      out.push_back (Point<T> (temp.row (row)));
  }
  out.push_back (in[in.size() - 2]);
  out.swap (in);
  return true;
}



template <typename T>
void Upsampler<T>::set_ratio (const size_t upsample_ratio)
{
  if (upsample_ratio > 1) {
    const size_t dim = upsample_ratio - 1;
    // Hermite tension = 0.1;
    // cubic interpolation (tension = 0.0) looks 'bulgy' between control points
    Math::Hermite<T> interp (0.1);
    M.allocate (dim, 4);
    for (size_t i = 0; i != dim; ++i) {
      interp.set ((i+1.0) / float(upsample_ratio));
      for (size_t j = 0; j != 4; ++j)
        M(i,j) = interp.coef(j);
    }
    temp.allocate (dim, 3);
  } else {
    M.clear();
    temp.clear();
  }
}



template <typename T>
bool Upsampler<T>::interp_prepare (std::vector< Point<T> >& in)
{
  if (!M.is_set() || in.size() < 2)
    return false;
  const size_t s = in.size();
  // Abandoned curvature-based extrapolation - badly posed when step size is not guaranteed to be consistent,
  //   and probably makes little difference anyways
  in.insert    (in.begin(), in[ 0 ] + (in[1] - in[0]));
  in.push_back (            in[ s ] + (in[s] - in[s-1]));
  for (size_t i = 0; i != 3; ++i) {
    data(0,i) = 0.0;
    data(1,i) = (in[0])[i];
    data(2,i) = (in[1])[i];
    data(3,i) = (in[2])[i];
  }
  return true;
}



template <typename T>
void Upsampler<T>::increment (const Point<T>& a)
{
  for (size_t i = 0; i != 3; ++i) {
    data(0,i) = data(1,i);
    data(1,i) = data(2,i);
    data(2,i) = data(3,i);
    data(3,i) = a[i];
  }
}










class Downsampler
{

  public:
    Downsampler () : ratio (1) { }
    Downsampler (const size_t downsample_ratio) : ratio (downsample_ratio) { }

    bool operator() (Tracking::GeneratedTrack&) const;

    template <typename T>
    bool operator() (std::vector< Point<T> >&) const;

    bool valid() const { return (ratio > 1); }
    size_t get_ratio() const { return ratio; }
    void set_ratio (const size_t i) { ratio = i; }

  private:
    size_t ratio;

};




template <typename T>
bool Downsampler::operator() (std::vector< Point<T> >& tck) const
{
  if (ratio <= 1 || tck.empty())
    return false;
  const size_t midpoint = tck.size()/2;
  size_t index_old = (((midpoint - 1) % ratio) + 1);
  size_t index_new = 1;
  while (index_old < tck.size() - 1) {
    tck[index_new++] = tck[index_old];
    index_old += ratio;
  }
  tck[index_new] = tck.back();
  tck.resize (index_new + 1);
  return true;
}






}
}
}

#endif



