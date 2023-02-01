/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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

#include "dwi/tractography/algorithms/ballsticks.h"

#include "image_helpers.h"
#include "types.h"
#include "file/nifti_utils.h"
#include "file/path.h"
#include "math/sphere.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Algorithms
      {



        using namespace App;


        const char* const seed_fixel_options[] { "first", "largest", "proportional", "random_fixel", "random_dir", nullptr };

        const OptionGroup BallSticksOptions = OptionGroup ("Options specific to the BallSticks tracking algorithms")

        + Option ("seed_fixel", "control which fixel is chosen for commencement of tracking from a given seed location; "
                                "options are: " + join (seed_fixel_options, ", "))
          + Argument ("choice").type_choice (seed_fixel_options);


        void load_ballsticks_options (Tractography::Properties& properties)
        {
          auto opt = get_options ("seed_fixel");
          if (opt.size())
            properties["seed_fixel"] = str<float> (opt[0][0]);
        }






        BallSticks::Shared::Shared (const std::string& diff_path, DWI::Tractography::Properties& property_set) :
            SharedBase (std::string(), property_set),
            num_fixels (0),
            seed_fixel (BallSticks::default_seed_fixel)
        {
          if (is_act() && act().backtrack())
            throw Exception ("Backtracking not available for BallSticks algorithm");

          if (rk4)
            throw Exception ("4th-order Runge-Kutta integration not valid for BallSticks algorithm");

          properties["method"] = "BallSticks";
          properties["source"] = diff_path;

          if (!Path::is_dir (diff_path))
            throw Exception ("BallSticks algorithm expects a directory path as input");

          Path::Dir dir (diff_path);
          std::set<std::string> filenames;
          std::string filename;

          const std::string merged_prefix ("merged_");
          const std::string samples_suffix ("samples.nii.gz");
          const vector<std::string> parameter_order { "ph", "th", "f" };

          while ((filename = dir.read_name()).size()) {
            if (filename.substr(0, merged_prefix.size()) == merged_prefix &&
                filename.substr(filename.size() - samples_suffix.size()) == samples_suffix)
              filenames.insert (filename);
          }
          if (filenames.size() % 3)
            throw Exception ("Unexpected number of bedpostx files of interest found; "
                             "total should be a multiple of 3 (" + join(parameter_order, ", ") + " per stick), but found " + str(filenames.size()) + " matches");
          num_fixels = filenames.size() / 3;
          vector<std::string> ordered_filenames;
          for (size_t fixel_index = 1; fixel_index <= num_fixels; ++fixel_index) {
            for (size_t parameter_index = 0; parameter_index != 3; ++parameter_index) {
              filename = merged_prefix + parameter_order[parameter_index] + str(fixel_index) + samples_suffix;
              if (!Path::is_file (Path::join (diff_path, filename)))
                throw Exception ("Unable to find expected image \"" + filename + "\" in bedpostx directory \"" + diff_path + "\"");
              ordered_filenames.push_back (filename);
            }
          }
          DEBUG("Total of " + str(filenames.size()) + " relevant images found and sorted in \"" + diff_path + "\": [ " + join(ordered_filenames, ", ") + " ]");

          Header H_scratch (Header::open (Path::join (diff_path, ordered_filenames.front())));
          const size_t num_realisations = H_scratch.size(3);
          H_scratch.ndim() = 5;
          H_scratch.size(3) = 4 * num_fixels;
          H_scratch.size(4) = num_realisations;
          H_scratch.stride(0) = 3;
          H_scratch.stride(1) = 4;
          H_scratch.stride(2) = 5;
          H_scratch.stride(3) = 1;
          H_scratch.stride(4) = 2;
          source = Image<float>::scratch (H_scratch, "Converted bedpostx realisations");

          {
            // For now, just open all of these images at once, and load all of the data at once;
            //   in the future, it may be preferable to load data from the inputs one image / image pair at a time,
            //   since they are typically stored compressed and therefore can't be memory-mapped,
            //   and this would therefore double command RAM requirements during initialisation
            vector<Image<float>> ordered_images;
            for (const auto& i : ordered_filenames) {
              ordered_images.emplace_back (Image<float>::open (Path::join (diff_path, i)));
              check_voxel_grids_match_in_scanner_space (ordered_images.front(), ordered_images.back());
            }

            class Functor
            {
              public:
                using transform_type = Eigen::Transform<float, 3, Eigen::AffineCompact>;
                Functor (const Image<float>& scratch, const vector<Image<float>>& in) :
                    num_fixels (scratch.size (3) / 4),
                    num_realisations (scratch.size (4)),
                    transform (scratch.transform().linear().cast<float>()),
                    output (scratch)
                {
                  for (const auto& i : in)
                    inputs.emplace_back (Image<float> (i));
                  // bvecs format actually assumes a LHS coordinate system even if image is
                  // stored using RHS - i axis is flipped to make linear 3x3 part of
                  // transform have negative determinant:
                  vector<size_t> order;
                  auto adjusted_transform = File::NIfTI::adjust_transform (output, order);
                  if (adjusted_transform.linear().determinant() > 0.0) {
                    transform_type i_flip (transform_type::Identity());
                    i_flip (0, 0) = -1.0;
                    transform = i_flip * transform;
                  }
                }

                Functor (const Functor& that) :
                    num_fixels (that.num_fixels),
                    num_realisations (that.num_realisations),
                    transform (that.transform),
                    output (that.output)
                {
                  for (auto& i : that.inputs)
                    inputs.emplace_back (Image<float> (i));
                }

                bool operator() (const Image<float>& pos)
                {
                  assign_pos_of (pos, 0, 3).to (output);
                  for (auto& i : inputs)
                    assign_pos_of (pos, 0, 3).to (i);
                  Eigen::Matrix<float, 2, 1> phi_theta;
                  Eigen::Matrix<float, 3, 1> xyz;
                  for (size_t realisation_index = 0; realisation_index != num_realisations; ++realisation_index) {
                    output.index (4) = realisation_index;
                    for (auto& i : inputs)
                      i.index (3) = realisation_index;
                    for (size_t fixel_index = 0; fixel_index != num_fixels; ++fixel_index) {
                      phi_theta[0]  = inputs[3*fixel_index    ].value();
                      phi_theta[1]  = inputs[3*fixel_index + 1].value();
                      const float f = inputs[3*fixel_index + 2].value();
                      Math::Sphere::spherical2cartesian (phi_theta, xyz);
                      xyz = transform * xyz;
                      output.index(3) = 4*fixel_index;     output.value() = xyz[0];
                      output.index(3) = 4*fixel_index + 1; output.value() = xyz[1];
                      output.index(3) = 4*fixel_index + 2; output.value() = xyz[2];
                      output.index(3) = 4*fixel_index + 3; output.value() = f;
                    }
                  }
                  return true;
                }

              protected:
                const size_t num_fixels;
                const size_t num_realisations;
                transform_type transform;
                Image<float> output;
                vector<Image<float>> inputs;

            };

            ThreadedLoop("Importing and converting bedpostx derivatives", source, 0, 3)
            .run (Functor (source, ordered_images), source);

          }

          set_step_and_angle (Tracking::Defaults::stepsize_voxels_firstorder,
                              Tracking::Defaults::angle_deterministic,
                              false);
          set_cutoff (Tracking::Defaults::cutoff_volfrac);
          set_num_points();

          auto it_seed_fixel = properties.find ("seed_fixel");
          if (it_seed_fixel != properties.end()) {
            // If one were to use booth of these options, it's ambiguous as to whether:
            // - -seed_fixel chooses a fixel, then -seed_direction sets the sign and
            //   flags whether or not the curvature constraint is violated;
            // or:
            // - -seed_direction sets the set of fixels that can plausibly be selected from,
            //   and -seed_fixel then decides how to select from that set
            if (init_dir.allFinite())
              throw Exception ("Cannot use both -seed_direction and -seed_fixel");
            if (it_seed_fixel->second == "first")
              seed_fixel = seed_fixel_t::FIRST;
            else if (it_seed_fixel->second == "largest")
              seed_fixel = seed_fixel_t::LARGEST;
            else if (it_seed_fixel->second == "proportional")
              seed_fixel = seed_fixel_t::PROPORTIONAL;
            else if (it_seed_fixel->second == "random_fixel")
              seed_fixel = seed_fixel_t::RANDOM_FIXEL;
            else if (it_seed_fixel->second == "random_dir")
              seed_fixel = seed_fixel_t::RANDOM_DIR;
            else
              throw Exception ("Unexpected value \"" + it_seed_fixel->second + "\" for seed fixel selection");
          }

        }



      }
    }
  }
}
