/* Copyright (c) 2008-2017 the MRtrix3 contributors
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
#include "__mrtrix_plugin.h"

#include <vector>

#include "command.h"
#include "progressbar.h"
#include "memory.h"

#include "file/ofstream.h"

#include "math/median.h"

#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/weights.h"




using namespace MR;
using namespace App;
using namespace MR::DWI;
using namespace MR::DWI::Tractography;



void usage ()
{

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Calculate statistics on streamlines length";

  ARGUMENTS
  + Argument ("tracks_in", "the input track file").type_tracks_in();

  OPTIONS
  + Option ("histogram", "output a histogram of streamline lengths")
    + Argument ("path").type_file_out()

  + Option ("dump", "dump the streamlines lengths to a text file")
    + Argument ("path").type_file_out()

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
  float max_length = 0.0f;
  double sum_lengths = 0.0, sum_weights = 0.0;
  vector<double> histogram;
  vector<LW> all_lengths;
  all_lengths.reserve (header_count);

  {
    Tractography::Properties properties;
    Tractography::Reader<float> reader (argument[0], properties);

    if (properties.find ("count") != properties.end())
      header_count = to<size_t> (properties["count"]);

    if (properties.find ("output_step_size") != properties.end())
      step_size = to<float> (properties["output_step_size"]);
    else
      step_size = to<float> (properties["step_size"]);
    if (!std::isfinite (step_size) || !step_size) {
      WARN ("Streamline step size undefined in header");
      if (get_options ("histogram").size())
        WARN ("Histogram will be henerated using a 1mm interval");
    }

    std::unique_ptr<File::OFStream> dump;
    auto opt = get_options ("dump");
    if (opt.size())
      dump.reset (new File::OFStream (std::string(opt[0][0]), std::ios_base::out | std::ios_base::trunc));

    ProgressBar progress ("Reading track file", header_count);
    Streamline<> tck;
    while (reader (tck)) {
      ++count;
      const float length = std::isfinite (step_size) ? tck.calc_length (step_size) : tck.calc_length();
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
      }
      if (dump)
        (*dump) << length << "\n";
      ++progress;
    }
  }

  if (histogram.front())
    WARN ("read " + str(histogram.front()) + " zero-length tracks");
  if (count != header_count)
    WARN ("expected " + str(header_count) + " tracks according to header; read " + str(count));

  const float mean_length = sum_lengths / sum_weights;

  float median_length = 0.0f;
  if (weights_provided) {
    // Perform a weighted median calculation
    std::sort (all_lengths.begin(), all_lengths.end());
    size_t median_index = 0;
    double sum = sum_weights - all_lengths[0].get_weight();
    while (sum > 0.5 * sum_weights) { sum -= all_lengths[++median_index].get_weight(); }
    median_length = all_lengths[median_index].get_length();
  } else {
    median_length = Math::median (all_lengths).get_length();
  }

  double stdev = 0.0;
  for (vector<LW>::const_iterator i = all_lengths.begin(); i != all_lengths.end(); ++i)
    stdev += i->get_weight() * Math::pow2 (i->get_length() - mean_length);
  stdev = std::sqrt (stdev / (((count - 1) / float(count)) * sum_weights));

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

  auto opt = get_options ("histogram");
  if (opt.size()) {
    File::OFStream out (opt[0][0], std::ios_base::out | std::ios_base::trunc);
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
