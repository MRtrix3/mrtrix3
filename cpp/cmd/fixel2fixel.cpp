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

#include <filesystem>
#include <string>
#include <string_view>

#include "algo/copy.h"
#include "command.h"
#include "enum.h"
#include "fixel/helpers.h"
#include "header.h"
#include "image.h"
#include "progressbar.h"
#include "thread_queue.h"
#include "types.h"

#include "fixel/correspondence/correspondence.h"
#include "fixel/correspondence/mapping.h"

using namespace MR;
using namespace App;
using namespace MR::Fixel::Correspondence;

constexpr float default_fillvalue = 0.0f;

enum class metric_t { SUM, MEAN, COUNT, ANGLE };
// Other potential metrics:
//   - Angle that also takes into account misalignment of multiple source fixels
//     that are mapped to the same target fixel

// clang-format off
void usage() {

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Project a fixel-wise quantity from one fixel dataset to another";

  DESCRIPTION
  + "This command requires pre-calculation of fixel correspondence between two fixel datasets;"
    " this would most typically be achieved using the fixelcorrespondence command,"
    " which produces a .npz file as input to this command."

  + "The -weighted option does not act as a per-fixel value multipler"
    " as is done in the calculation of the Fibre Density and Cross-section (FDC) measure."
    " Rather, whenever a quantitative value for a target fixel is to be determined"
    " from the aggregation of multiple source fixels,"
    " the fixel data file provided via the -weights option will be used"
    " to modulate the magnitude by which each source fixel contributes to that aggregate."
    " Most typically this would be a file containing fixel densities / volumes,"
    " if e.g. the value for a low-density source fixel should not contribute"
    " as much as a high-density source fixel in calculation of a weighted mean value for a target fixel.";

  // TODO Should data_in be a directions file if angle is the metric of interest?

  EXAMPLES
  + Example("Project a fixel-wise additive measure (such as fibre density)",
            "fixel2fixel subject/fd.mif fixelmapping.npz sum template subject_fd.mif",
            "For a measure that is naturally additive,"
            " such as is the case for a fibre density measure relating to axonal volume,"
            " the 'sum' metric should be used;"
            " ie. if two subject fixels map to a single template fixel,"
            " then the fibre density ascribed to that template fixel"
            " should be the sum of the fibre densities of the two subject fixels.")

  + Example("Project a fixel-wise non-additive measure (such as axonal diameter)",
            "fixel2fixel subject/ad.mif fixelmapping.npz mean template subject_ad.mif"
            " -weighted subject/fd.mif",
            "For some fixel-wise measures,"
            " such as axonal diameter in this example,"
            " it would not be suitable to sum those measures across multiple fixels."
            " Eg. if two subject fixels, with ascribed axonal diameters of 2um and 4um,"
            " needed to be merged in order to map to a single template fixel,"
            " then it would not be suitable to assign an axonal diameter of 6um to the template fixel;"
            " in the absence of any other information,"
            " a value of 3um would intuitively be more suitable."
            " This can be further improved by specifying the -weighted option,"
            " providing as input a fixel data file encoding some form of fibre density:"
            " if eg. the 4um fixel is three times more dense than the 2um fixel,"
            " then the weighted mean value projected to the template fixel should be 3.5um.")

  + Example("Replicate the behaviour of the fixelcorrespondence command from MRtrix version 3.0.x",
            "fixelcorrespondence subject/fd.mif template/fd.mif fixelmapping.npz -algorithm legacy;"
            " fixel2fixel subject/fd.mif fixelmapping.npz sum fd_template subject.mif -ignore_weights",
            "To reproduce the behaviour of the 3.0.x version of the fixelcorrespondence command"
            " requires two explicit modifications to the default behaviours"
            " of both the new fixelcorrespondence command and command fixel2fixel."
            " When running the new fixelcorrespondence command,"
            " matching algorithm 'legacy' must be used;"
            " this simply chooses for each template fixel the nearest subject fixel,"
            " provided that it is within some maximal angular distance."
            " However if multiple template fixels were to select the same subject fixel,"
            " the entirety of the fibre density in that fixel would be projected to both template fixels."
            " Under the default behaviour of command fixel2fixel,"
            " the fibre density of that subject fixel would instead be split between those two template fixels."
            " Option -ignore_weights disables that behaviour,"
            " thereby manifesting the same (potentially undesirable) behaviour of the earlier software."
            " Note that this example is provided for understanding and backwards compatibility"
            " and should not be interpreted as explicit advocacy for its use.");

  ARGUMENTS
  + Argument("data_in",
             "the source fixel data file").type_image_in()
  + Argument("correspondence",
             "the .npz file containing the fixel-fixel correspondence mapping").type_file_in()
  + Argument("metric",
             "the metric to calculate when mapping multiple input fixels to an output fixel;"
             " options are: " + Enum::join<metric_t>()).type_choice<metric_t>()
  + Argument("directory_out",
             "the output fixel directory in which the output fixel data file will be placed").type_text()
  + Argument("data_out",
             "the name of the output fixel data file").type_text();

  OPTIONS
  + Option("weighted",
           "specify fixel data file containing weights to use during aggregation of multiple source fixels")
    + Argument("weights_in").type_image_in()

  + Option("ignore_weights",
           "do not apply the fixel-fixel mapping weights as stored in the correspondence data file")

  + OptionGroup("Options relating to filling data values for specific fixels")
    + Option("fill",
             "value for output fixels to which no input fixels are mapped"
             " (default: " + str(default_fillvalue) + ")")
      + Argument("value").type_float()
    + Option("nan_many2one",
             "insert NaN value in cases where multiple input fixels map to the same output fixel")
    + Option("nan_one2many",
             "insert NaN value in cases where one input fixel maps to multiple output fixels");
}
// clang-format off

struct FillSettings {
  float value;
  bool nan_many2one, nan_one2many;
};

class Functor {

private:
  // TODO In this scenario, the use of a Fixel class will be slightly different:
  // - Initialise with the template fixel
  // - Add subject fixels - need to keep a list
  // - Extract the relevant metric
  // Is a class needed here, or just deal with it in the functor?
public:
  Functor(const std::filesystem::path &input_path,
          const Mapping &correspondence,
          const metric_t metric,
          const FillSettings &fill_settings,
          Image<float> &explicit_weights,
          const std::filesystem::path &output_directory)
      : correspondence(correspondence),
        metric(metric),
        fill(fill_settings),
        ignore_weights(!get_options("ignore_weights").empty()),
        explicit_weights(explicit_weights) {
    if (std::filesystem::is_directory(input_path))
      throw Exception("Input must be a fixel data file to be mapped, not a fixel directory");
    Header input_header(Header::open(input_path));
    if (!MR::Fixel::is_data_file(input_header))
      throw Exception("Input image is not a fixel data file");
    if (explicit_weights.valid() && explicit_weights.size(0) != input_header.size(0))
      throw Exception("Number of fixels in input file (" + str(input_header.size(0)) + ")" + //
                      " does not match number of fixels in fixel weights file" +             //
                      " (" + str(explicit_weights.size(0)) + ")");                           //

    const std::filesystem::path fixel_directory = MR::Fixel::get_fixel_directory(input_path);
    input_directions = MR::Fixel::find_directions_header(fixel_directory).get_image<float>();
    input_data = input_header.get_image<float>();

    target_directions = MR::Fixel::find_directions_header(output_directory).get_image<float>();
    if (target_directions.size(0) != correspondence.size())
      throw Exception("Number of fixels in output directory (" + str(target_directions.size(0)) + ")" + //
                      " does not match number of lines in fixel correspondence file" +                  //
                      " (" + str(correspondence.size()) + ")");                                         //

    Header H_output(target_directions);
    H_output.size(1) = 1;
    output_data = Image<float>::scratch(H_output, "scratch storage of remapped fixel data");
  }

  // Input argument is the fixel index of the output file
  bool operator()(const size_t &out_index) {
    assert(out_index < correspondence.size());
    output_data.index(0) = out_index;

    const auto &in_entries = correspondence[out_index];
    if (in_entries.empty()) {
      output_data.value() = fill.value;
      return true;
    }
    if (in_entries.size() > 1 && fill.nan_many2one) {
      output_data.value() = std::numeric_limits<float>::quiet_NaN();
      return true;
    }

    // Regardless of which metric we are calculating, still need to
    //   accumulate all of the input fixel data for this output fixel

    std::vector<dir_t> directions;
    std::vector<float> values, weights;
    for (const auto &e : in_entries) {
      // Detect source fixels that contribute to more than one target (weight < 1)
      if (fill.nan_one2many && e.weight < 1.0f) {
        output_data.value() = std::numeric_limits<float>::quiet_NaN();
        return true;
      }
      input_directions.index(0) = e.index;
      directions.emplace_back(dir_t(input_directions.row(1)));
      input_data.index(0) = e.index;
      values.push_back(input_data.value());

      const float agg_weight = ignore_weights ? 1.0f : e.weight;
      if (explicit_weights.valid()) {
        explicit_weights.index(0) = e.index;
        weights.push_back(agg_weight * explicit_weights.value());
      } else {
        weights.push_back(agg_weight);
      }
    }

    float result = 0.0f;
    switch (metric) {
    case metric_t::SUM: {
      for (size_t i = 0; i != in_entries.size(); ++i)
        result += values[i] * weights[i];
    } break;
    case metric_t::MEAN: {
      float sum_weights = 0.0f;
      for (size_t i = 0; i != in_entries.size(); ++i) {
        result += values[i] * weights[i];
        sum_weights += weights[i];
      }
      result /= sum_weights;
    } break;
    case metric_t::COUNT:
      result = in_entries.size();
      break;
    case metric_t::ANGLE: {
      target_directions.index(0) = out_index;
      const dir_t out_dir(target_directions.row(1));
      dir_t mean_dir(0.0f, 0.0f, 0.0f);
      for (size_t i = 0; i != in_entries.size(); ++i)
        mean_dir += directions[i] * weights[i] * (out_dir.dot(directions[i]) < 0.0 ? -1.0f : +1.0f);
      mean_dir.normalize();
      result = std::acos(out_dir.dot(mean_dir));
    } break;
    }

    output_data.value() = result;
    return true;
  }

  void save(const std::filesystem::path &path) {
    Image<float> out = Image<float>::create(path, output_data);
    copy(output_data, out);
  }

private:
  const Mapping &correspondence;
  const metric_t metric;
  const FillSettings &fill;
  const bool ignore_weights;

  Image<float> input_data;
  Image<float> explicit_weights;
  Image<float> input_directions;
  Image<float> target_directions;
  Image<float> output_data;
};

class Source {
public:
  Source(const size_t i) : size(i), progress("remapping fixel data", i), counter(0) {}
  bool operator()(size_t &index) {
    ++progress;
    return ((index = counter++) < size);
  }

private:
  const size_t size;
  ProgressBar progress;
  size_t counter;
};

void run() {
  FillSettings fill_settings;
  fill_settings.value = get_option_value("fill", default_fillvalue);
  fill_settings.nan_many2one = get_options("nan_many2one").size();
  fill_settings.nan_one2many = get_options("nan_one2many").size();

  const std::filesystem::path input_path(argument[0]);
  const Mapping correspondence(argument[1]);
  const metric_t metric = MR::Enum::from_name<metric_t>(argument[2]);
  // "directory_out" / "data_out" are declared type_text(); take the raw string
  //   via as_text() before constructing a path (matching voxel2fixel), as a direct
  //   filesystem::path cast of a non-filesystem argument type trips a runtime assertion
  const std::filesystem::path output_directory(argument[3].as_text());
  const std::filesystem::path output_filename(argument[4].as_text());

  if (!std::filesystem::is_directory(output_directory))
    throw Exception("Output fixel directory \"" + output_directory.string() + "\" not found");

  Image<float> explicit_weights;
  auto opt = get_options("weighted");
  if (opt.size()) {
    explicit_weights = Image<float>::open(opt[0][0]);
    if (!MR::Fixel::is_data_file(explicit_weights))
      throw Exception("Image provided via -weighted option must be a fixel data file");
  }

  Source source(correspondence.size());
  Functor functor(input_path, correspondence, metric, fill_settings, explicit_weights, output_directory);
  Thread::run_queue(source, Thread::batch(size_t()), Thread::multi(functor));
  functor.save(output_directory / output_filename);
}
