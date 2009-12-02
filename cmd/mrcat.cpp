/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

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

#include "app.h"
#include "progressbar.h"
#include "image/voxel.h"
#include "dataset/copy.h"

using namespace std; 
using namespace MR; 

SET_VERSION_DEFAULT;

DESCRIPTION = {
  "concatenate several images into one",
  NULL
};

ARGUMENTS = {
  Argument ("image1", "first input image", "the first input image.").type_image_in (),
  Argument ("image2", "second input image", "the second input image.", AllowMultiple).type_image_in (),
  Argument ("output", "output image", "the output image.").type_image_out (),
  Argument::End
};

OPTIONS = { 
  Option ("axis", "concatenation axis", "specify axis along which concatenation should be performed. By default, the program will use the axis after the last non-singleton axis of any of the input images")
    .append (Argument ("axis", "axis", "the concatenation axis").type_integer (0, INT_MAX, 0)),

  Option ("datatype", "data type", "specify output image data type.")
    .append (Argument ("spec", "specifier", "the data type specifier.").type_choice (DataType::identifiers)),

  Option::End 
};



class Wrapper {
  public:
    Wrapper (const VecPtr<Image::Header>& in, size_t axis = SIZE_MAX) : 
      V (in.size()),
      offset (in.size()),
      current (0) { 
        N = SIZE_MAX;
        size_t ndim_max (0);
        for (size_t n = 0; n < in.size(); ++n) {
          V[n] = new Voxel (*in[n]);
          if (in[n]->ndim() < N) N = in[n]->ndim();
          if (in[n]->ndim() > ndim_max) ndim_max = in[n]->ndim();
        }

        if (axis == SIZE_MAX) {
          for (size_t a = N; a < ndim_max && axis == SIZE_MAX; ++a) {
            for (size_t n = 1; n < V.size(); ++n) {
              if (V[n]->dim(a) != V[0]->dim(a)) {
                axis = a; 
                break;
              }
            }
          }
          if (axis == SIZE_MAX) axis = N;
        }

        for (size_t a = 0; a < ndim_max; ++a) {
          if (a == axis) continue;
          for (size_t n = 1; n < in.size(); ++n) {
            if (V[0]->dim(a) != V[n]->dim(a)) throw Exception ("dimensions do not match between non-concatenated axes");
          }
        }

        A = axis;
        if (A >= N) N = A+1;
        NA = 0;

        for (size_t n = 0; n < in.size(); ++n) {
          offset[n] = NA;
          NA += V[n]->dim(A);
        }
      }

    std::string name () const { return ("concatenated set"); }
    size_t ndim () const { return (N); }
    ssize_t dim (size_t axis) const { return (axis == A ? NA : V[0]->dim(axis)); }
    float   vox (size_t axis) const { return (V[0]->vox(axis)); }
    float   value () const { return (V[current]->value()); }
    ssize_t pos (size_t axis) const {
      ssize_t p = V[current]->pos(axis);
      if (axis == A) p += offset[current];
      return (p);
    }
    void pos (size_t axis, ssize_t position) {
      if (axis == A) {
        size_t n = 0;
        while (n < offset.size()-1) {
          if (position < offset[n+1]) break;
          ++n;
        }
        if (n != current) {
          for (size_t a = 0; a < V[n]->ndim(); ++a) 
            if (a != A) 
              V[n]->pos(a, V[current]->pos(a));
          current = n;
        }
        position -= offset[current];
      }
      V[current]->pos (axis, position);
    }
    void move (size_t axis, ssize_t increment) {
      if (axis == A) {
        ssize_t p = V[current]->pos(A);
        if (p+increment < 0 || p+increment >= V[current]->dim(A)) {
          pos (A, pos(A)+increment);
          return;
        }
      }
      V[current]->move (axis, increment);
    }

    const Math::Matrix<float>& transform () const { return (V[0]->transform()); }

  private:
    class Voxel : public Image::Voxel<float> {
      public:
        typedef Image::Voxel<float>::value_type value_type;

        Voxel (const Image::Header& header) : Image::Voxel<float> (header) { }
        ssize_t dim (size_t axis) const { return (axis < ndim() ? Image::Voxel<float>::dim (axis) : 1); }
        ssize_t pos (size_t axis) const { return (axis < ndim() ? Image::Voxel<float>::pos (axis) : 0); }
        void pos (size_t axis, ssize_t position) { 
          if (axis < ndim()) Image::Voxel<float>::pos (axis, position); 
          else assert (position == 0); 
        }
        void move (size_t axis, ssize_t increment) { 
          if (axis < ndim()) Image::Voxel<float>::move (axis, increment); 
          else assert (0); 
        }
    };

    VecPtr<Voxel> V;
    std::vector<ssize_t> offset;
    size_t N, NA, A, current;
};

EXECUTE {
  size_t axis = SIZE_MAX;

  std::vector<OptBase> opt = get_options (0); // axis
  if (opt.size()) axis = opt[0][0].get_int();

  VecPtr<Image::Header> in (argument.size()-1);
  for (size_t n = 0; n < in.size(); ++n)
    in[n] = new Image::Header (argument[n].get_image());

  Wrapper wrapper (in, axis);

  Image::Header header (*in[0]);
  header = wrapper;

  header.datatype() = DataType::Float32;
  opt = get_options (1); // datatype
  if (opt.size()) header.datatype().parse (DataType::identifiers[opt[0][0].get_int()]);

  assert (!header.is_complex());

  const Image::Header header_out (argument.back().get_image (header));
  Image::Voxel<float> out (header_out);

  DataSet::loop2 ("concatenating...", DataSet::copy_kernel<Image::Voxel<float>, Wrapper>, out, wrapper);
}

