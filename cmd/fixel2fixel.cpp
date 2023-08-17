/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#include <string>

#include "command.h"
#include "header.h"
#include "image.h"
#include "progressbar.h"
#include "thread_queue.h"
#include "types.h"
#include "algo/copy.h"
#include "fixel/helpers.h"

#include "fixel/correspondence/correspondence.h"
#include "fixel/correspondence/mapping.h"

using namespace MR;
using namespace App;
using namespace MR::Fixel::Correspondence;

constexpr float default_fillvalue = 0.0f;

enum class metric_t { SUM, MEAN, COUNT, ANGLE };
const char* metrics[] = { "sum", "mean", "count", "angle", nullptr };
// TODO Other metrics:
//   - Angle that also takes into account misalignment of multiple source fixels
//     that are mapped to the same target fixel


void usage ()
{
  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Project quantities from one fixel dataset to another";

  DESCRIPTION
  + "This command requires pre-calculation of fixel correspondence between two fixel datasets; "
    "this would most typically be achieved using the fixelcorrespondence command."

  + "The -weighted option does not act as a per-fixel value multipler as is done in the "
    "calculation of the Fibre Density and Cross-section (FDC) measure. Rather, whenever "
    "a quantitative value for a target fixel is to be determined from the aggregation of "
    "multiple source fixels, the fixel data file provided via the -weights option will "
    "be used to modulate the magnitude by which each source fixel contributes to that "
    "aggregate. Most typically this would be a file containing fixel densities / volumes, "
    "if e.g. the value for a low-density source fixel should not contribute as much as a "
    "high-density source fixel in calculation of a weighted mean value for a target fixel.";

  // TODO Should data_in be a directions file if angle is the metric of interest?

  ARGUMENTS
  + Argument ("data_in", "the source fixel data file").type_image_in()
  + Argument ("correspondence", "the directory containing the fixel-fixel correspondence mapping").type_directory_in()
  + Argument ("metric", "the metric to calculate when mapping multiple input fixels to an output fixel; "
                        "options are: " + join(metrics, ", ")).type_choice (metrics)
  + Argument ("directory_out", "the output fixel directory in which the output fixel data file will be placed").type_text()
  + Argument ("data_out", "the name of the output fixel data file").type_text();

  OPTIONS

  + Option ("weighted", "specify fixel data file containing weights to use during aggregation of multiple source fixels")
    + Argument ("weights_in").type_image_in()

  + OptionGroup ("Options relating to filling data values for specific fixels")
  + Option ("fill", "value for output fixels to which no input fixels are mapped "
                    "(default: " + str(default_fillvalue) + ")")
    + Argument ("value").type_float()
  + Option ("nan_many2one", "insert NaN value in cases where multiple input fixels map to the same output fixel")
  + Option ("nan_one2many", "insert NaN value in cases where one input fixel maps to multiple output fixels");

}



struct FillSettings
{
    float value;
    bool nan_many2one, nan_one2many;
};



class Functor
{

  private:
    // TODO In this scenario, the use of a Fixel class will be slightly different:
    // - Initialise with the template fixel
    // - Add subject fixels - need to keep a list
    // - Extract the relevant metric
    // Is a class needed here, or just deal with it in the functor?
  public:
    Functor (const std::string& input_path,
             const Mapping& correspondence,
             const metric_t metric,
             const FillSettings& fill_settings,
             Image<float>& explicit_weights,
             const std::string& output_directory) :
        correspondence (correspondence),
        metric (metric),
        fill (fill_settings),
        explicit_weights (explicit_weights)
    {
      if (Path::is_dir (input_path))
        throw Exception ("Input must be a fixel data file to be mapped, not a fixel directory");
      Header input_header (Header::open (input_path));
      if (!MR::Fixel::is_data_file (input_header))
        throw Exception ("Input image is not a fixel data file");
      if (explicit_weights.valid() && explicit_weights.size(0) != input_header.size(0))
        throw Exception ("Number of fixels in input file (" + str(input_header.size(0)) + ") does not match number of fixels in fixel weights file (" + str(explicit_weights.size(0)) + ")");

      const std::string fixel_directory = MR::Fixel::get_fixel_directory (input_path);
      input_directions = MR::Fixel::find_directions_header (fixel_directory).get_image<float>();
      input_data = input_header.get_image<float>();

      target_directions = MR::Fixel::find_directions_header (output_directory).get_image<float>();
      if (target_directions.size(0) != correspondence.size())
        throw Exception ("Number of fixels in output directory (" + str(target_directions.size(0)) +
                         ") does not match number of lines in fixel correspondence file (" + str(correspondence.size()) + ")");

      Header H_output (target_directions);
      H_output.size(1) = 1;
      output_data = Image<float>::scratch (H_output, "scratch storage of remapped fixel data");

      // Here we need the number of output fixels to which each input fixel maps,
      //   for two reasons:
      //   - If fill.nan_one2many is set, want to detect this as soon as possible,
      //     insert the fill value and exit
      //   - Wherever an input fixel contributes to more than one output fixel, its
      //     volume is effectively "spread" over those fixels; hence it needs to
      //     contribute with less weight
      vector<uint8_t> objectives_per_source_fixel (input_header.size(0), 0);
      for (size_t out_index = 0; out_index != correspondence.size(); ++out_index) {
        for (auto i : correspondence[out_index]) {
          assert (i < input_header.size(0));
          ++objectives_per_source_fixel[i];
        }
      }
      implicit_weights = Image<float>::scratch (input_data, "Implicit weights for source fixels based on multiple objective target fixels");
      for (auto l = Loop(0) (implicit_weights); l; ++l)
        implicit_weights.value() = objectives_per_source_fixel[implicit_weights.index(0)] ?
                                   1.0f / float(objectives_per_source_fixel[implicit_weights.index(0)]) :
                                   0.0f;
    }


    // Input argument is the fixel index of the output file
    bool operator() (const size_t& out_index)
    {
      assert (out_index < correspondence.size());
      output_data.index(0) = out_index;

      const auto& in_indices = correspondence[out_index];
      if (!in_indices.size()) {
        output_data.value() = fill.value;
        return true;
      }
      if (in_indices.size() > 1 && fill.nan_many2one) {
        output_data.value() = std::numeric_limits<float>::quiet_NaN();
        return true;
      }

      // Regardless of which metric we are calculating, still need to
      //   accumulate all of the input fixel data for this output fixel

      vector<dir_t> directions;
      vector<float> values, weights;
      for (auto i : in_indices) {
        // If set up to fill with NaN whenever an input fixel contributes to more than one output fixel,
        //   need to see if any of the input fixels for this output fixel also contribute to at least one
        //   other output fixel
        implicit_weights.index(0) = i;
        if (fill.nan_one2many && implicit_weights.value() < 1.0f) {
          output_data.value() = std::numeric_limits<float>::quiet_NaN();
          return true;
        }
        input_directions.index(0) = i;
        directions.emplace_back (dir_t (input_directions.row(1)));
        input_data.index(0) = i;
        values.push_back (input_data.value());

        if (explicit_weights.valid()) {
          explicit_weights.index(0) = i;
          weights.push_back (implicit_weights.value() * explicit_weights.value());
        } else {
          weights.push_back (implicit_weights.value());
        }
      }

      float result = 0.0f;
      switch (metric) {
        case metric_t::SUM:
          {
            for (size_t i = 0; i != in_indices.size(); ++i)
              result += values[i] * weights[i];
          }
          break;
        case metric_t::MEAN:
          {
            float sum_weights = 0.0f;
            for (size_t i = 0; i != in_indices.size(); ++i) {
              result += values[i] * weights[i];
              sum_weights += weights[i];
            }
          result /= sum_weights;
          }
          break;
        case metric_t::COUNT:
          result = in_indices.size();
          break;
        case metric_t::ANGLE:
          {
            target_directions.index(0) = out_index;
            const dir_t out_dir (target_directions.row(1));
            dir_t mean_dir (0.0f, 0.0f, 0.0f);
            for (size_t i = 0; i != in_indices.size(); ++i)
              mean_dir += directions[i] * weights[i] * (out_dir.dot (directions[i]) < 0.0 ? -1.0f : +1.0f);
            mean_dir.normalize();
            result = std::acos (out_dir.dot (mean_dir));
          }
          break;
      }

      output_data.value() = result;
      return true;
    }



    void save (const std::string& path)
    {
      Image<float> out = Image<float>::create (path, output_data);
      copy (output_data, out);
    }



  private:
    const Mapping& correspondence;
    const metric_t metric;
    const FillSettings& fill;

    Image<float> input_data;
    Image<float> implicit_weights;
    Image<float> explicit_weights;
    Image<float> input_directions;
    Image<float> target_directions;
    Image<float> output_data;

};



class Source
{
  public:
    Source (const size_t i) :
        size (i),
        progress ("remapping fixel data", i),
        counter (0) { }
    bool operator() (size_t& index)
    {
      ++progress;
      return ((index = counter++) < size);
    }

  private:
    const size_t size;
    ProgressBar progress;
    size_t counter;
};




void run()
{
  FillSettings fill_settings;
  fill_settings.value = get_option_value ("fill", default_fillvalue);
  fill_settings.nan_many2one = get_options ("nan_many2one").size();
  fill_settings.nan_one2many = get_options ("nan_one2many").size();

  const std::string input_path (argument[0]);
  const Mapping correspondence ((std::string (argument[1])));
  metric_t metric;
  switch (int(argument[2])) {
    case 0: metric = metric_t::SUM; break;
    case 1: metric = metric_t::MEAN; break;
    case 2: metric = metric_t::COUNT; break;
    case 3: metric = metric_t::ANGLE; break;
  }

  const std::string output_directory (argument[3]);

  if (!Path::is_dir (output_directory))
    throw Exception ("Output fixel directory \"" + output_directory + "\" not found");

  Image<float> explicit_weights;
  auto opt = get_options ("weighted");
  if (opt.size()) {
    explicit_weights = Image<float>::open (opt[0][0]);
    if (!MR::Fixel::is_data_file (explicit_weights))
      throw Exception ("Image provided via -weighted option must be a fixel data file");
  }

  Source source (correspondence.size());
  Functor functor (input_path, correspondence, metric, fill_settings, explicit_weights, output_directory);
  Thread::run_queue (source, Thread::batch (size_t()), Thread::multi (functor));
  functor.save (Path::join(output_directory, argument[4]));
}

