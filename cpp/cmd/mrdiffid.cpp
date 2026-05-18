/* Copyright (c) 2008-2026 the MRtrix3 contributors.
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

#include <Eigen/Dense>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <map>
#include <vector>

#include "command.h"
#include "file/matrix.h"
#include "fixel/helpers.h"
#include "header.h"
#include "image.h"
#include "image_helpers.h"
#include "misc/voxel2vector.h"
#include "progressbar.h"

using namespace MR;
using namespace App;

// clang-format off
void usage() {

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Compute the Differential Identifiability measure across an image dataset";

  DESCRIPTION
  + "Given a set of input images and a corresponding list of integer subject identifiers,"
    " this command computes pairwise cosine similarities between all subjects,"
    " and derives the Differential Identifiability measure I_diff."
  + "I_diff = (I_self - I_others) x 100,"
    " where I_self is the mean pairwise cosine similarity for subjects sharing the same identifier,"
    " and I_others is the mean pairwise cosine similarity for subjects with differing identifiers."
  + "This command operates equivalently on volumetric images and fixel data files."
    " All input images must possess identical dimensions;"
    " volumetric images must additionally reside on a common voxel grid in scanner space,"
    " their intensity data being serialised into the columns of the data matrix."
  + "The -mask option modulates which image elements contribute to the quantification."
    " For volumetric inputs the mask is a 3D image,"
    " every non-zero voxel of which contributes one row to the data matrix;"
    " for fixel data files the mask is itself a fixel data file,"
    " selecting which fixels are carried through to the data matrix.";

  ARGUMENTS
  + Argument("images",
             "a text file containing the filesystem paths of the images to be processed"
             " (one path per line);").type_file_in()

  + Argument("ids",
             "a text file containing a 1D integer matrix of subject identifiers,"
             " with one entry per input image").type_file_in();

  OPTIONS
  + Option("mask",
           "only include those image elements within the specified mask"
           " in the computation of Differential Identifiability;"
           " a volumetric image for volumetric inputs,"
           " or a fixel data file for fixel inputs")
    + Argument("image").type_image_in();

  REFERENCES
  + "Amico E, Goni J."
    " The quest for identifiability in human functional connectomes."
    " Scientific Reports, 2018, 8(1), 8254.";

}
// clang-format on

void run() {
  // Read input image paths from text file
  std::vector<std::filesystem::path> image_paths;
  {
    const std::filesystem::path list_path = argument[0];
    std::ifstream ifs(list_path);
    if (!ifs)
      throw Exception("Unable to open image list file \"" + argument[0].as_text() + "\"");
    std::string line;
    while (std::getline(ifs, line)) {
      const size_t end = line.find_last_not_of(" \t\r\n");
      if (end != std::string::npos) {
        line.erase(end + 1);
        if (!line.empty())
          image_paths.emplace_back(line);
      }
    }
  }
  if (image_paths.empty())
    throw Exception("No image paths found in file \"" + argument[0].as_text() + "\"");

  const size_t nsubjects = image_paths.size();

  // Load subject identifiers
  const Eigen::Matrix<int, Eigen::Dynamic, 1> ids = File::Matrix::load_vector<int>(argument[1]);
  if (static_cast<size_t>(ids.size()) != nsubjects)
    throw Exception("Number of subject identifiers (" + str(ids.size()) + ")" + //
                    " does not match number of input images (" + str(nsubjects) + ")");

  // Open and validate all input images;
  //   the type of the first image (fixel data file vs. volumetric) is taken
  //   as authoritative, and all subsequent images are checked against it
  std::vector<Image<float>> images;
  images.reserve(nsubjects);
  Header first_header;
  bool inputs_are_fixel = false;
  {
    ProgressBar progress("Opening and validating input images", nsubjects);
    for (const auto &path : image_paths) {
      try {
        Header header = Header::open(path);
        if (images.empty()) {
          inputs_are_fixel = Fixel::is_data_file(header);
          if (inputs_are_fixel) {
            if (header.size(1) != 1)
              throw Exception(std::string("fixel data file does not have unity size along the second axis") + //
                              " (size is " + str(header.size(1)) + ")");                                      //
          } else {
            check_effective_dimensionality(header, 3);
          }
          first_header = Header(header);
        } else {
          check_dimensions(first_header, header);
          // Volumetric data must additionally coincide on a common voxel grid
          //   in scanner space; fixel data files carry no meaningful spatial
          //   transform and are compared on dimensions alone
          if (!inputs_are_fixel)
            check_voxel_grids_match_in_scanner_space(first_header, header);
        }
        images.push_back(header.get_image<float>());
      } catch (Exception &e) {
        throw Exception(e, "Error opening input image \"" + path.string() + "\"");
      }
      ++progress;
    }
  }

  // Establish the mapping from input image elements to data matrix rows.
  //   For volumetric inputs this is delegated to Voxel2Vector, which serialises
  //   the 3D intensity data (optionally restricted by a processing mask) into a
  //   single column; for fixel data files the mapping is a permutation vector of
  //   fixel indices, again optionally restricted by a mask fixel data file.
  auto opt = get_options("mask");
  std::unique_ptr<Voxel2Vector> v2v;
  std::vector<Voxel2Vector::index_t> fixel_selection;
  ssize_t nelements = -1;
  if (inputs_are_fixel) {
    const ssize_t nfixels = first_header.size(0);
    if (opt.empty()) {
      fixel_selection.resize(nfixels);
      for (ssize_t f = 0; f != nfixels; ++f)
        fixel_selection[f] = static_cast<Voxel2Vector::index_t>(f);
    } else {
      Header mask_header = Header::open(opt[0][0]);
      if (!Fixel::is_data_file(mask_header))
        throw Exception("Mask image \"" + opt[0][0].as_text() + "\" is not a fixel data file," + //
                        " as required to match the fixel data file inputs");                     //
      if (mask_header.size(0) != nfixels)
        throw Exception("Mask fixel data file \"" + opt[0][0].as_text() + "\"" +             //
                        " contains " + str(mask_header.size(0)) + " fixels," +               //
                        " inconsistent with the " + str(nfixels) + " fixels in the inputs"); //
      Image<bool> mask_image = mask_header.get_image<bool>();
      fixel_selection.reserve(nfixels);
      for (ssize_t f = 0; f != nfixels; ++f) {
        mask_image.index(0) = f;
        if (mask_image.value())
          fixel_selection.push_back(static_cast<Voxel2Vector::index_t>(f));
      }
    }
    nelements = static_cast<ssize_t>(fixel_selection.size());
  } else {
    if (opt.empty()) {
      // Pass a const reference so that overload resolution selects the
      //   Header-only constructor rather than the mask-templated constructor
      v2v = std::make_unique<Voxel2Vector>(static_cast<const Header &>(first_header));
    } else {
      Header mask_header = Header::open(opt[0][0]);
      check_effective_dimensionality(mask_header, 3);
      if (!dimensions_match(first_header, mask_header, 0, 3))
        throw Exception("Mask image \"" + opt[0][0].as_text() + "\"" + //
                        " does not match the dimensions of the input images");
      Image<bool> mask_image = mask_header.get_image<bool>();
      v2v = std::make_unique<Voxel2Vector>(mask_image, first_header);
    }
    nelements = static_cast<ssize_t>(v2v->size());
  }
  if (nelements == 0)
    throw Exception("No image elements selected for processing"
                    " (mask may be empty)");
  DEBUG("Number of image elements: " + str(nelements));
  DEBUG("Number of subjects: " + str(nsubjects));

  // Populate data matrix: nelements x nsubjects
  Eigen::MatrixXd data(nelements, nsubjects);
  {
    ProgressBar progress("Populating data matrix from input images", nsubjects);
    for (size_t col = 0; col != nsubjects; ++col) {
      Image<float> &image = images[col];
      if (inputs_are_fixel) {
        const Eigen::VectorXf column = Eigen::VectorXf(image.row(0));
        for (ssize_t row = 0; row != nelements; ++row)
          data(row, col) = static_cast<double>(column[fixel_selection[row]]);
      } else {
        for (ssize_t row = 0; row != nelements; ++row) {
          const std::vector<Voxel2Vector::index_t> &pos = (*v2v)[row];
          for (size_t axis = 0; axis != pos.size(); ++axis)
            image.index(axis) = pos[axis];
          data(row, col) = static_cast<double>(image.value());
        }
      }
      ++progress;
    }
  }
  images.clear();

  // Pre-compute per-column L2 norms for cosine similarity
  const Eigen::VectorXd norms = data.colwise().norm();

  // Compute theoretical upper-triangular pair counts from subject identifiers
  size_t theoretical_count_self = 0;
  {
    std::map<int, size_t> id_counts;
    for (ssize_t i = 0; i != ids.size(); ++i)
      ++id_counts[ids[i]];
    for (const auto &[id, count] : id_counts)
      theoretical_count_self += count * (count - 1) / 2;
  }
  const size_t theoretical_count_others = nsubjects * (nsubjects - 1) / 2 - theoretical_count_self;
  DEBUG("Theoretical same-identifier pair count: " + str(theoretical_count_self));
  DEBUG("Theoretical different-identifier pair count: " + str(theoretical_count_others));

  // Compute upper triangular pairwise cosine similarities;
  // accumulate separately for same-identifier and different-identifier pairs
  double sum_self = 0.0;
  double sum_others = 0.0;
  size_t count_self = 0;
  size_t count_others = 0;
  {
    ProgressBar progress("Computing pairwise cosine similarities", //
                         nsubjects * (nsubjects - 1) / 2);         //
    for (size_t j = 1; j != nsubjects; ++j) {
      for (size_t i = 0; i != j; ++i) {
        const double norm_product = norms[i] * norms[j];
        const double similarity = (norm_product > 0.0) ? data.col(i).dot(data.col(j)) / norm_product : 0.0;
        if (ids[i] == ids[j]) {
          sum_self += similarity;
          ++count_self;
        } else {
          sum_others += similarity;
          ++count_others;
        }
        ++progress;
      }
    }
  }

  assert(count_self == theoretical_count_self);
  assert(count_others == theoretical_count_others);

  if (count_self == 0)
    throw Exception("No same-identifier pairs found in upper triangular similarity matrix;"
                    " cannot compute I_self");
  if (count_others == 0)
    throw Exception("No different-identifier pairs found in upper triangular similarity matrix;"
                    " cannot compute I_others");

  const double I_self = sum_self / static_cast<double>(count_self);
  const double I_others = sum_others / static_cast<double>(count_others);
  const double I_diff = (I_self - I_others) * 100.0;

  DEBUG("I_self: " + str(I_self) + " (from " + str(count_self) + " pairs)");
  DEBUG("I_others: " + str(I_others) + " (from " + str(count_others) + " pairs)");
  DEBUG("I_diff: " + str(I_diff));

  std::cout << str(I_diff) << "\n";
}
