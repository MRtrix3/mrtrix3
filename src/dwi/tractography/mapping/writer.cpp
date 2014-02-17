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


#include "dwi/tractography/mapping/writer.h"


namespace MR {
namespace DWI {
namespace Tractography {
namespace Mapping {


template <> float get_factor<SetVoxel>          (const SetVoxel&          set, const SetVoxel         ::const_iterator item) { return set.factor; }
template <> float get_factor<SetVoxelDEC>       (const SetVoxelDEC&       set, const SetVoxelDEC      ::const_iterator item) { return set.factor; }
template <> float get_factor<SetVoxelDir>       (const SetVoxelDir&       set, const SetVoxelDir      ::const_iterator item) { return set.factor; }
template <> float get_factor<SetVoxelFactor>    (const SetVoxelFactor&    set, const SetVoxelFactor   ::const_iterator item) { return item->get_factor(); }
template <> float get_factor<SetVoxelDECFactor> (const SetVoxelDECFactor& set, const SetVoxelDECFactor::const_iterator item) { return item->get_factor(); }




template <>
bool MapWriter<float, SetVoxelDir>::operator () (const SetVoxelDir& in)
{
  for (SetVoxelDir::const_iterator i = in.begin(); i != in.end(); ++i) {
    Image::Nav::set_pos (v_buffer, *i);
    const float factor = i->get_length();
    switch (MapWriterBase<SetVoxelDir>::voxel_statistic) {
    case V_SUM:  v_buffer.value() += factor;                       break;
    case V_MIN:  v_buffer.value() = MIN(v_buffer.value(), factor); break;
    case V_MEAN:
      // Only increment counts[] if it is necessary to do so given the chosen statistic
      v_buffer.value() += factor;
      Image::Nav::set_pos (*v_counts, *i);
      (*v_counts).value() += 1;
      break;
    case V_MAX:  v_buffer.value() = MAX(v_buffer.value(), factor); break;
    default:
      throw Exception ("Unknown / unhandled voxel statistic in MapWriter::execute()");
    }
  }
  return true;
}


template <>
bool MapWriterColour<SetVoxelDir>::operator () (const SetVoxelDir& in)
{
  for (SetVoxelDir::const_iterator i = in.begin(); i != in.end(); ++i) {
    Image::Nav::set_pos (v_buffer, *i);
    const Point<float> tangent (i->get_dir());
    const Point<float> pos_tangent (Math::abs (tangent[0]), Math::abs (tangent[1]), Math::abs (tangent[2]));
    const Point<float> current_value = get_value();
    set_value (current_value + (pos_tangent * i->get_length()));
  }
  return true;
}


}
}
}
}




