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
#include <cmath>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "algo/loop.h"
#include "algo/threaded_copy.h"
#include "command.h"
#include "enum.h"
#include "fixel/helpers.h"
#include "header.h"
#include "image.h"
#include "image_helpers.h"
#include "mrtrix.h"
#include "progressbar.h"

using namespace MR;
using namespace App;

// Forms of the Intraclass Correlation Coefficient,
//   labelled using the (model, form) notation of Shrout & Fleiss (1979);
//   "k" denotes the average-measurement variants.
// clang-format off
enum class form_t { ICC_1_1,    // one-way random effects, single measurement
                    ICC_2_1,    // two-way random effects, absolute agreement, single measurement
                    ICC_3_1,    // two-way mixed effects, consistency, single measurement
                    ICC_1_k,    // one-way random effects, average measurement
                    ICC_2_k,    // two-way random effects, absolute agreement, average measurement
                    ICC_3_k };  // two-way mixed effects, consistency, average measurement
// clang-format on

// Model 1 (one-way random effects) does not model a measurement factor;
//   the design file therefore provides subject identifiers only.
inline bool is_one_way(const form_t form) { return form == form_t::ICC_1_1 || form == form_t::ICC_1_k; }

// clang-format off
void usage() {

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Compute the Intraclass Correlation Coefficient (ICC) across an image dataset";

  DESCRIPTION
  + "Given a set of input images and a corresponding design file,"
    " this command computes,"
    " for each image element independently,"
    " a chosen form of the Intraclass Correlation Coefficient (ICC)."
  + "This command operates equivalently on volumetric images and fixel data files;"
    " all input images must possess identical dimensions"
    " (and, for volumetric images, identical voxel grids in scanner space),"
    " and the computation is performed independently for every image element."
  + "The available forms of ICC are labelled using the (model, form) notation"
    " of Shrout & Fleiss (1979),"
    " where the second index \"k\" denotes the average-measurement variants:"
    " \"icc_1_1\" is one-way random effects, single measurement;"
    " \"icc_2_1\" is two-way random effects, absolute agreement, single measurement;"
    " \"icc_3_1\" is two-way mixed effects, consistency, single measurement;"
    " \"icc_1_k\", \"icc_2_k\" and \"icc_3_k\" are the corresponding"
    " average-measurement variants."
  + "The design file is a text file with one row per input image,"
    " its columns being delimiter-separated"
    " (whitespace, comma or semicolon may be used as the delimiter)."
    " The number of columns required, and their interpretation,"
    " depends on the model of the chosen ICC form."
  + "For the one-way random effects models (\"icc_1_1\" and \"icc_1_k\"),"
    " the design file must contain a single column:"
    " a subject identifier for each input image."
    " Each subject must be represented by an equal number of images;"
    " no correspondence of measurements across subjects is modelled,"
    " and the ordering of images within a subject is immaterial."
  + "For the two-way models"
    " (\"icc_2_1\", \"icc_3_1\", \"icc_2_k\" and \"icc_3_k\"),"
    " the design file must contain two columns:"
    " a subject identifier followed by a measurement identifier."
    " A balanced design is required:"
    " every subject must possess exactly one image for every measurement identifier,"
    " and the same set of measurement identifiers must be present for all subjects."
    " Because each input image is explicitly tagged with both"
    " a subject identifier and a measurement identifier,"
    " a consistent ordering of measurements across subjects is not assumed.";

  ARGUMENTS
  + Argument("images",
             "a text file containing the filesystem paths of the images to be processed"
             " (one path per line)").type_file_in()

  + Argument("form",
             "the form of Intraclass Correlation Coefficient to compute"
             " (one of: " + MR::Enum::join<form_t>(", ") + ")").type_choice<form_t>()

  + Argument("design",
             "a text file with one row per input image,"
             " containing either subject identifiers only (one-way models)"
             " or subject and measurement identifiers (two-way models)").type_file_in()

  + Argument("output",
             "the output image of per-element ICC values").type_image_out();

  REFERENCES
  + "Shrout PE, Fleiss JL."
    " Intraclass correlations: uses in assessing rater reliability."
    " Psychological Bulletin, 1979, 86(2), 420-428."
  + "McGraw KO, Wong SP."
    " Forming inferences about some intraclass correlation coefficients."
    " Psychological Methods, 1996, 1(1), 30-46.";

}
// clang-format on

void run() {
  const form_t form = MR::Enum::from_name<form_t>(argument[1]);
  const bool one_way = is_one_way(form);

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
  const size_t nimages = image_paths.size();

  // Read the design file; the number of expected columns depends on the ICC model:
  //   one-way models require subject identifiers only,
  //   whereas two-way models additionally require measurement identifiers
  const size_t expected_columns = one_way ? 1 : 2;
  std::vector<std::string> subject_ids;
  std::vector<std::string> measurement_ids;
  subject_ids.reserve(nimages);
  measurement_ids.reserve(nimages);
  {
    const std::filesystem::path design_path = argument[2];
    std::ifstream ifs(design_path);
    if (!ifs)
      throw Exception("Unable to open design file \"" + argument[2].as_text() + "\"");
    std::string line;
    size_t row = 0;
    while (std::getline(ifs, line)) {
      const std::vector<std::string> tokens = split(line, " \t,;", true);
      if (tokens.empty())
        continue;
      if (tokens.size() != expected_columns)
        throw Exception("Malformed design file \"" + argument[2].as_text() + "\":" +              //
                        " row " + str(row + 1) + " contains " + str(tokens.size()) + " fields," + //
                        " but the chosen ICC form requires exactly " + str(expected_columns) +    //
                        (one_way ? " (a subject identifier)"                                      //
                                 : " (a subject identifier and a measurement identifier)"));      //
      subject_ids.push_back(tokens[0]);
      measurement_ids.push_back(one_way ? std::string() : tokens[1]);
      ++row;
    }
  }
  if (subject_ids.size() != nimages)
    throw Exception("Number of rows in design file (" + str(subject_ids.size()) + ")" + //
                    " does not match number of input images (" + str(nimages) + ")");

  // Establish subject ordering based on order of appearance
  std::map<std::string, size_t> subject_index;
  for (size_t i = 0; i != nimages; ++i)
    subject_index.emplace(subject_ids[i], subject_index.size());
  const size_t nsubjects = subject_index.size();
  if (nsubjects < 2)
    throw Exception(std::string("ICC computation requires at least two subjects") + //
                    " (only " + str(nsubjects) + " distinct subject identifiers provided)");

  // Map each subject (row) and measurement (column) to its input image index,
  //   constructing an n x k layout matrix that the per-element computation indexes into
  size_t nmeasurements = 0;
  Eigen::Matrix<ssize_t, Eigen::Dynamic, Eigen::Dynamic> layout;
  if (one_way) {
    // No measurement factor is modelled: images are simply grouped by subject,
    //   and every subject must contribute an equal number of measurements
    std::vector<std::vector<size_t>> subject_images(nsubjects);
    for (size_t i = 0; i != nimages; ++i)
      subject_images[subject_index.at(subject_ids[i])].push_back(i);
    nmeasurements = subject_images.front().size();
    for (const auto &[subject, s] : subject_index) {
      if (subject_images[s].size() != nmeasurements)
        throw Exception(std::string("Unbalanced design:") +                                        //
                        " subject \"" + subject + "\" has " + str(subject_images[s].size()) +      //
                        " images, whereas subject \"" + subject_index.begin()->first + "\" has " + //
                        str(nmeasurements) + ";" +                                                 //
                        " all subjects must be represented by an equal number of images");         //
    }
    if (nmeasurements < 2)
      throw Exception(std::string("ICC computation requires at least two measurements per subject") + //
                      " (only " + str(nmeasurements) + " provided)");
    layout.resize(nsubjects, nmeasurements);
    for (size_t s = 0; s != nsubjects; ++s)
      for (size_t m = 0; m != nmeasurements; ++m)
        layout(s, m) = static_cast<ssize_t>(subject_images[s][m]);
  } else {
    // Two-way crossed design: measurements are matched across subjects by identifier
    std::map<std::string, size_t> measurement_index;
    for (size_t i = 0; i != nimages; ++i)
      measurement_index.emplace(measurement_ids[i], measurement_index.size());
    nmeasurements = measurement_index.size();
    if (nmeasurements < 2)
      throw Exception(std::string("ICC computation requires at least two measurements") + //
                      " (only " + str(nmeasurements) + " distinct measurement identifiers provided)");
    layout.resize(nsubjects, nmeasurements);
    layout.setConstant(-1);
    for (size_t i = 0; i != nimages; ++i) {
      const size_t s = subject_index.at(subject_ids[i]);
      const size_t m = measurement_index.at(measurement_ids[i]);
      if (layout(s, m) >= 0)
        throw Exception(std::string("Invalid design:") +                                        //
                        " subject \"" + subject_ids[i] + "\"" +                                 //
                        " has multiple images for measurement \"" + measurement_ids[i] + "\""); //
      layout(s, m) = static_cast<ssize_t>(i);
    }
    for (const auto &[subject, s] : subject_index) {
      for (const auto &[measurement, m] : measurement_index) {
        if (layout(s, m) < 0)
          throw Exception(std::string("Unbalanced design:") +                        //
                          " subject \"" + subject + "\"" +                           //
                          " has no image for measurement \"" + measurement + "\";" + //
                          " every subject must possess exactly one image"            //
                          " for every measurement identifier");                      //
      }
    }
  }

  // Open all input images and verify that their dimensions are identical
  std::vector<Image<float>> images;
  images.reserve(nimages);
  Header out_header;
  bool inputs_are_fixel = false;
  {
    ProgressBar progress("Opening and validating input images", nimages);
    for (const auto &path : image_paths) {
      try {
        Header header = Header::open(path);
        if (images.empty()) {
          inputs_are_fixel = Fixel::is_data_file(header);
          out_header = Header(header);
          out_header.datatype() = DataType::native(DataType::Float32);
          out_header.keyval().clear();
        } else {
          check_dimensions(out_header, header);
          // For volumetric data, additionally require that the voxel grids
          //   coincide in scanner space; fixel data files carry no meaningful
          //   spatial transform and are compared on dimensions alone
          if (!inputs_are_fixel)
            check_voxel_grids_match_in_scanner_space(out_header, header);
        }
        images.push_back(header.get_image<float>());
      } catch (Exception &e) {
        throw Exception(e, "Error opening input image \"" + path.string() + "\"");
      }
      ++progress;
    }
  }

  DEBUG("ICC form: " + MR::Enum::lowercase_name(form));
  DEBUG("Number of input images: " + str(nimages));
  DEBUG("Number of subjects (n): " + str(nsubjects));
  DEBUG("Number of measurements per subject (k): " + str(nmeasurements));

  // The per-element ICC is computed into a scratch buffer first, so that the
  //   whole-image aggregate ICC can be recorded in the header before the buffer
  //   is exported to the filesystem
  Image<float> icc_image = Image<float>::scratch(out_header, "per-element ICC");

  // Compute the chosen ICC form independently for each image element using ANOVA mean squares;
  //   input image values are accessed on demand at the shared grid position
  const double n = static_cast<double>(nsubjects);
  const double k = static_cast<double>(nmeasurements);
  const double df_rows = n - 1.0;
  const double df_cols = k - 1.0;
  const double df_within = n * (k - 1.0);
  const double df_error = df_rows * df_cols;

  // Evaluate the chosen ICC form from a set of ANOVA mean squares.
  //   Every form is a ratio of linear combinations of the mean squares,
  //   with coefficients that depend only on the (element-invariant) design
  //   dimensions n and k; the identical expression is therefore applied both
  //   per-element and, after the loop, to the variance pooled across all elements
  auto icc_from_ms = [&](const double ms_rows,   //
                         const double ms_within, //
                         const double ms_cols,   //
                         const double ms_error) -> double {
    double numerator = 0.0;
    double denominator = 0.0;
    if (one_way) {
      // One-way random effects: only between- and within-subject variation
      numerator = ms_rows - ms_within;
      denominator = (form == form_t::ICC_1_1) ? (ms_rows + df_cols * ms_within) : ms_rows;
    } else {
      // Two-way models: partition residual from the measurement (column) effect
      numerator = ms_rows - ms_error;
      switch (form) {
      case form_t::ICC_2_1:
        denominator = ms_rows + df_cols * ms_error + (k / n) * (ms_cols - ms_error);
        break;
      case form_t::ICC_2_k:
        denominator = ms_rows + (ms_cols - ms_error) / n;
        break;
      case form_t::ICC_3_1:
        denominator = ms_rows + df_cols * ms_error;
        break;
      case form_t::ICC_3_k:
        denominator = ms_rows;
        break;
      default:
        assert(false);
      }
    }
    return (std::fabs(denominator) > 0.0) ? (numerator / denominator) : 0.0;
  };

  // Pool sums of squares across all elements to form a whole-image aggregate ICC.
  //   Constant (zero-variance) elements such as image background contribute
  //   nothing to these sums, and so drop out of the aggregate without masking
  double sum_ss_rows = 0.0;
  double sum_ss_within = 0.0;
  double sum_ss_cols = 0.0;
  double sum_ss_error = 0.0;

  Eigen::MatrixXd table(nsubjects, nmeasurements);
  for (auto l = Loop("Computing per-element " + MR::Enum::lowercase_name(form), icc_image)(icc_image); l; ++l) {
    for (auto &image : images)
      assign_pos_of(icc_image).to(image);
    for (size_t s = 0; s != nsubjects; ++s)
      for (size_t m = 0; m != nmeasurements; ++m)
        table(s, m) = images[layout(s, m)].value();

    const double grand_mean = table.mean();
    const Eigen::VectorXd row_means = table.rowwise().mean();
    const double ss_rows = k * (row_means.array() - grand_mean).square().sum();
    const double ss_total = (table.array() - grand_mean).square().sum();

    double ss_within = 0.0;
    double ss_cols = 0.0;
    double ss_error = 0.0;
    double icc = 0.0;
    if (one_way) {
      ss_within = ss_total - ss_rows;
      icc = icc_from_ms(ss_rows / df_rows, ss_within / df_within, 0.0, 0.0);
    } else {
      const Eigen::VectorXd col_means = table.colwise().mean();
      ss_cols = n * (col_means.array() - grand_mean).square().sum();
      ss_error = ss_total - ss_rows - ss_cols;
      icc = icc_from_ms(ss_rows / df_rows, 0.0, ss_cols / df_cols, ss_error / df_error);
    }
    icc_image.value() = static_cast<float>(icc);

    sum_ss_rows += ss_rows;
    sum_ss_within += ss_within;
    sum_ss_cols += ss_cols;
    sum_ss_error += ss_error;
  }

  // Whole-image aggregate: evaluate the same ICC form on the pooled sums of squares.
  //   As numerator and denominator are each linear in the mean squares, this equals
  //   the denominator-weighted (i.e. variance-weighted) mean of the per-element ICCs
  const double aggregate_icc = icc_from_ms(sum_ss_rows / df_rows,     //
                                           sum_ss_within / df_within, //
                                           sum_ss_cols / df_cols,     //
                                           sum_ss_error / df_error);
  out_header.keyval()["icc_form"] = MR::Enum::lowercase_name(form);
  out_header.keyval()["icc_aggregate"] = str(aggregate_icc);
  CONSOLE("Whole-image aggregate " + MR::Enum::lowercase_name(form) + " = " + str(aggregate_icc));

  // Export the scratch buffer now that the aggregate has been stored in the header
  Image<float> out_image = Image<float>::create(argument[3], out_header);
  threaded_copy(icc_image, out_image);
}
