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
#include "dataset/interp/nearest.h"
#include "dataset/interp/linear.h"
#include "dataset/interp/cubic.h"
#include "dataset/misc.h"
#include "dataset/loop.h"
#include "dataset/copy.h"


using namespace std; 
using namespace MR; 

SET_VERSION_DEFAULT;

DESCRIPTION = {
 "apply spatial transformations or reslice images.",
  NULL
};

ARGUMENTS = {
  Argument ("input", "input image", "input image to be transformed.").type_image_in (),
  Argument ("output", "output image", "the output image.").type_image_out (),
  Argument::End
};


const char* interp_choices[] = { "NEAREST", "LINEAR", "CUBIC" };

OPTIONS = {

  Option ("transform", "the transform to use", "specified the 4x4 transform to apply.")
    .append (Argument ("transform", "transform", "the transform to apply, in the form of a 4x4 ascii file.").type_file ()),

  Option ("replace", "replace transform", "replace the current transform by that specified, rather than applying it to the current transform."),

  Option ("inverse", "use inverse transform", "invert the specified transform before using it."),

  Option ("template", "template image", "reslice the input image to match the specified template image.")
    .append (Argument ("image", "image", "the template image.").type_image_in ()),

  Option ("reference", "reference image for transform", "in case the transform supplied maps from the input image onto a reference image, use this option to specify the reference. Noe that this implicitly sets the -replace option.")
    .append (Argument ("image", "image", "the reference image.").type_image_in ()),

  Option ("flipx", "assume x-flipped transform", "assume the transform is supplied assuming a coordinate system with the x-axis reversed relative to the MRtrix convention (i.e. x increases from right to left). This is required to handle transform matrices produced by FSL's FLIRT command. This is only used in conjunction with the -reference option."),

  Option ("interp", "interpolation method", "set the interpolation method. Valid choices are nearest, linear and cubic (default is linear).")
    .append (Argument ("method", "method", "the interpolation method").type_choice (interp_choices)),

  Option::End 
};







template <template <class T> class Interp, class Set> class ResliceKernel 
{
  public:
    ResliceKernel (Set& source_image, const float* transform) : vox (source_image), src (vox), R (transform) { }

    void operator() (Set& dest) { 
      Point pos (
          R[0]*dest[0] + R[1]*dest[1] + R[2]*dest[2] + R[3],
          R[4]*dest[0] + R[5]*dest[1] + R[6]*dest[2] + R[7],
          R[8]*dest[0] + R[9]*dest[1] + R[10]*dest[2] + R[11]);
      src.voxel (pos);
      if (!src) dest.value() = 0.0;
      else {
        for (size_t i = 3; i < vox.ndim(); ++i)
          vox[i] = dest[i];
        dest.value() = src.value();
      }
    }

    static void reslice (Set& dest_image, Set& source_image, const float* transform) {
      ResliceKernel<Interp,Set> kernel (source_image, transform);
      DataSet::loop1 ("reslicing image...", kernel, dest_image);
    }

  private:
      Set& vox;
      Interp<Set> src;
      const float* R;
};




EXECUTE {
  Math::Matrix<float> T(4,4);
  T.identity();
  bool transform_supplied = false;

  std::vector<OptBase> opt = get_options (0); // transform
  if (opt.size()) {
    transform_supplied = true;
    T.load (opt[0][0].get_string());
    if (T.rows() != 4 || T.columns() != 4) 
      throw Exception (std::string("transform matrix supplied in file \"") + opt[0][0].get_string() + "\" is not 4x4");
  }

  bool replace = get_options(1).size(); // replace
  bool inverse = get_options(2).size(); // inverse

  Image::Header header_in (argument[0].get_image());
  Image::Header header (header_in);

  if (transform_supplied) {

    if (inverse) {
      Math::Matrix<float> I;
      Math::LU::inv (I, T);
      T.swap (I);
    }

    opt = get_options(4); // reference 
    if (opt.size() && transform_supplied) {
      replace = true;
      Image::Header ref_header (opt[0][0].get_image());

      if (get_options(5).size()) { // flipx 
        Math::Matrix<float> R(4,4);
        R.identity();
        R(0,0) = -1.0;
        R(0,3) = (ref_header.dim(0)-1) * ref_header.vox(0);

        Math::Matrix<float> M;
        Math::mult (M, R, T);

        R(0,3) = (header.dim(0)-1) * header.vox(0);

        Math::mult (T, M, R);
      }

      Math::Matrix<float> M;
      Math::mult (M, ref_header.transform(), T);
      T.swap (M);
    }



    if (replace) header.transform() = T;
    else {
      Math::Matrix<float> M;
      Math::mult (M, T, header.transform());
      header.transform() = M;
    }

    header.comments.push_back ("transform modified");
  }


  opt = get_options(3); // template : need to reslice
  if (opt.size()) {
    Math::Matrix<float> Mi(4,4);
    DataSet::Transform::scanner2voxel (Mi, header_in);

    Image::Header template_header (opt[0][0].get_image());
    header.axes[0].dim = template_header.axes[0].dim;
    header.axes[1].dim = template_header.axes[1].dim;
    header.axes[2].dim = template_header.axes[2].dim;
    header.axes[0].vox = template_header.axes[0].vox;
    header.axes[1].vox = template_header.axes[1].vox;
    header.axes[2].vox = template_header.axes[2].vox;
    header.transform() = template_header.transform();
    header.comments.push_back ("resliced to reference image \"" + template_header.name() + "\"");

    Math::Matrix<float> M, M2(4,4);
    DataSet::Transform::voxel2scanner (M2, template_header);
    Math::mult (M, Mi, M2);

    const float R[] = { 
      M(0,0), M(0,1), M(0,2), M(0,3), 
      M(1,0), M(1,1), M(1,2), M(1,3), 
      M(2,0), M(2,1), M(2,2), M(2,3)
    };

    Image::Voxel<float> in (header_in);
    const Image::Header header_out = argument[1].get_image (header);
    Image::Voxel<float> out (header_out);

    int interp = 1;
    opt = get_options (6); // interp
    if (opt.size()) interp = opt[0][0].get_int();

    switch (interp) {
      case 0: ResliceKernel<DataSet::Interp::Nearest, Image::Voxel<float> >::reslice (out, in, R); break;
      case 1: ResliceKernel<DataSet::Interp::Linear, Image::Voxel<float> >::reslice (out, in, R); break;
      case 2: ResliceKernel<DataSet::Interp::Cubic, Image::Voxel<float> >::reslice (out, in, R); break;
      default: assert (0);
    }
  }
  else {
    Image::Voxel<float> in (header_in);
    const Image::Header header_out = argument[1].get_image (header);
    Image::Voxel<float> out (header_out);
    DataSet::copy_with_progress (out, in);
  }
}

