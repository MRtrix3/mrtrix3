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


#include "command.h"

#include "image/buffer.h"
#include "image/header.h"
#include "image/loop.h"
#include "image/voxel.h"

#include "dwi/tractography/ACT/act.h"
#include "dwi/tractography/ACT/tissues.h"


using namespace MR;
using namespace App;



#define VALUE_DEFAULT_BG   0.0
#define VALUE_DEFAULT_CGM  0.5
#define VALUE_DEFAULT_SGM  0.75
#define VALUE_DEFAULT_WM   1.0
#define VALUE_DEFAULT_CSF  0.15
#define VALUE_DEFAULT_PATH 2.0



void usage ()
{

	AUTHOR = "Robert E. Smith (r.smith@brain.org.au)";

  DESCRIPTION
  + "generate an image for visualisation purposes from an ACT 5TT segmented anatomical image";

  ARGUMENTS
  + Argument ("input",  "the input 4D tissue-segmented image").type_image_in()
  + Argument ("output", "the output 3D image for visualisation").type_image_out();

  OPTIONS

  + Option ("bg",   "image intensity of background")
    + Argument ("value").type_float (0.0, VALUE_DEFAULT_BG,   1.0)

  + Option ("cgm",  "image intensity of cortical grey matter")
    + Argument ("value").type_float (0.0, VALUE_DEFAULT_CGM,  1.0)

  + Option ("sgm",  "image intensity of sub-cortical grey matter")
    + Argument ("value").type_float (0.0, VALUE_DEFAULT_SGM,  1.0)

  + Option ("wm",   "image intensity of white matter")
    + Argument ("value").type_float (0.0, VALUE_DEFAULT_WM,   1.0)

  + Option ("csf",  "image intensity of CSF")
    + Argument ("value").type_float (0.0, VALUE_DEFAULT_CSF,  1.0)

  + Option ("path", "image intensity of pathological tissue")
    + Argument ("value").type_float (0.0, VALUE_DEFAULT_PATH, 1.0);

};







void run ()
{

  Image::Header H_in (argument[0]);
  DWI::Tractography::ACT::verify_5TT_image (H_in);
  Image::Buffer<float> in (H_in);

  Image::Header H_out (in);
  H_out.set_ndim (3);
  H_out.datatype() = DataType::Float32;

  Options
  opt = get_options ("bg");   const float bg_multiplier   = opt.size() ? opt[0][0] : VALUE_DEFAULT_BG;
  opt = get_options ("cgm");  const float cgm_multiplier  = opt.size() ? opt[0][0] : VALUE_DEFAULT_CGM;
  opt = get_options ("sgm");  const float sgm_multiplier  = opt.size() ? opt[0][0] : VALUE_DEFAULT_SGM;
  opt = get_options ("wm");   const float wm_multiplier   = opt.size() ? opt[0][0] : VALUE_DEFAULT_WM;
  opt = get_options ("csf");  const float csf_multiplier  = opt.size() ? opt[0][0] : VALUE_DEFAULT_CSF;
  opt = get_options ("path"); const float path_multiplier = opt.size() ? opt[0][0] : VALUE_DEFAULT_PATH;

  Image::Buffer<float> out (argument[1], H_out);
  auto v_in = in.voxel();
  auto v_out = out.voxel();

  auto f = [&] (decltype(v_in)& in, decltype(v_out)& out) {
    const DWI::Tractography::ACT::Tissues t (in);
    const float bg = 1.0 - (t.get_cgm() + t.get_sgm() + t.get_wm() + t.get_csf() + t.get_path());
    out.value() = (bg_multiplier * bg) + (cgm_multiplier * t.get_cgm()) + (sgm_multiplier * t.get_sgm()) 
      + (wm_multiplier * t.get_wm()) + (csf_multiplier * t.get_csf()) + (path_multiplier * t.get_path());
  };
  Image::ThreadedLoop (v_out, 0, 3).run (f, v_in, v_out);

}

