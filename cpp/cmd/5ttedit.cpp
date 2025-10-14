/* Copyright (c) 2008-2025 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#include "command.h"

#include "header.h"
#include "image.h"
#include "image_helpers.h"

#include "algo/iterator.h"

#include "dwi/tractography/ACT/act.h"

using namespace MR;
using namespace App;

// clang-format off
void usage() {

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Manually set the partial volume fractions"
             " in an ACT five-tissue-type (5TT) image";

  DESCRIPTION
  + "The user-provided images are interpreted as desired partial volume fractions "
    "in the output image. For any voxel with a non-zero value in such an image, this "
    "will be the value for that tissue in the output 5TT image. For tissues for which "
    "the user has not explicitly requested a change to the partial volume fraction, "
    "the partial volume fraction may nevertheless change in order to preserve the "
    "requirement of a unity sum of partial volume fractions in each voxel."

  + "Any voxels that are included in the mask provided by the -none option will "
    "be erased in the output image; this supersedes all other user inputs.";

  ARGUMENTS
  + Argument ("input",  "the 5TT image to be modified").type_image_in()
  + Argument ("output", "the output modified 5TT image").type_image_out();

  OPTIONS
  + Option ("cgm", "provide an image of new cortical grey matter partial volumes fractions")
    + Argument ("image").type_image_in()

  + Option ("sgm", "provide an image of new sub-cortical grey matter partial volume fractions")
    + Argument ("image").type_image_in()

  + Option ("wm", "provide an image of new white matter partial volume fractions")
    + Argument ("image").type_image_in()

  + Option ("csf", "provide an image of new CSF partial volume fractions")
    + Argument ("image").type_image_in()

  + Option ("path", "provide an image of new pathological tissue partial volume fractions")
    + Argument ("image").type_image_in()

  + Option ("none", "provide a mask of voxels that should be cleared"
                    " (i.e. are non-brain)")
    + Argument ("image").type_image_in();

}
// clang-format on

class Modifier {
public:
  Modifier(Image<float> &input_image, Image<float> &output_image)
      : v_in(input_image), v_out(output_image), excess_volume_count(0), inadequate_volume_count(0) {}

  ~Modifier() {
    if (excess_volume_count) {
      WARN("A total of " + str(excess_volume_count) + " voxels" +                                  //
           " had a sum of partial volume fractions across user-provided images greater than one" + //
           " (these were auto-scaled to sum to one," +                                             //
           " but there may have been an error in generation of input images)");                    //
    }
    if (inadequate_volume_count) {
      WARN("A total of " + str(excess_volume_count) + " voxels were outside the brain in the input image," + //
           " the user provided non-zero partial volume fractions in at least one input volume," +            //
           " but the sum of partial volume fractions across user-provided images was less than one" +        //
           " (these were auto-scaled to sum to one," +                                                       //
           " but there may have been an error in generation of input images)");                              //
    }
  }

  void set_cgm_input(const std::string &path) { load(path, 0); }
  void set_sgm_input(const std::string &path) { load(path, 1); }
  void set_wm_input(const std::string &path) { load(path, 2); }
  void set_csf_input(const std::string &path) { load(path, 3); }
  void set_path_input(const std::string &path) { load(path, 4); }

  void set_none_mask(const std::string &path) {
    none = Image<bool>::open(path);
    if (!dimensions_match(v_in, none, 0, 3))
      throw Exception("Image " + str(path) + " does not match 5TT image dimensions");
  }

  bool operator()(const Iterator &pos);

private:
  Image<float> v_in, v_out;
  Image<float> buffers[5];
  Image<bool> none;
  size_t excess_volume_count;
  size_t inadequate_volume_count;

  void load(const std::string &path, const size_t index) {
    assert(index <= 4);
    buffers[index] = Image<float>::open(path);
    if (!dimensions_match(v_in, buffers[index], 0, 3))
      throw Exception("Image " + str(path) + " does not match 5TT image dimensions");
  }
};

bool Modifier::operator()(const Iterator &pos) {
  assign_pos_of(pos, 0, 3).to(v_in, v_out);
  bool voxel_nulled = false;
  if (none.valid()) {
    assign_pos_of(pos, 0, 3).to(none);
    if (none.value()) {
      for (auto i = Loop(3)(v_out); i; ++i)
        v_out.value() = 0.0;
      voxel_nulled = true;
    }
  }
  if (!voxel_nulled) {
    default_type sum_user = 0.0;
    for (size_t tissue = 0; tissue != 5; ++tissue) {
      if (buffers[tissue].valid()) {
        assign_pos_of(pos, 0, 3).to(buffers[tissue]);
        const float value = buffers[tissue].value();
        if (value < 0.0)
          throw Exception("Invalid negative value found in image \"" + buffers[tissue].name() + "\"");
        if (value > 1.0)
          throw Exception("Invalid value greater than zero found in image \"" + buffers[tissue].name() + "\"");
        sum_user += value;
      }
    }
    if (sum_user) {
      if (float(sum_user) > 1.0f) {
        // Erroneous input from user;
        //   we can rescale so that the sum of partial volume fractions is one,
        //   but we should also warn the user about the bad input
        for (auto i = Loop(3)(v_out); i; ++i)
          v_out.value() = buffers[v_out.index(3)].valid() ? buffers[v_out.index(3)].value() / sum_user : 0.0;
        ++excess_volume_count;
      } else {
        // Total of residual tissue fractions ignoring what is being explicitly modified
        default_type sum_unmodified = 0.0;
        for (auto i = Loop(3)(v_in); i; ++i) {
          if (!buffers[v_in.index(3)].valid() || !buffers[v_in.index(3)].value())
            sum_unmodified += v_in.value();
        }
        default_type multiplier = std::numeric_limits<default_type>::quiet_NaN();
        if (sum_unmodified) {
          // Scale all of these so that after the requested tissue overrides,
          //   inclusion of the unmodified tissues still results in a partial volume sum of one
          multiplier = sum_unmodified ? ((1.0 - sum_user) / sum_unmodified) : 0.0;
          for (auto i = Loop(3)(v_in, v_out); i; ++i)
            v_out.value() = (buffers[v_in.index(3)].valid() && buffers[v_in.index(3)].value())
                                ? buffers[v_in.index(3)].value()
                                : multiplier * v_in.value();
        } else {
          // Voxel is zero-filled in input image;
          //   ideally the user will have provided a unity sum of volume fractions as their input
          multiplier = 1.0f;
          if (float(sum_user) < 1.0f) {
            multiplier = 1.0 / sum_user;
            ++inadequate_volume_count;
          }
          for (auto i = Loop(3)(v_out); i; ++i)
            v_out.value() = buffers[v_out.index(3)].valid() ? (buffers[v_out.index(3)].value() * multiplier) : 0.0;
        }
#ifndef NDEBUG
        default_type sum_result = 0.0;
        for (auto i = Loop(3)(v_out); i; ++i)
          sum_result += v_out.value();
        if (std::abs(sum_result - 1.0) > 1e-5) {
          std::cerr << "[" << str(pos.index(0)) << "," << str(pos.index(1)) << "," << str(pos.index(2)) << "]\n";
          std::cerr << "Input image values: ";
          for (auto i = Loop(3)(v_in); i; ++i)
            std::cerr << str(v_in.value()) << " ";
          std::cerr << "\n";
          std::cerr << "User modification values: ";
          for (size_t tissue = 0; tissue != 5; ++tissue) {
            if (buffers[tissue].valid()) {
              std::cerr << str<float>(buffers[tissue].value()) << " ";
            } else {
              std::cerr << "<> ";
            }
          }
          std::cerr << "\n";
          std::cerr << "Output image values: ";
          for (auto i = Loop(3)(v_out); i; ++i)
            std::cerr << str(v_out.value()) << " ";
          std::cerr << "\n";
          std::cerr << "sum_user=" << str<float>(sum_user) << "; sum_unmodified=" << str<float>(sum_unmodified)
                    << "; multiplier=" << str<float>(multiplier) << "\n";
        }
#endif
      }
    } else {
      for (auto i = Loop(3)(v_in, v_out); i; ++i)
        v_out.value() = v_in.value();
    }
  }
  return true;
}

void run() {

  auto in = Image<float>::open(argument[0]);
  DWI::Tractography::ACT::verify_5TT_image(in);
  auto out = Image<float>::create(argument[1], in);

  Modifier modifier(in, out);

  auto opt = get_options("cgm");
  if (!opt.empty())
    modifier.set_cgm_input(opt[0][0]);
  opt = get_options("sgm");
  if (!opt.empty())
    modifier.set_sgm_input(opt[0][0]);
  opt = get_options("wm");
  if (!opt.empty())
    modifier.set_wm_input(opt[0][0]);
  opt = get_options("csf");
  if (!opt.empty())
    modifier.set_csf_input(opt[0][0]);
  opt = get_options("path");
  if (!opt.empty())
    modifier.set_path_input(opt[0][0]);
  opt = get_options("none");
  if (!opt.empty())
    modifier.set_none_mask(opt[0][0]);

  ThreadedLoop("Modifying ACT 5TT image", in, 0, 3, 2).run(modifier);
}
