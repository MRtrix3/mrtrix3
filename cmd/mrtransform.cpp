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
#include "image/interp.h"
#include "image/misc.h"
#include "math/LU.h"

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

  Option::End 
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

  Image::Object& in_obj (*argument[0].get_image());
  Image::Header header (in_obj);

  if (transform_supplied) {

    if (inverse) {
      Math::Matrix<float> I;
      Math::LU::inv (I, T);
      T.swap (I);
    }

    opt = get_options(4); // reference 
    if (opt.size() && transform_supplied) {
      replace = true;
      const Image::Header& ref_header (opt[0][0].get_image()->header());

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



    if (replace) header.transform_matrix = T;
    else {
      Math::Matrix<float> M;
      Math::mult (M, T, header.transform());
      header.transform_matrix.swap (M);
    }

    header.comments.push_back ("transform modified");
  }


  opt = get_options(3); // template : need to reslice
  if (opt.size()) {
    Math::Matrix<float> Mi(4,4);
    Image::Transform::R2P (Mi, in_obj);

    Image::Header template_header (opt[0][0].get_image()->header());
    header.axes[0].dim = template_header.axes[0].dim;
    header.axes[1].dim = template_header.axes[1].dim;
    header.axes[2].dim = template_header.axes[2].dim;
    header.axes[0].vox = template_header.axes[0].vox;
    header.axes[1].vox = template_header.axes[1].vox;
    header.axes[2].vox = template_header.axes[2].vox;
    header.transform_matrix = template_header.transform();
    header.comments.push_back ("resliced to reference image \"" + template_header.name + "\"");

    Math::Matrix<float> M, M2(4,4);
    Image::Transform::P2R (M2, in_obj);
    Math::mult (M, Mi, M2);

    float R[] = { 
      M(0,0), M(0,1), M(0,2), M(0,3), 
      M(1,0), M(1,1), M(1,2), M(1,3), 
      M(2,0), M(2,1), M(2,2), M(2,3)
    };

    Image::Voxel in_vox (in_obj);
    Image::Interp<Image::Voxel> in (in_vox);
    Image::Voxel out (*argument[1].get_image (header));
    Point pos;

    in_obj.map();
    out.image.map();

    ProgressBar::init (voxel_count(out), "reslicing image...");
    do { 
      for (out[2] = 0; out[2] < out.dim(2); out[2]++) {
        for (out[1] = 0; out[1] < out.dim(1); out[1]++) {
          pos[0] = R[1]*out[1] + R[2]*out[2] + R[3];
          pos[1] = R[5]*out[1] + R[6]*out[2] + R[7];
          pos[2] = R[9]*out[1] + R[10]*out[2] + R[11];
          for (out[0] = 0; out[0] < out.dim(0); out[0]++) {
            in.P (pos);
            out.value() = !in ? 0.0 : in.value();
            pos[0] += R[0];
            pos[1] += R[4];
            pos[2] += R[8];
            ProgressBar::inc();
          }
        }
      }
    } while (out++);
    ProgressBar::done();
  }
  else {
    Image::Voxel in (in_obj);
    Image::Voxel out (*argument[1].get_image (header));
    ProgressBar::init (voxel_count(out), "copying image data...");
    do { 
      out.value() = in.value();
      in++;
      ProgressBar::inc();
    } while (out++);
    ProgressBar::done();
  }
}

