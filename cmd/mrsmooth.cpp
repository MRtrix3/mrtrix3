/*
    Copyright 2010 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 26/09/10.

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
#include "image/kernel.h"
#include "image/voxel.h"
#include "image/data.h"

MRTRIX_APPLICATION

using namespace MR;
using namespace App;

void usage ()
{
  DESCRIPTION
  + "smooth images using a convolution kernel";

  ARGUMENTS
  + Argument ("input", "input image to be smoothed.").type_image_in ()
  + Argument ("output", "the output image.").type_image_out ();

  OPTIONS
  + Option ("gaussian", "apply Gaussian smoothing with the specified width. "
            "This can be specified either as a single value to be used for all 3 axes, "
            "or as a comma-separated list of 3 values, one for each axis.")
  + Argument ("size").type_sequence_float()

  + Option ("extent", "specify extent of neighbourhood in voxels. "
            "This can be specified either as a single value to be used for all 3 axes, "
            "or as a comma-separated list of 3 values, one for each axis (default: 3x3x3).")
  + Argument ("size").type_sequence_int();
}





template <typename value_type> class GaussianFunctor
{
  public:
    GaussianFunctor (const std::vector<int>& extent, const std::vector<value_type>& width) {
      if (extent.size() == 1)
        dim[0] = dim[1] = dim[2] = extent[0];
      else {
        dim[0] = extent[0];
        dim[1] = extent[1];
        dim[2] = extent[2];
      }

      if (width.size() == 1)
        w[0] = w[1] = w[2] = width[0];
      else {
        w[0] = width[0];
        w[1] = width[1];
        w[2] = width[2];
      }
    }

    ssize_t extent (size_t axis) const {
      return (dim[axis]);
    }

    template <class Set>
    void prepare (Set& set, size_t x_axis, size_t y_axis, size_t z_axis) {
      ssize_t d [] = { (dim[x_axis]+1) /2, (dim[y_axis]+1) /2, (dim[z_axis]+1) /2 };
      float sd [] = {
        w[x_axis] > 0.0 ? set.vox (x_axis) / (M_SQRT2* w[x_axis]) : INFINITY,
        w[y_axis] > 0.0 ? set.vox (y_axis) / (M_SQRT2* w[y_axis]) : INFINITY,
        w[z_axis] > 0.0 ? set.vox (z_axis) / (M_SQRT2* w[z_axis]) : INFINITY
      };

      for (size_t j = 0; j < 3; ++j) {
        coefs[j].resize (d[j]);
        for (ssize_t i = 0; i < d[j]; ++i)
          coefs[j][i] = finite (sd[j]) ?
                        Math::exp (- Math::pow2 (i*sd[j])) :
                        (i == 0 ? 1.0 : 0.0);
      }
      value_type total = 0.0;
      for (ssize_t k = -d[2]+1; k < d[2]; ++k) {
        value_type a = 0.0;
        for (ssize_t j = -d[1]+1; j < d[1]; ++j) {
          value_type b = 0.0;
          for (ssize_t i = -d[0]+1; i < d[0]; ++i)
            b += coefs[0][i<0?-i:i];
          a += b * coefs[1][j<0?-j:j];
        }
        total += a * coefs[2][k<0?-k:k];
      }

      for (ssize_t i = 0; i < d[2]; ++i)
        coefs[2][i] /= total;
    }

    value_type operator() (const Image::Kernel::Data<value_type>& kernel) const {
      value_type retval = 0.0;
      for (ssize_t k = kernel.from (2); k < kernel.to (2); ++k) {
        value_type a = 0.0;
        for (ssize_t j = kernel.from (1); j < kernel.to (1); ++j) {
          value_type b = 0.0;
          for (ssize_t i = kernel.from (0); i < kernel.to (0); ++i)
            b += kernel (i,j,k) * coefs[0][i<0?-i:i];
          a += b * coefs[1][j<0?-j:j];
        }
        retval += a * coefs[2][k<0?-k:k];
      }
      return (retval);
    }

  private:
    ssize_t dim[3];
    value_type w[3];
    std::vector<value_type> coefs[3];
};






void run () {
  std::vector<int> extent (1);
  extent[0] = 3;

  Options opt = get_options ("extent");

  if (opt.size()) {
    extent = parse_ints (opt[0][0]);
    for (size_t i = 0; i < extent.size(); ++i)
      if (! (extent[i] & int (1)))
        throw Exception ("expected odd number for extent");
    if (extent.size() != 1 && extent.size() != 3)
      throw Exception ("unexpected number of elements specified in extent");
  }

  size_t num_conflicting_options = get_options ("gaussian").size();
  if (num_conflicting_options != 1)
    throw Exception ("a single type of smoothing must be supplied");

  Image::Header source (argument[0]);
  Image::Header destination (source);
  destination.create (argument[1]);

  Image::Data<float> src_data (source);
  Image::Data<float>::voxel_type src (src_data);

  Image::Data<float> dest_data (destination);
  Image::Data<float>::voxel_type dest (dest_data);

  opt = get_options ("gaussian");
  if (opt.size()) {
    std::vector<float> width = parse_floats (opt[0][0]);
    for (size_t i = 0; i < width.size(); ++i)
      if (width[i] < 0.0)
        throw Exception ("width values cannot be negative");
    if (width.size() != 1 && width.size() != 3)
      throw Exception ("unexpected number of elements specified in Gaussian width");

    Image::Kernel::run (dest, src, GaussianFunctor<float> (extent, width), "Gaussian smoothing...");
  }

}


