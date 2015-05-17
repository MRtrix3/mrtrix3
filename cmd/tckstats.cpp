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


#include <vector>

#include "command.h"
#include "progressbar.h"
#include "memory.h"

#include "file/ofstream.h"

#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/weights.h"




using namespace MR;
using namespace App;
using namespace MR::DWI;
using namespace MR::DWI::Tractography;



void usage ()
{

  AUTHOR = "Robert E. Smith (r.smith@brain.org.au)";

  DESCRIPTION
  + "calculate statistics on streamlines length.";

  ARGUMENTS
  + Argument ("tracks_in", "the input track file").type_file_in();

  OPTIONS
  + Option ("length_hist", "output a histogram of streamline lengths")
    + Argument ("path").type_file_out()

  + Option ("dump", "dump the streamlines lengths to a text file")
    + Argument ("path").type_file_out()

  + Tractography::TrackWeightsInOption;

};



void run ()
{

  const bool weights_provided = get_options ("tck_weights_in").size();

  float step_size = 1.0;
  size_t count = 0, header_count = 0;
  std::vector<double> histogram;

  {
    Tractography::Properties properties;
    Tractography::Reader reader (argument[0], properties);

    header_count = properties["count"].empty() ? 0 : to<size_t> (properties["count"]);

    if (properties.find ("output_step_size") != properties.end())
      step_size = to<float> (properties["output_step_size"]);
    else
      step_size = to<float> (properties["step_size"]);
    if (!std::isfinite (step_size) || !step_size) {
      WARN ("Streamline step size undefined; setting to unity");
      step_size = 1.0;
    }

    std::unique_ptr<File::OFStream> dump;
    auto opt = get_options ("dump");
    if (opt.size())
      dump.reset (new File::OFStream (std::string(opt[0][0]), std::ios_base::out | std::ios_base::trunc));

    ProgressBar progress ("Reading track file... ", header_count);
    Tractography::Streamline tck;
    while (reader (tck)) {
      ++count;
      const size_t length = tck.size() ? (tck.size()-1) : 0;
      while (histogram.size() <= length)
        histogram.push_back (0.0);
      histogram[length] += tck.weight;
      if (dump)
        (*dump) << (length * step_size) << "\n";
      ++progress;
    }
  }

  if (histogram.front())
    WARN ("read " + str(histogram.front()) + " zero-length tracks");
  if (count != header_count)
    WARN ("expected " + str(header_count) + " tracks according to header; read " + str(count));

  size_t min = histogram.size();
  double total = 0.0, mean = 0.0;
  for (size_t i = 0; i != histogram.size(); ++i) {
    total += histogram[i];
    mean += i * histogram[i];
    if (histogram[i] && min == histogram.size())
      min = i;
  }
  mean /= total;
  double stdev = 0.0;
  for (size_t i = 0; i != histogram.size(); ++i)
    stdev += histogram[i] * Math::pow2 (i - mean);
  stdev = std::sqrt(stdev / total);
  size_t median = 0;
  double running_sum = 0.0, prev_sum = 0.0;
  do {
    prev_sum = running_sum;
    running_sum += histogram[median++];
  } while (running_sum < 0.5*total);
  --median;
  if (std::abs (prev_sum - (0.5*total)) < std::abs (running_sum - (0.5*total)))
    --median;
  const size_t max = histogram.size() - 1;

  const size_t width = 12;

  std::cout << " " << std::setw(width) << std::right << "mean"
            << " " << std::setw(width) << std::right << "median"
            << " " << std::setw(width) << std::right << "std. dev."
            << " " << std::setw(width) << std::right << "min"
            << " " << std::setw(width) << std::right << "max"
            << " " << std::setw(width) << std::right << "count\n";

  std::cout << " " << std::setw(width) << std::right << (step_size * mean)
            << " " << std::setw(width) << std::right << (step_size * median)
            << " " << std::setw(width) << std::right << (step_size * stdev)
            << " " << std::setw(width) << std::right << (step_size * min)
            << " " << std::setw(width) << std::right << (step_size * max)
            << " " << std::setw(width) << std::right << (total) << "\n";

  auto opt = get_options ("length_hist");
  if (opt.size()) {
    File::OFStream out (opt[0][0], std::ios_base::out | std::ios_base::trunc);
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
