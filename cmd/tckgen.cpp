/*
    Copyright 2009 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 25/10/09.

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
#include "image/voxel.h"
#include "dataset/interp/linear.h"
#include "dwi/tractography/exec.h"
#include "dwi/tractography/iFOD1.h"
#include "dwi/tractography/iFOD2.h"


using namespace MR; 

SET_VERSION_DEFAULT;

DESCRIPTION = {
  "perform streamlines tracking.",
  NULL
};

ARGUMENTS = {
  Argument ("FOD", "FOD image", "the image containing the FOD data, expressed in spherical harmonics.").type_image_in(),
  Argument ("tracks", "output tracks file", "the output file containing the tracks generated.").type_file(),
  Argument::End
};

const char* algorithms[] = { "ifod1", "ifod2", NULL };

OPTIONS = {
  Option ("algorithm", "algorithm", "specify the tractography algorithm to use (default: iFOD2).")
    .append (Argument ("name", "algorithm name", "the name of the algorithm to use. Valid choices are: iFOD1, iFOD2.").type_choice (algorithms)),

  Option ("seed", "seed region", "specify the seed region of interest.", AllowMultiple)
    .append (Argument ("spec", "ROI specification", "specifies the parameters necessary to define the ROI. This should be either the path to a binary mask image, or a comma-separated list of 4 floating-point values, specifying the [x,y,z] coordinates of the centre and radius of a spherical ROI.").type_string()),

  Option ("include", "inclusion ROI", "specify an inclusion region of interest, in the same format as the seed region. Only tracks that enter all such inclusion ROI will be produced.", Optional | AllowMultiple)
    .append (Argument ("spec", "ROI specification", "specifies the parameters necessary to define the ROI.").type_string()),

  Option ("exclude", "exclusion ROI", "specify an exclusion region of interest, in the same format as the seed region. Only tracks that enter any such exclusion ROI will be discarded.", Optional | AllowMultiple)
    .append (Argument ("spec", "ROI specification", "specifies the parameters necessary to define the ROI.").type_string()),

  Option ("mask", "mask ROI", "specify a mask region of interest, in the same format as the seed region. Tracks will be terminated when they leave any such ROI.", Optional | AllowMultiple)
    .append (Argument ("spec", "ROI specification", "specifies the parameters necessary to define the ROI.").type_string()),

  Option ("step", "step size", "set the step size of the algorithm.")
    .append (Argument ("size", "step size", "the step size to use in mm (default is 0.2 mm).").type_float (1e-6, 10.0, 0.2)),

  Option ("angle", "maximum deflection angle", "set the maximum angle between successive steps (default is 45°).")
    .append (Argument ("theta", "theta", "theta, the maximum angle.").type_float (1e-6, 90.0, 45.0)),

  Option ("number", "desired number of tracks", "set the desired number of tracks. The program will continue to generate tracks until this number of tracks have been selected and written to the output file (default is 100 for *_STREAM methods, 1000 for *_PROB methods).")
    .append (Argument ("tracks", "number of tracks", "the number of tracks.").type_integer (1, INT_MAX, 1)),

  Option ("maxnum", "maximum number of tracks to generate", "set the maximum number of tracks to generate. The program will not generate more tracks than this number, even if the desired number of tracks hasn't yet been reached (default is 100 x number).")
    .append (Argument ("tracks", "maximum number of tracks", "the maximum number of tracks.").type_integer (1, INT_MAX, 1)),

  Option ("length", "track length", "set the maximum length of any track.")
    .append (Argument ("value", "track distance", "the maximum length to use in mm (default is 200 mm).").type_float (1e-2, 1e6, 200.0)),

  Option ("minlength", "minimum track length", "set the minimum length of any track.")
    .append (Argument ("value", "track distance", "the minimum length to use in mm (default is 10 mm).").type_float (1e-2, 1e6, 10.0)),

  Option ("cutoff", "cutoff threshold", "set the FA or FOD amplitude cutoff for terminating tracks (default is 0.1).")
    .append (Argument ("value", "value", "the cutoff to use.").type_float (0, 1e6, 0.1)),

  Option ("initcutoff", "intial cutoff threshold", "set the minimum FA or FOD amplitude for initiating tracks (default is twice the normal cutoff).")
    .append (Argument ("value", "value", "the initial cutoff to use.").type_float (0, 1e6, 0.1)),

  Option ("trials", "number of trials", "set the maximum number of sampling trials at each point (only used for probabilistic tracking).")
    .append (Argument ("number", "number", "the number of trials.").type_integer(1, 10000, 50)),

  Option ("unidirectional", "unidirectional", "track from the seed point in one direction only (default is to track in both directions)."),

  Option ("initdirection", "initial direction", "specify an initial direction for the tracking.")
    .append (Argument ("dir", "direction", "the vector specifying the initial direction.").type_sequence_float()),

  Option ("noprecomputed", "no precomputation", "do NOT pre-compute legendre polynomial values. Warning: this will slow down the algorithm by a factor of approximately 4."),

  Option ("samples", "samples per step", "set the number of FOD samples to take per step for the 2nd order method (iFOD2).")
    .append (Argument ("number", "number", "the number of samples.").type_integer(1, 20, 3)),

  Option::End
};



EXECUTE {
  using namespace DWI::Tractography;

  Properties properties;
  properties["step_size"] = "0.2";
  properties["max_dist"] = "200";
  properties["min_dist"] = "10";
  properties["threshold"] = "0.1";
  properties["unidirectional"] = "0";
  properties["sh_precomputed"] = "1";

  int algorithm = 1;
  std::vector<OptBase> opt = get_options ("algorithm");
  if (opt.size()) algorithm = opt[0][0].get_int();

  opt = get_options ("seed");
  for (std::vector<OptBase>::iterator i = opt.begin(); i != opt.end(); ++i)
    properties.seed.add (ROI (std::string ((*i)[0].get_string())));

  opt = get_options ("include");
  for (std::vector<OptBase>::iterator i = opt.begin(); i != opt.end(); ++i)
    properties.include.add (ROI (std::string ((*i)[0].get_string())));

  opt = get_options ("exclude");
  for (std::vector<OptBase>::iterator i = opt.begin(); i != opt.end(); ++i)
    properties.exclude.add (ROI (std::string ((*i)[0].get_string())));

  opt = get_options ("mask");
  for (std::vector<OptBase>::iterator i = opt.begin(); i != opt.end(); ++i)
    properties.mask.add (ROI (std::string ((*i)[0].get_string())));

  opt = get_options ("step");
  if (opt.size()) properties["step_size"] = str (opt[0][0].get_float());

  opt = get_options ("angle");
  if (opt.size()) properties["max_angle"] = str (opt[0][0].get_float());

  opt = get_options ("number");
  if (opt.size()) properties["max_num_tracks"] = str (opt[0][0].get_int());

  opt = get_options ("maxnum");
  if (opt.size()) properties["max_num_attempts"] = str (opt[0][0].get_int());

  opt = get_options ("length");
  if (opt.size()) properties["max_dist"] = str (opt[0][0].get_float());

  opt = get_options ("min_length");
  if (opt.size()) properties["min_dist"] = str (opt[0][0].get_float());

  opt = get_options ("cutoff");
  if (opt.size()) properties["threshold"] = str (opt[0][0].get_float());

  opt = get_options ("initcutoff");
  if (opt.size()) properties["init_threshold"] = str (opt[0][0].get_float());

  opt = get_options ("trials");
  if (opt.size()) properties["max_trials"] = str (opt[0][0].get_int());

  opt = get_options ("unidirectional");
  if (opt.size()) properties["unidirectional"] = "1";

  opt = get_options ("initdirection");
  if (opt.size()) properties["init_direction"] = opt[0][0].get_string();

  opt = get_options ("noprecomputed");
  if (opt.size()) properties["sh_precomputed"] = "0";

  opt = get_options ("samples");
  if (opt.size()) properties["samples_per_step"] = str (opt[0][0].get_int());

  switch (algorithm) {
    case 0: Exec<iFOD1>::run (argument[0].get_image(), argument[1].get_string(), properties); break;
    case 1: Exec<iFOD2>::run (argument[0].get_image(), argument[1].get_string(), properties); break;
    default: assert (0);
  }
}
