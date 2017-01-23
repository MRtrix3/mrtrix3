/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */



#include "command.h"

#include "header.h"
#include "image.h"
#include "image_helpers.h"

#include "algo/iterator.h"

#include "dwi/tractography/ACT/act.h"


using namespace MR;
using namespace App;



void usage ()
{

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  DESCRIPTION
  + "manually set the partial volume fractions in an ACT five-tissue-type (5TT) image using mask images";

  ARGUMENTS
  + Argument ("input",  "the 5TT image to be modified").type_image_in()
  + Argument ("output", "the output modified 5TT image").type_image_out();

  OPTIONS
  + Option ("cgm",  "provide a mask of voxels that should be set to cortical grey matter")
    + Argument ("image").type_image_in()

  + Option ("sgm",  "provide a mask of voxels that should be set to sub-cortical grey matter")
    + Argument ("image").type_image_in()

  + Option ("wm",   "provide a mask of voxels that should be set to white matter")
    + Argument ("image").type_image_in()

  + Option ("csf",  "provide a mask of voxels that should be set to CSF")
    + Argument ("image").type_image_in()

  + Option ("path", "provide a mask of voxels that should be set to pathological tissue")
    + Argument ("image").type_image_in()

  + Option ("none", "provide a mask of voxels that should be cleared (i.e. are non-brain); note that this will supersede all other provided masks")
    + Argument ("image").type_image_in();

}




class Modifier
{ MEMALIGN(Modifier)
  public:
    Modifier (Image<float>& input_image, Image<float>& output_image) :
        v_in  (input_image),
        v_out (output_image) { }

    void set_cgm_mask  (const std::string& path) { load (path, 0); }
    void set_sgm_mask  (const std::string& path) { load (path, 1); }
    void set_wm_mask   (const std::string& path) { load (path, 2); }
    void set_csf_mask  (const std::string& path) { load (path, 3); }
    void set_path_mask (const std::string& path) { load (path, 4); }
    void set_none_mask (const std::string& path) { load (path, 5); }


    bool operator() (const Iterator& pos)
    {
      assign_pos_of (pos, 0, 3).to (v_out);
      bool voxel_nulled = false;
      if (buffers[5].valid()) {
        assign_pos_of (pos, 0, 3).to (buffers[5]);
        if (buffers[5].value()) {
          for (auto i = Loop(3) (v_out); i; ++i)
            v_out.value() = 0.0;
          voxel_nulled = true;
        }
      }
      if (!voxel_nulled) {
        unsigned int count = 0;
        float values[5] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
        for (size_t tissue = 0; tissue != 5; ++tissue) {
          if (buffers[tissue].valid()) {
            assign_pos_of (pos, 0, 3).to (buffers[tissue]);
            if (buffers[tissue].value()) {
              ++count;
              values[tissue] = 1.0;
            }
          }
        }
        if (count) {
          if (count > 1) {
            const float multiplier = 1.0 / float(count);
            for (size_t tissue = 0; tissue != 5; ++tissue)
              values[tissue] *= multiplier;
          }
          for (auto i = Loop(3) (v_out); i; ++i)
            v_out.value() = values[v_out.index(3)];
        } else {
          assign_pos_of (pos, 0, 3).to (v_in);
          for (auto i = Loop(3) (v_in, v_out); i; ++i)
            v_out.value() = v_in.value();
        }
      }
      return true;
    }


  private:
    Image<float> v_in, v_out;
    Image<bool> buffers[6];

    void load (const std::string& path, const size_t index)
    {
      assert (index <= 5);
      buffers[index] = Image<bool>::open (path);
      if (!dimensions_match (v_in, buffers[index], 0, 3))
        throw Exception ("Image " + str(path) + " does not match 5TT image dimensions");
    }

};




void run ()
{

  auto in = Image<float>::open (argument[0]);
  DWI::Tractography::ACT::verify_5TT_image (in);
  auto out = Image<float>::create (argument[1], in);

  Modifier modifier (in, out);

  auto
  opt = get_options ("cgm");  if (opt.size()) modifier.set_cgm_mask  (opt[0][0]);
  opt = get_options ("sgm");  if (opt.size()) modifier.set_sgm_mask  (opt[0][0]);
  opt = get_options ("wm");   if (opt.size()) modifier.set_wm_mask   (opt[0][0]);
  opt = get_options ("csf");  if (opt.size()) modifier.set_csf_mask  (opt[0][0]);
  opt = get_options ("path"); if (opt.size()) modifier.set_path_mask (opt[0][0]);
  opt = get_options ("none"); if (opt.size()) modifier.set_none_mask (opt[0][0]);

  ThreadedLoop ("Modifying ACT 5TT image", in, 0, 3, 2).run (modifier);

}

