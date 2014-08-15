/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 2013.

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
#include "ptr.h"

#include "image/buffer.h"
#include "image/info.h"
#include "image/iterator.h"
#include "image/loop.h"
#include "image/nav.h"
#include "image/position.h"
#include "image/threaded_loop.h"
#include "image/voxel.h"

#include "dwi/tractography/ACT/act.h"


using namespace MR;
using namespace App;



void usage ()
{

	AUTHOR = "Robert E. Smith (r.smith@brain.org.au)";

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

};




class Modifier
{
  public:
    Modifier (Image::Buffer<float>& input_image, Image::Buffer<float>& output_image) :
        v_in  (input_image),
        v_out (output_image) { }

    Modifier (const Modifier& that) :
        v_in  (that.v_in),
        v_out (that.v_out)
    {
      for (size_t index = 0; index != 6; ++index) {
        if (that.buffers[index]) {
          buffers[index] = that.buffers[index];
          voxels [index] = new Image::Buffer<bool>::voxel_type (*buffers[index]);
        }
      }
    }

    void set_cgm_mask  (const std::string& path) { load (path, 0); }
    void set_sgm_mask  (const std::string& path) { load (path, 1); }
    void set_wm_mask   (const std::string& path) { load (path, 2); }
    void set_csf_mask  (const std::string& path) { load (path, 3); }
    void set_path_mask (const std::string& path) { load (path, 4); }
    void set_none_mask (const std::string& path) { load (path, 5); }


    bool operator() (const Image::Iterator& pos)
    {
      Image::Nav::set_pos (v_out, pos, 0, 3);
      bool voxel_nulled = false;

      if (buffers[5]) {
        Image::Nav::set_pos (*voxels[5], pos, 0, 3);
        if (voxels[5]->value()) {
          for (v_out[3] = 0; v_out[3] != 5; ++v_out[3])
            v_out.value() = 0.0;
          voxel_nulled = true;
        }
      }
      if (!voxel_nulled) {
        unsigned int count = 0;
        memset (values, 0x00, 5 * sizeof (float));
        for (size_t tissue = 0; tissue != 5; ++tissue) {
          if (buffers[tissue]) {
            Image::Nav::set_pos (*voxels[tissue], pos, 0, 3);
            if (voxels[tissue]->value()) {
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
          for (v_out[3] = 0; v_out[3] != 5; ++v_out[3])
            v_out.value() = values[v_out[3]];
        } else {
          Image::Nav::set_pos (v_in, pos, 0, 3);
          for (v_in[3] = v_out[3] = 0; v_out[3] != 5; ++v_in[3], ++v_out[3])
            v_out.value() = v_in.value();
        }
      }

      return true;
    }


  private:
    Image::Buffer<float>::voxel_type v_in, v_out;
    RefPtr< Image::Buffer<bool> > buffers[6];
    Ptr< Image::Buffer<bool>::voxel_type > voxels[6];
    float values[5];

    void load (const std::string& path, const size_t index)
    {
      assert (index <= 5);
      buffers[index] = new Image::Buffer<bool> (path);
      if (!Image::dimensions_match (v_in, *buffers[index], 0, 3))
        throw Exception ("Image " + str(path) + " does not match 5TT image dimensions");
      voxels[index] = new Image::Buffer<bool>::voxel_type (*buffers[index]);
    }

};




void run ()
{

  Image::Header H (argument[0]);
  DWI::Tractography::ACT::verify_5TT_image (H);
  Image::Buffer<float> in (H);
  Image::Buffer<float> out (argument[1], H);

  Modifier modifier (in, out);

  Options
  opt = get_options ("cgm");  if (opt.size()) modifier.set_cgm_mask  (opt[0][0]);
  opt = get_options ("sgm");  if (opt.size()) modifier.set_sgm_mask  (opt[0][0]);
  opt = get_options ("wm");   if (opt.size()) modifier.set_wm_mask   (opt[0][0]);
  opt = get_options ("csf");  if (opt.size()) modifier.set_csf_mask  (opt[0][0]);
  opt = get_options ("path"); if (opt.size()) modifier.set_path_mask (opt[0][0]);
  opt = get_options ("none"); if (opt.size()) modifier.set_none_mask (opt[0][0]);

  Image::ThreadedLoop loop ("Modifying ACT 5TT image... ", H, 2, 0, 3);
  loop.run (modifier);

}

