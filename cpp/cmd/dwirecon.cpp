/* Copyright (c) 2008-2024 the MRtrix3 contributors.
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

#include <limits>
#include <string>
#include <vector>

#include "adapter/gradient1D.h"
#include "command.h"
#include "dwi/gradient.h"
#include "dwi/shells.h"
#include "filter/smooth.h"
#include "header.h"
#include "image.h"
#include "image_helpers.h"
#include "math/SH.h"
#include "math/sphere.h"
#include "metadata/phase_encoding.h"
#include "progressbar.h"

using namespace MR;
using namespace App;

// Other future operations that might be applicable here:
// - "leave-one-out": Predict each intensity based on all observations excluding that one
// - SHARD recon
const std::vector<std::string> operations = {"combine_pairs", "combine_predicted"};
constexpr default_type default_combinepredicted_exponent = 1.0;

// clang-format off
const OptionGroup common_options = OptionGroup("Options common to multiple operations")
  + Option("field", "provide a B0 field offset image in Hz")
      + Argument("image").type_image_in();
  // TODO Another option that may be applicable to multiple operations
  //   (though only "combine_predicted" for now):
  //   weighted SH fit,
  //   with weights determined by proximity to any given observation

//const OptionGroup combinepairs_options = OptionGroup("Options specific to \"combine_pairs\" operation")
// TODO Give capability to provide this information from the command-line,
    //   rather than relying on internal heuristics to achieve the pairing;
    //   calculations made prior to motion correction may be more robust
    //+ Option("volume_pairs", "provide a text file specifying the volume indices that are paired and should therefore be combined")
    //  + Argument("file").type_file_in()
// TODO An additional command-line option could deal with the case
//   where such bijective correspondence can't be achieved from the data,
//   but it can instead be asserted that their order within each PE group is fixed

const OptionGroup combinepredicted_options = OptionGroup("Options specific to \"combine_predicted\" operation")
  + Option("lmax", "set the maximal spherical harmonic degrees to use"
                   " (one for each b-value)"
                   " during signal reconstruction")
      + Argument("value").type_sequence_int()
  + Option("exponent", "set the exponent modulating relative contributions"
                       " between empirical and predicted signal"
                       " (see Description)")
      + Argument("value").type_float();

void usage() {

  AUTHOR = "Robert E. Smith (fobert.smith@florey.edu.au)";

  SYNOPSIS = "Perform reconstruction of DWI data from an input DWI series";

  DESCRIPTION
  + "This command provides a range of mechanisms by which to reconstruct estimated DWI data"
    " given a set of input DWI data and possibly other information about the image"
    " acquisition and/or reconstruction process. "
    "The operation that is appropriate for a given workflow is entirely dependent"
    " on the context of the details of that workflow and how the image data were acquired. "
    "Each operation available is described in further detail below."

  + "The \"combine_pairs\" operation is applicable in the scenario where the DWI acquisition"
    " involves acquiring the same diffusion gradient table twice,"
    " with the direction of phase encoding reversed in the second acquisition. "
    "It is a requirement in this case that the total readout time be equivalent between the two series;"
    " that is, they vary based only on the direction of phase encoding, not the speed."
    "The purpose of this command in that context is to take as input the full set of volumes"
    " (ie. from both phase encoding directions),"
    " find those pairs of DWI volumes with equivalent diffusion sensitisation"
    " but opposite phase encoding direction,"
    " and explicitly combine each pair into a single output volume,"
    " where the contribution of each image in the pair to the output image intensity"
    " is modulated by the relative Jacobians of the two distorted images:"
    " out = ((in_1 * jacobian_1^2) + (in_2 * jacobian_2^2)) / (jacobian_1^2 + jacobian_2^2)."

  + "The \"combine_predicted\" operation is intended for DWI acquisition designs"
    " where the diffusion gradient table is split between different phase encoding directions. "
    "Here, where there is greater uncertainty in what the DWI signal should look like"
    " due to susceptibility-driven signal compression in the acquired image data,"
    " the reconstructed image will be moreso influenced by the signal intensity"
    " that is estimated from those volumes with different phase encoding"
    " that did not experience such compression. "
    "The output signal intensity is determined by the expression:"
    " out = (weight * empirical) + ((1.0 - weight) * predicted),"
    " where weight = max(0, min(1, jacobian^exponent));"
    " in this way,"
    " where the Jacobian for a volume is 1 or greater"
    " (ie. signal was expanded in the acquired image data)"
    " the empirical intensity is preserved exactly,"
    " whereas where it is less than 1"
    " (ie. signal was compressed in the acquired image data,"
    " leading to a loss of spatial contrast),"
    " the empirical data are aggregated with that predicted"
    " from groups of volumes with alternative phase encoding directions,"
    " with the relative contributions influenced by the value of command-line option -exponent"
    " (which has a default value of 1.0).";

  ARGUMENTS
    + Argument ("input", "the input DWI series").type_image_in()
    + Argument ("operation", "the way in which output DWIs will be reconstructed;"
                " one of: " + join(operations, ", ")).type_choice(operations)
    + Argument ("output", "the output DWI series").type_image_out();

  OPTIONS
    + common_options
    //+ combinepairs_options
    + combinepredicted_options

    // TODO Appropriate to have other command-line options to specify the phase encoding design?
    + Metadata::PhaseEncoding::ImportOptions
    + DWI::GradImportOptions()
    + DWI::GradExportOptions();

}
// clang-format on

using scheme_type = Eigen::Matrix<default_type, Eigen::Dynamic, Eigen::Dynamic>;
using spherical_scheme_type = Eigen::Matrix<default_type, Eigen::Dynamic, 2>;
using sh_transform_type = Eigen::Matrix<default_type, Eigen::Dynamic, Eigen::Dynamic>;
using data_vector_type = Eigen::Matrix<default_type, Eigen::Dynamic, 1>;

//////////////////////
// Shared functions //
//////////////////////

Image<float> get_field_image(const Image<float> dwi_in, const std::string &operation, const bool compulsory) {

  auto opt = get_options("field");
  Image<float> field_image;
  if (opt.empty()) {
    if (compulsory)
      throw Exception("-field option is compulsory for \"" + operation + "\" operation");
    WARN(std::string("No susceptibility field image provided") + //
         " for \"" + operation + "\" operation;" +               //
         " some functionality will be omitted");                 //
  } else {
    field_image = Image<float>::open(std::string(opt[0][0]));
    if (!voxel_grids_match_in_scanner_space(dwi_in, field_image))
      throw Exception("Susceptibility field image and DWI series not defined on same voxel grid");
    if (!(field_image.ndim() == 3 || (field_image.ndim() == 4 && field_image.size(3) == 1)))
      throw Exception("Susceptibility field image expected to be 3D");
  }
  return field_image;
}

// Generate mapping from volume index to shell index
std::vector<int> get_vol2shell(const DWI::Shells &shells, const size_t volume_count) {
  std::vector<int> vol2shell(volume_count, -1);
  for (size_t shell_index = 0; shell_index != shells.count(); ++shell_index) {
    const DWI::Shell &shell(shells[shell_index]);
    for (const auto &volume_index : shell.get_volumes()) {
      assert(vol2shell[volume_index] == -1);
      vol2shell[volume_index] = shell_index;
    }
  }
  assert(*std::min_element(vol2shell.begin(), vol2shell.end()) == 0);
  return vol2shell;
}

// Find:
//   - Which image axis the gradient is to be computed along
//   - Whether the sign needs to be negated
std::pair<size_t, default_type> get_pe_axis_and_polarity(const Eigen::Block<scheme_type, 1, 3> pe_dir) {
  for (size_t axis_index = 0; axis_index != 3; ++axis_index) {
    if (pe_dir[axis_index] != 0)
      return std::make_pair(axis_index, pe_dir[axis_index] > 0 ? 1.0 : -1.0);
  }
  assert(false);
  return std::make_pair(size_t(3), std::numeric_limits<default_type>::signaling_NaN());
}

/////////////////////////////////////////
// Functions for individual operations //
/////////////////////////////////////////

// TODO Technically the restrictions on this could be relaxed a bit:
//   hypothetically one could have a dataset
//   where each desired diffusion sensitisation
//   has been acquired with > 2 phase encoding directions,
//   and one wishes for all of those volumes to be combined into a single output volume,
//   with weights determined by their respective Jacobians
void run_combine_pairs(Image<float> &dwi_in, const scheme_type &grad_in, const scheme_type &pe_in, Header &header_out) {

  if (grad_in.rows() % 2)
    throw Exception("Cannot perform explicit volume recombination based on phase encoding pairs:"
                    " number of volumes is odd");

  const std::vector<std::string> invalid_options{"exponent", "lmax"};
  for (const auto opt : invalid_options)
    if (!get_options(opt).empty())
      throw Exception("-" + opt + " option not supported for \"combine_pairs\" operation");

  Image<float> field_image = get_field_image(dwi_in, "combine_pairs", false);

  scheme_type pe_config;
  Eigen::Array<int, Eigen::Dynamic, 1> pe_indices;
  // Even though the function is called "topup2eddy()",
  //   we can nevertheless use it here to arrange volumes into groups
  Metadata::PhaseEncoding::topup2eddy(pe_in, pe_config, pe_indices);
  if (pe_config.rows() % 2)
    throw Exception("Cannot perform explicit volume recombination based on phase encoding pairs:"
                    " number of unique phase encodings is odd");
  // The FSL topup / eddy format indexes from one;
  //   change to starting from zero for internal array indexing
  pe_indices -= 1;
  DEBUG("pe_in:\n" + str(pe_in));
  DEBUG("pe_indices:\n" + str(pe_indices.transpose()));
  DEBUG("pe_config:\n" + str(pe_config));

  // Ensure that for each line in pe_config,
  //   there is a corresponding line with the same total readout time
  //   but opposite phase encoding
  std::vector<int> peindex2paired(pe_config.rows(), -1);
  {
    std::vector<std::pair<size_t, size_t>> pe_pairs;
    for (size_t pe_first_index = 0; pe_first_index != pe_config.rows(); ++pe_first_index) {
      if (peindex2paired[pe_first_index] >= 0)
        continue;
      const auto pe_first = pe_config.row(pe_first_index);
      size_t pe_second_index;
      for (pe_second_index = pe_first_index + 1; pe_second_index != pe_config.rows(); ++pe_second_index) {
        const auto pe_second = pe_config.row(pe_second_index);
        // Phase encoding same axis but reversed direction
        // and
        // Equal total readout time (if present)
        if (((pe_second.head(3) + pe_first.head(3)).squaredNorm() == 0) &&                     //
            (pe_config.cols() == 3 ||                                                          //
             std::abs(pe_second[3] - pe_first[3]) < Metadata::PhaseEncoding::trt_tolerance)) { //
          peindex2paired[pe_first_index] = pe_second_index;
          peindex2paired[pe_second_index] = pe_first_index;
          pe_pairs.push_back(std::make_pair(pe_first_index, pe_second_index));
          break;
        }
      }
      if (pe_second_index == pe_config.rows())
        throw Exception(std::string("Unable to find corresponding reversed phase encoding volumes") + //
                        " for: [" + str(pe_first.transpose()) + "]");                                 //
    }
    assert(*std::min_element(peindex2paired.begin(), peindex2paired.end()) == 0);
  }

  DWI::Shells shells(grad_in);
  const std::vector<int> vol2shell = get_vol2shell(shells, grad_in.rows());
  DEBUG("grad_in:\n" + str(grad_in));
  std::stringstream ss_vol2shell;
  for (const auto si : vol2shell)
    ss_vol2shell << str(si) << " ";
  DEBUG("Shell indices: " + ss_vol2shell.str());

  std::vector<std::pair<size_t, size_t>> volume_pairs;
  volume_pairs.reserve(grad_in.rows() / 2);
  for (size_t shell_index = 0; shell_index != shells.size(); ++shell_index) {
    const DWI::Shell &shell(shells[shell_index]);
    if (shell.is_bzero()) {
      // b==0 shell
      // For each volume in turn,
      //   just find the first unallocated volume in the reversed phase encoding group
      std::vector<bool> used(shell.size(), false);
      for (size_t first_index = 0; first_index != shell.size(); ++first_index) {
        if (used[first_index])
          continue;
        const size_t first_volume = shell.get_volumes()[first_index];
        // Which phase encoding group does this volume belong to?
        const size_t pe_first_index = pe_indices[first_volume];
        // Which phase encoding group must the paired volume therefore belong to?
        const size_t pe_second_index = peindex2paired[pe_first_index];
        size_t second_index = first_index + 1;
        for (; second_index != shell.size(); ++second_index) {
          if (used[second_index])
            continue;
          const size_t second_volume = shell.get_volumes()[second_index];
          if (pe_indices[second_volume] != pe_second_index)
            continue;
          volume_pairs.push_back(std::make_pair(first_volume, second_volume));
          used[first_index] = true;
          used[second_index] = true;
          break;
        }
        if (second_index == shell.size())
          throw Exception(std::string("Unbalanced distribution of b=0 volumes") +    //
                          " across reversed phase encoding directions" +             //
                          " (no match found for volume " + str(first_volume) + ")"); //
      }
    } else {
      // b!=0 shell
      // Generate full similarity matrix
      // Flag comparisons between volumes that do not belong to opposed phase encoding groups
      Eigen::MatrixXd dp_matrix(Eigen::MatrixXd::Constant(shell.size(), shell.size(), -1.0));
      for (size_t row = 0; row != shell.size(); ++row) {
        const size_t first_volume = shell.get_volumes()[row];
        const size_t pe_first_index = pe_indices[first_volume];
        const size_t pe_second_index = peindex2paired[pe_first_index];
        for (size_t col = row + 1; col != shell.size(); ++col) {
          const size_t second_volume = shell.get_volumes()[col];
          if (pe_indices[second_volume] != pe_second_index)
            continue;
          dp_matrix(row, col) = dp_matrix(col, row) =
              std::abs(grad_in.block<1, 3>(first_volume, 0).dot(grad_in.block<1, 3>(second_volume, 0)));
        }
      }
      // Establish bijection
      default_type min_closest_dp = 1.0;
      default_type max_nonclosest_dp = -1.0;
      for (size_t col = 0; col != shell.size(); ++col) {
        ssize_t row;
        const default_type this_closest_dp = dp_matrix.col(col).maxCoeff(&row);
        if (this_closest_dp < 0.0)
          throw Exception(std::string("No reversed phase encoding volume found") + //
                          " for volume " + str(shell.get_volumes()[col]));         //
        if (col > row)
          continue;
        ssize_t min_col;
        dp_matrix.col(row).maxCoeff(&min_col);
        if (min_col != col)
          throw Exception(std::string("Ambiguity in establishing reversed phase encoding volumes") + //
                          "for shell b=" + str(int(std::round(shell.get_mean()))));                  //
        volume_pairs.push_back(std::make_pair(shell.get_volumes()[col], shell.get_volumes()[row]));
        min_closest_dp = std::min(min_closest_dp, this_closest_dp);
        // Ensure that there are no two unmatched volumes
        //   that are closer than any two matched volumes
        dp_matrix(row, col) = dp_matrix(col, row) = -1.0;
        max_nonclosest_dp = std::min({max_nonclosest_dp, dp_matrix.col(col).maxCoeff(), dp_matrix.col(row).maxCoeff()});
      }
      if (max_nonclosest_dp > min_closest_dp) {
        WARN("Potential ambiguity in reversed phase encoding volume correspondence;"
             "recommend checking manually");
      }
    }
  }

  // Arrange these volume pairs into a suitable order
  //   in which to write them into the output image
  auto sort_pairs = [](const std::pair<size_t, size_t> &one, const std::pair<size_t, size_t> &two) {
    return std::min(one.first, one.second) < std::min(two.first, two.second);
  };
  std::sort(volume_pairs.begin(), volume_pairs.end(), sort_pairs);

  // Populate data relating to the combination of these images
  //   arising from a prior algorithmic approach
  std::vector<ssize_t> in2outindex(grad_in.rows(), -1);
  scheme_type grad_out(grad_in.rows() / 2, 4);
  for (ssize_t out_index = 0; out_index != grad_out.rows(); ++out_index) {
    const size_t first_volume = volume_pairs[out_index].first;
    const size_t second_volume = volume_pairs[out_index].second;
    in2outindex[first_volume] = out_index;
    in2outindex[second_volume] = out_index;
    grad_out.block<1, 3>(out_index, 0) =
        grad_in.block<1, 3>(first_volume, 0).dot(grad_in.block<1, 3>(second_volume, 0)) < 0.0
            ? (grad_in.block<1, 3>(first_volume, 0) - grad_in.block<1, 3>(second_volume, 0)).normalized()
            : (grad_in.block<1, 3>(first_volume, 0) + grad_in.block<1, 3>(second_volume, 0)).normalized();
    grad_out(out_index, 3) = 0.5 * (grad_in(first_volume, 3) + grad_in(second_volume, 3));
  }

  assert(*std::min_element(in2outindex.begin(), in2outindex.end()) == 0);
  std::stringstream ss_volpairs;
  for (const auto p : volume_pairs)
    ss_volpairs << "[" << str(p.first) << "," << str(p.second) << "] ";
  DEBUG("Volume pairs:\n" + ss_volpairs.str());
  std::stringstream ss_in2out;
  for (const auto i : in2outindex)
    ss_in2out << str(i) << " ";
  DEBUG("Input to output index:\n" + ss_in2out.str());

  header_out.size(3) = dwi_in.size(3) / 2;
  DWI::set_DW_scheme(header_out, grad_out);
  Image<float> dwi_out = Image<float>::create(header_out.name(), header_out);

  if (field_image.valid()) {

    // std::vector<Image<float>> jacobian_images;
    std::vector<Image<float>> weight_images;
    {
      // Need to calculate the "weight" to be applied to each phase encoding group during volume recombination
      // This is based on the Jacobian of the field along the phase encoding direction,
      //   scaled by the total readout time
      ProgressBar progress("Computing phase encoding group weighting images", pe_config.rows() + 1);
      // Apply a little smoothing to the field image before computing Jacobians;
      //   this is to be consistent with prior dwifslpreproc code,
      //   which directly invoked "mrfilter gradient"
      // Allow smoothing filter to use internal default
      Filter::Smooth smoothing_filter(field_image);
      Image<float> smoothed_field = Image<float>::scratch(field_image, "Smoothed field offset image");
      smoothing_filter(field_image, smoothed_field);
      ++progress;
      Adapter::Gradient1D<Image<float>> gradient(smoothed_field);
      for (size_t pe_index = 0; pe_index != pe_config.rows(); ++pe_index) {
        // Image<float> jacobian_image =
        //     Image<float>::scratch(field_image, "Scratch Jacobian image for PE index " + str(pe_index));
        Image<float> weight_image =
            Image<float>::scratch(field_image, "Scratch weight image for PE index " + str(pe_index));
        const auto pe_axis_and_multiplier = get_pe_axis_and_polarity(pe_config.block<1, 3>(pe_index, 0));
        gradient.set_axis(pe_axis_and_multiplier.first);
        const default_type multiplier = pe_axis_and_multiplier.second * pe_config(pe_index, 3);
        for (auto l = Loop(gradient)(gradient, /*jacobian_image,*/ weight_image); l; ++l) {
          const default_type jacobian = std::max(0.0, 1.0 + (gradient.value() * multiplier));
          // jacobian_image.value() = jacobian;
          weight_image.value() = Math::pow2(jacobian);
        }
        // jacobian_images.push_back(std::move(jacobian_image));
        weight_images.push_back(std::move(weight_image));
        ++progress;
      }
    }
    ProgressBar progress("Performing explicit volume recombination", header_out.size(3));
    for (size_t out_volume = 0; out_volume != header_out.size(3); ++out_volume) {
      dwi_out.index(3) = out_volume;
      Image<float> first_volume(dwi_in), second_volume(dwi_in);
      first_volume.index(3) = volume_pairs[out_volume].first;
      second_volume.index(3) = volume_pairs[out_volume].second;
      Image<float> first_weight(weight_images[pe_indices[volume_pairs[out_volume].first]]);
      Image<float> second_weight(weight_images[pe_indices[volume_pairs[out_volume].second]]);
      for (auto l = Loop(dwi_out, 0, 3)(dwi_out, first_volume, second_volume, first_weight, second_weight); l; ++l) {
        dwi_out.value() =
            ((first_volume.value() * first_weight.value()) + (second_volume.value() * second_weight.value())) / //
            (first_weight.value() + second_weight.value());
      }
      ++progress;
    }

  } else {

    // No field map image provided; do a straight averaging of input volumes into output
    ProgressBar progress("Performing explicit volume recombination", header_out.size(3));
    for (size_t out_volume = 0; out_volume != header_out.size(3); ++out_volume) {
      Image<float> first_volume(dwi_in), second_volume(dwi_in);
      dwi_out.index(3) = out_volume;
      first_volume.index(3) = volume_pairs[out_volume].first;
      second_volume.index(3) = volume_pairs[out_volume].second;
      for (auto l = Loop(dwi_out, 0, 3)(dwi_out, first_volume, second_volume); l; ++l)
        dwi_out.value() = 0.5 * (first_volume.value() + second_volume.value());
      ++progress;
    }
  }
}

// TODO Identify code from combine_pairs that can be shared
void run_combine_predicted(Image<float> &dwi_in,
                           const scheme_type &grad_in,
                           const scheme_type &pe_in,
                           Header &header_out) {

  const std::vector<std::string> invalid_options{"volume_pairs"};
  for (const auto opt : invalid_options)
    if (!get_options(opt).empty())
      throw Exception("-" + opt + " option not supported for \"combine_predicted\" operation");

  Image<float> field_image = get_field_image(dwi_in, "combine_predicted", true);
  const default_type exponent = get_option_value("exponent", default_combinepredicted_exponent);

  scheme_type pe_config;
  Eigen::Array<int, Eigen::Dynamic, 1> pe_indices;
  // Even though the function is called "topup2eddy()",
  //   we can nevertheless use it to perform the arrangement of volumes into groups
  Metadata::PhaseEncoding::topup2eddy(pe_in, pe_config, pe_indices);
  // The FSL topup / eddy format indexes from one;
  //   change to starting from zero for internal array indexing
  pe_indices -= 1;

  DWI::Shells shells(grad_in);
  const std::vector<int> vol2shell = get_vol2shell(shells, grad_in.rows());

  auto opt = get_options("lmax");
  std::vector<int> lmax_user;
  if (!opt.empty()) {
    lmax_user = parse_ints<int>(opt[0][0]);
    if (lmax_user.size() != shells.count())
      throw Exception("-lmax option must specify one lmax for each unique b-value");
    for (size_t shell_index = 0; shell_index != shells.count(); ++shell_index) {
      if (lmax_user[shell_index] % 2)
        throw Exception("-lmax values must be even numbers");
      // Technically this is a weak constraint:
      //   user-requested lmax may not be possible once excluding a phase encoding group
      if (lmax_user[shell_index] > Math::SH::NforL(shells[shell_index].count()))
        throw Exception("Requested lmax=" + str(lmax_user[shell_index]) +                                  //
                        " for shell b=" + str<int>(shells[shell_index].get_mean()) + "," +                 //
                        " but only " + str(shells[shell_index].count()) + " volumes," +                    //
                        " which only supports lmax=" + str(Math::SH::NforL(shells[shell_index].count()))); //
    }
  }

  // Apply a little smoothing to the field image before computing Jacobians;
  //   this is to be consistent with prior dwifslpreproc code,
  //   which directly invoked "mrfilter gradient"
  // Allow smoothing filter to use internal default
  Image<float> smoothed_field = Image<float>::scratch(field_image, "Smoothed field offset image");
  std::vector<Image<float>> jacobian_images;
  {
    ProgressBar progress("Calculating phase encoding group Jacobians", pe_config.rows() + 1);
    Filter::Smooth smoothing_filter(field_image);
    smoothing_filter(field_image, smoothed_field);
    ++progress;
    Adapter::Gradient1D<Image<float>> gradient(smoothed_field);

    // Immediately generate Jacobian images for each phase encoding group;
    //   these can then be used for both:
    //   - Computing the weight to be attributed to the empirical data in output data generation
    //   - Construction of weighted SH fit
    for (size_t pe_index = 0; pe_index != pe_config.rows(); ++pe_index) {
      const auto pe_axis_and_multiplier = get_pe_axis_and_polarity(pe_config.block<1, 3>(pe_index, 0));
      gradient.set_axis(pe_axis_and_multiplier.first);
      const default_type multiplier = pe_axis_and_multiplier.second * pe_config(pe_index, 3);
      Image<float> jacobian_image =
          Image<float>::scratch(field_image, "Jacobian image for phase encoding group " + str(pe_index));
      for (auto l = Loop(gradient)(gradient, jacobian_image); l; ++l)
        jacobian_image.value() = std::max(0.0, 1.0 + (gradient.value() * multiplier));
      jacobian_images.push_back(std::move(jacobian_image));
      ++progress;
    }
  }

  Image<float> dwi_out = Image<float>::create(header_out.name(), header_out);

  ProgressBar progress("Reconstructing volumes combining empirical and predicted intensities",
                       pe_config.rows() * shells.count());
  for (size_t pe_index = 0; pe_index != pe_config.rows(); ++pe_index) {

    // Branch depending on how many other phase encoding groups there are:
    //  - If only one other, then just construct the A->SH->A transform for that one group;
    //  - If more than one, construct the weighted A->SH->A transform considering all other groups
    // In both cases,
    //   the output amplitude directions are based on the phase encoding group currently being reconstructed,
    //   whereas the input amplitude directions are those of all other phase encoding groups
    // However a key difference is that:
    //   - In the situation where there's only one other phase encoding group,
    //     we can pre-compute the source2SH transformation
    //     (and therefore the source2target transformation)
    //     just once, and use it for every voxel,
    //     merely modulating how much the prediction vs. the empirical data contribute in each voxel
    //   - If using data from all other phase encoding groups together,
    //     the source2SH transformation has to be recomputed in every voxel
    //     (and will therefore likely be considerably slower)
    //
    // TODO Reconsider how predictions are generated:
    // Once a weighted fit is performed,
    //   the prediction could actually make use of information within the phase encoding group of interest,
    //   and this could nevertheless be integrated into generation of predictions
    // Among other things,
    //   this would remove the complications of checking whether a user-requested lmax is sensible,
    //   since it currently doesn't consider that SH transforms are computed while omitting data
    // It would however arguably place greater development priority on:
    // - Use of leave-one-out
    // - Weighting samples by proximity

    // Loop over shells
    for (size_t shell_index = 0; shell_index != shells.count(); ++shell_index) {

      DEBUG(std::string("Commencing reconstruction") +              //
            " for PE group " + str(pe_config.row(pe_index)) + "," + //
            " shell b=" + str(shells[shell_index].get_mean()));     //

      // Obtain volumes that belong both to this shell and:
      // - To the source phase encoding group; or
      // - To any other phase encoding group
      std::vector<size_t> source_volumes;
      std::vector<size_t> target_volumes;
      for (const auto volume : shells[shell_index].get_volumes()) {
        if (pe_indices[volume] == pe_index)
          target_volumes.push_back(volume);
        else
          source_volumes.push_back(volume);
      }
      assert(!source_volumes.empty());
      assert(!target_volumes.empty());
      std::stringstream ss_sources, ss_targets;
      for (const auto i : source_volumes)
        ss_sources << str(i) << " ";
      for (const auto i : target_volumes)
        ss_targets << str(i) << " ";
      DEBUG(str(source_volumes.size()) + " source volumes for this reconstruction: " + ss_sources.str());
      DEBUG(str(target_volumes.size()) + " target volumes for this reconstruction: " + ss_targets.str());
      const size_t lmax_data = shells[shell_index].is_bzero() ? 0 : Math::SH::LforN(source_volumes.size());
      size_t lmax;
      if (lmax_user.empty()) {
        lmax = lmax_data;
      } else {
        lmax = lmax_user[shell_index];
        if (lmax > lmax_data)
          throw Exception("User-requested lmax=" + str(lmax) +
                          " for shell b=" + str<int>(shells[shell_index].get_mean()) +
                          " exceeds what can be predicted from data after phase encoding group exclusion");
      }
      DEBUG("Reconstruction will use lmax=" + str(lmax));

      // Generate the direction set for the target data
      spherical_scheme_type target_dirset(target_volumes.size(), 2);
      for (size_t target_index = 0; target_index != target_volumes.size(); ++target_index)
        Math::Sphere::cartesian2spherical(grad_in.block<1, 3>(target_volumes[target_index], 0),
                                          target_dirset.row(target_index));
      // Generate the transformation from SH to the target data
      // TODO Need to confirm behaviour when the lmax of the source data exceeds
      //   what can actually be achieved for the target data in constructing the inverse transform
      const sh_transform_type SH2target = Math::SH::init_transform(target_dirset, lmax);

      spherical_scheme_type source_dirset(source_volumes.size(), 2);
      data_vector_type source_data(source_volumes.size());
      sh_transform_type source2SH(Math::SH::NforL(lmax), source_volumes.size());
      sh_transform_type source2target(target_volumes.size(), source_volumes.size());
      data_vector_type predicted_data(target_volumes.size());

      if (pe_config.rows() == 2) {

        // Generate the direction set for the source data
        for (size_t source_index = 0; source_index != source_volumes.size(); ++source_index)
          Math::Sphere::cartesian2spherical(grad_in.block<1, 3>(source_volumes[source_index], 0),
                                            source_dirset.row(source_index));
        // Generate the transformation from the source data to spherical harmonics
        // TODO For now, using the maximal spherical harmonic degree enabled by the source data
        // TODO For now, weighting all samples equally
        source2SH = Math::pinv(Math::SH::init_transform(source_dirset, lmax));
        // Compose transformation from source data to target data
        source2target = SH2target * source2SH;

        // Now we are ready to loop over the image
        Image<float> jacobian(jacobian_images[pe_index]);
        for (auto l = Loop(jacobian)(jacobian, dwi_in, dwi_out); l; ++l) {
          // How much weight are we attributing to the empirical data?
          // (if 1.0, we don't need to bother generating predictions)
          default_type empirical_weight = std::max(0.0, std::min(1.0, default_type(jacobian.value())));
          if (empirical_weight == 1.0) {
            for (const auto volume : target_volumes) {
              dwi_in.index(3) = dwi_out.index(3) = volume;
              dwi_out.value() = dwi_in.value();
            }
          } else {
            // Clamp here is only to deal with the prospect of "-exponent -inf"
            empirical_weight = std::min(1.0, std::pow(empirical_weight, exponent));
            // Grab the input data for generating the predictions
            for (size_t source_index = 0; source_index != source_volumes.size(); ++source_index) {
              dwi_in.index(3) = source_volumes[source_index];
              source_data[source_index] = dwi_in.value();
            }
            // Generate the predictions
            predicted_data.noalias() = source2target * source_data;
            // Write these to the output image
            for (size_t target_index = 0; target_index != target_volumes.size(); ++target_index) {
              dwi_in.index(3) = dwi_out.index(3) = target_volumes[target_index];
              dwi_out.value() =
                  (empirical_weight * dwi_in.value()) + ((1.0 - empirical_weight) * predicted_data[target_index]);
            }
          }
        }

      } else {

        // More than two phase encoding groups;
        //   therefore multiple phase encoding groups contributing to predictions

        data_vector_type source_weights(source_volumes.size());
        data_vector_type jacobians(pe_config.rows());

        // Build part of the requisite data for the A2SH transform in this voxel
        //   (the directions are the same for every voxel;
        //   the weights, which are influenced by the Jacobians, are not)
        for (size_t source_index = 0; source_index != source_volumes.size(); ++source_index)
          Math::Sphere::cartesian2spherical(grad_in.block<1, 3>(source_volumes[source_index], 0),
                                            source_dirset.row(source_index));

        for (auto l = Loop(dwi_in, 0, 3)(dwi_in, dwi_out); l; ++l) {

          // We may need access to Jacobians for all phase encoding groups
          for (size_t jacobian_index = 0; jacobian_index != pe_config.rows(); ++jacobian_index) {
            assign_pos_of(dwi_in, 0, 3).to(jacobian_images[jacobian_index]);
            jacobians[jacobian_index] = jacobian_images[jacobian_index].value();
          }

          // If using exclusively empirical data,
          //   make that determination as soon as possible to avoid unnecessary computation
          default_type empirical_weight = std::max(0.0, std::min(1.0, jacobians[pe_index]));
          if (empirical_weight == 1.0) {
            for (const auto volume : target_volumes) {
              dwi_in.index(3) = dwi_out.index(3) = volume;
              dwi_out.value() = dwi_in.value();
            }
          } else {
            empirical_weight = std::min(1.0, std::pow(empirical_weight, exponent));
            // Build the rest of the requisite data for the A2SH transform in this voxel
            // Also grab the input data while we're looping
            // Should the relative contributions from the other phase encoding groups
            //   be modulated by a similar expression
            //   to how the weight ascribed to the empirical data is determined?
            // This would need to deal with the prospect of an exponent of -inf / +inf
            //   (or be a separate command-line option)
            for (size_t source_index = 0; source_index != source_volumes.size(); ++source_index) {
              source_weights[source_index] = std::max(0.0, jacobians[pe_indices[source_volumes[source_index]]]);
              dwi_in.index(3) = source_volumes[source_index];
              source_data[source_index] = dwi_in.value();
            }
            // Build the transformation from data in all other phase encoding groups to SH
            source2SH = Math::wls(Math::SH::init_transform(source_dirset, lmax), source_weights);
            // Compose transformation from source data to target data
            source2target.noalias() = SH2target * source2SH;
            // Generate the predictions
            predicted_data.noalias() = source2target * source_data;
            // Write these to the output image
            for (size_t target_index = 0; target_index != target_volumes.size(); ++target_index) {
              dwi_in.index(3) = dwi_out.index(3) = target_volumes[target_index];
              dwi_out.value() =
                  (empirical_weight * dwi_in.value()) + ((1.0 - empirical_weight) * predicted_data[target_index]);
            }
          }
        }

      } // End branching on number of phase encoding groups being 2 or more

      ++progress;
    } // End looping over shells

  } // End looping over phase encoding groups
}

void run() {

  auto dwi_in = Header::open(argument[0]).get_image<float>();
  auto grad_in = DWI::get_DW_scheme(dwi_in);
  auto pe_in = Metadata::PhaseEncoding::get_scheme(dwi_in);

  Header header_out(dwi_in);
  header_out.datatype() = DataType::Float32;
  header_out.datatype().set_byte_order_native();
  header_out.name() = std::string(argument[2]);

  switch (int(argument[1])) {

  case 0:
    run_combine_pairs(dwi_in, grad_in, pe_in, header_out);
    Metadata::PhaseEncoding::clear_scheme(header_out.keyval());
    break;

  case 1:
    run_combine_predicted(dwi_in, grad_in, pe_in, header_out);
    break;

  default: // no others yet implemented
    assert(false);
  }

  DWI::export_grad_commandline(header_out);
}
