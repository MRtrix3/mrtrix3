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

#include "command.h"
#include "memory.h"
#include "progressbar.h"
#include "types.h"

#include "file/ofstream.h"

#include "math/median.h"

#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/weights.h"




using namespace MR;
using namespace App;
using namespace MR::DWI;
using namespace MR::DWI::Tractography;


// TODO Make compatible with stats generic options?
// - Some features would not be compatible due to potential presence of track weights


const char * field_choices[] = { "mean", "median", "std", "min", "max", "count", NULL };


void usage ()
{

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Calculate statistics on streamlines lengths";

  ARGUMENTS
  + Argument ("tracks_in", "the input track file").type_tracks_in();

  OPTIONS

  + Option ("output",
      "output only the field specified. Multiple such options can be supplied if required. "
      "Choices are: " + join (field_choices, ", ") + ". Useful for use in scripts.").allow_multiple()
    + Argument ("field").type_choice (field_choices)

  + Option ("histogram", "output a histogram of streamline lengths")
    + Argument ("path").type_file_out()

  + Option ("dump", "dump the streamlines lengths to a text file")
    + Argument ("path").type_file_out()

  + Option ("ignorezero", "do not generate a warning if the track file contains streamlines with zero length")

  + Tractography::TrackWeightsInOption;

}


// Store length and weight of each streamline
class LW { NOMEMALIGN
  public:
    LW (const float l, const float w) : length (l), weight (w) { }
    LW () : length (NaN), weight (NaN) { }
    bool operator< (const LW& that) const { return length < that.length; }
    float get_length() const { return length; }
    float get_weight() const { return weight; }
  private:
    float length, weight;
    friend LW operator+ (const LW&, const LW&);
    friend LW operator/ (const LW&, const double);
};

// Necessary for median template function
LW operator+ (const LW& one, const LW& two)
{
  return LW (one.get_length() + two.get_length(), one.get_weight() + two.get_weight());
}

LW operator/ (const LW& lw, const double div)
{
  return LW (lw.get_length() / div, lw.get_weight() / div);
}


void run ()
{

  const bool weights_provided = get_options ("tck_weights_in").size();

  float step_size = NaN;
  size_t count = 0, header_count = 0;
  float min_length = std::numeric_limits<float>::infinity();
  float max_length = -std::numeric_limits<float>::infinity();
  size_t empty_streamlines = 0, zero_length_streamlines = 0;
  default_type sum_lengths = 0.0, sum_weights = 0.0;
  vector<default_type> histogram;
  vector<LW> all_lengths;
  all_lengths.reserve (header_count);

  {
    Tractography::Properties properties;
    Tractography::Reader<float> reader (argument[0], properties);

    if (properties.find ("count") != properties.end())
      header_count = to<size_t> (properties["count"]);

    step_size = properties.get_stepsize();
    if ((!std::isfinite (step_size) || !step_size) && get_options ("histogram").size()) {
      WARN ("Do not have streamline step size with which to bin histogram; histogram will be generated using 1mm bin widths");
    }

    std::unique_ptr<File::OFStream> dump;
    auto opt = get_options ("dump");
    if (opt.size())
      dump.reset (new File::OFStream (std::string(opt[0][0]), std::ios_base::out | std::ios_base::trunc));

    ProgressBar progress ("Reading track file", header_count);
    Streamline<> tck;
    while (reader (tck)) {
      ++count;
      const float length = Tractography::length (tck);
      if (std::isfinite (length)) {
        min_length = std::min (min_length, length);
        max_length = std::max (max_length, length);
        sum_lengths += tck.weight * length;
        sum_weights += tck.weight;
        all_lengths.push_back (LW (length, tck.weight));
        const size_t index = std::isfinite (step_size) ? std::round (length / step_size) : std::round (length);
        while (histogram.size() <= index)
          histogram.push_back (0.0);
        histogram[index] += tck.weight;
        if (!length)
          ++zero_length_streamlines;
      } else {
        ++empty_streamlines;
      }
      if (dump)
        (*dump) << length << "\n";
      ++progress;
    }
  }

  if (!get_options ("ignorezero").size() && (empty_streamlines || zero_length_streamlines)) {
    std::string s ("read");
    if (empty_streamlines) {
      s += " " + str(empty_streamlines) + " empty streamlines";
      if (zero_length_streamlines)
        s += " and";
    }
    if (zero_length_streamlines)
      s += " " + str(zero_length_streamlines) + " streamlines with zero length (one vertex only)";
    WARN (s);
  }
  if (count != header_count)
    WARN ("expected " + str(header_count) + " tracks according to header; read " + str(count));
  if (!std::isfinite (min_length))
    min_length = NaN;
  if (!std::isfinite (max_length))
    max_length = NaN;

  const float mean_length = sum_weights ? (sum_lengths / sum_weights) : NaN;

  float median_length = 0.0f;
  if (count) {
    if (weights_provided) {
      // Perform a weighted median calculation
      std::sort (all_lengths.begin(), all_lengths.end());
      size_t median_index = 0;
      default_type sum = sum_weights - all_lengths[0].get_weight();
      while (sum > 0.5 * sum_weights) { sum -= all_lengths[++median_index].get_weight(); }
      median_length = all_lengths[median_index].get_length();
    } else {
      median_length = Math::median (all_lengths).get_length();
    }
  } else {
    median_length = NaN;
  }

  default_type ssd = 0.0;
  for (vector<LW>::const_iterator i = all_lengths.begin(); i != all_lengths.end(); ++i)
    ssd += i->get_weight() * Math::pow2 (i->get_length() - mean_length);
  const float stdev = sum_weights ? (std::sqrt (ssd / (((count - 1) / default_type(count)) * sum_weights))) : NaN;

  vector<std::string> fields;
  auto opt = get_options ("output");
  for (size_t n = 0; n < opt.size(); ++n)
    fields.push_back (opt[n][0]);

  if (fields.size()) {

    for (size_t n = 0; n < fields.size(); ++n) {
      if (fields[n] == "mean")        std::cout << str(mean_length) << " ";
      else if (fields[n] == "median") std::cout << str(median_length) << " ";
      else if (fields[n] == "std")    std::cout << str(stdev) << " ";
      else if (fields[n] == "min")    std::cout << str(min_length) << " ";
      else if (fields[n] == "max")    std::cout << str(max_length) << " ";
      else if (fields[n] == "count")  std::cout << count << " ";
    }
    std::cout << "\n";

  } else {

    const size_t width = 12;

    std::cout << " " << std::setw(width) << std::right << "mean"
              << " " << std::setw(width) << std::right << "median"
              << " " << std::setw(width) << std::right << "std. dev."
              << " " << std::setw(width) << std::right << "min"
              << " " << std::setw(width) << std::right << "max"
              << " " << std::setw(width) << std::right << "count\n";

    std::cout << " " << std::setw(width) << std::right << (mean_length)
              << " " << std::setw(width) << std::right << (median_length)
              << " " << std::setw(width) << std::right << (stdev)
              << " " << std::setw(width) << std::right << (min_length)
              << " " << std::setw(width) << std::right << (max_length)
              << " " << std::setw(width) << std::right << (count) << "\n";

  }

  opt = get_options ("histogram");
  if (opt.size()) {
    File::OFStream out (opt[0][0], std::ios_base::out | std::ios_base::trunc);
    out << "# " << App::command_history_string << "\n";
    if (!std::isfinite (step_size))
      step_size = 1.0f;
    if (weights_provided) {
      out << "Length,Sum_weights\n";
      for (size_t i = 0; i != histogram.size(); ++i)
        out << str(i * step_size) << "," << str(histogram[i]) << "\n";
    } else {
      out << "Length,Count\n";
      for (size_t i = 0; i != histogram.size(); ++i)
        out << str(i * step_size) << "," << str<size_t>(histogram[i]) << "\n";
    }
    out << "\n";
  }

}
