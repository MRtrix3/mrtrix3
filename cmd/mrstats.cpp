/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */


#include <iomanip>
#include <vector>

#include "command.h"
#include "datatype.h"
#include "image.h"
#include "image_helpers.h"
#include "memory.h"
#include "algo/loop.h"
#include "file/ofstream.h"
#include "stats.h"


using namespace MR;
using namespace App;


void usage ()
{
AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)";

DESCRIPTION
  + "compute images statistics.";

ARGUMENTS
  + Argument ("image",
  "the input image from which statistics will be computed.")
  .type_image_in ();

OPTIONS
  + Stats::Options

  + Option ("voxel",
  "only perform computation within the specified voxel, supplied as a "
  "comma-separated vector of 3 integer values (multiple voxels can be included).")
  .allow_multiple()
  + Argument ("pos").type_sequence_int ()

  + Option ("position",
  "dump the position of the voxels in the mask to a text file.")
  + Argument ("file").type_file_out ();

}


typedef Stats::value_type value_type;
typedef Stats::complex_type complex_type;

class Volume_loop { NOMEMALIGN
  public:
    Volume_loop (Image<complex_type>& in) :
        image (in),
        is_4D (in.ndim() == 4),
        status (true)
    {
      if (is_4D)
        image.index(3) = 0;
    }

    void operator++ ()
    {
      if (is_4D) {
        image.index(3)++;
      } else {
        assert (status);
        status = false;
      }
    }
    operator bool() const
    {
      if (is_4D)
        return (image.index(3) >= 0 && image.index(3) < image.size(3));
      else
        return status;
    }

  private:
    Image<complex_type>& image;
    const bool is_4D;
    bool status;
};



void run ()
{

  auto header = Header::open (argument[0]);
  const bool is_complex = header.datatype().is_complex();
  auto data = header.get_image<complex_type>();
  if (data.ndim() > 4)
    throw Exception ("mrstats is not designed to handle images greater than 4D");

  auto inner_loop = Loop(0, 3);

  std::unique_ptr<File::OFStream> dumpstream, hist_stream, position_stream;

  auto opt = get_options ("histogram");
  if (opt.size()) {
    if (is_complex)
      throw Exception ("histogram generation not supported for complex data types");
    hist_stream.reset (new File::OFStream (opt[0][0]));
  }

  int nbins = DEFAULT_HISTOGRAM_BINS;
  opt = get_options ("bins");
  if (opt.size())
    nbins = opt[0][0];
  Stats::CalibrateHistogram calibrate (nbins);

  opt = get_options ("dump");
  if (opt.size())
    dumpstream.reset (new File::OFStream (opt[0][0]));

  opt = get_options ("position");
  if (opt.size())
    position_stream.reset (new File::OFStream (opt[0][0]));

  std::vector<std::string> fields;
  opt = get_options ("output");
  for (size_t n = 0; n < opt.size(); ++n) 
    fields.push_back (opt[n][0]);

  bool header_shown (!App::log_level || fields.size());

  auto voxels = get_options ("voxel");

  opt = get_options ("mask");
  if (opt.size()) { // within mask:

    if (voxels.size())
      throw Exception ("cannot use mask with -voxel option");

    auto mask = Image<bool>::open (opt[0][0]);
    check_dimensions (mask, data, 0, 3);

    if (hist_stream) {
      ProgressBar progress ("calibrating histogram", voxel_count (data));
      for (auto i = Volume_loop (data); i; ++i) {
        for (auto j = inner_loop (mask, data); j; ++j) {
          if (mask.value())
            calibrate (complex_type(data.value()).real());
          ++progress;
        }
      }
      calibrate.init (*hist_stream);
    }

    for (auto i = Volume_loop (data); i; ++i) {
      Stats::Stats stats (is_complex);

      if (dumpstream)
        stats.dump_to (*dumpstream);

      if (hist_stream)
        stats.generate_histogram (calibrate);

      for (auto j = inner_loop (mask, data); j; ++j) {
        if (mask.value() > 0.5) {
          stats (data.value());
          if (position_stream) {
            for (size_t i = 0; i < data.ndim(); ++i)
              *position_stream << data.index(i) << " ";
            *position_stream << "\n";
          }
        }
      }

      if (!header_shown)
        Stats::print_header (is_complex);
      header_shown = true;

      stats.print (data, fields);

      if (hist_stream)
        stats.write_histogram (*hist_stream);
    }

    return;
  }




  if (!voxels.size()) { // whole data set:

    if (hist_stream) {
      ProgressBar progress ("calibrating histogram", voxel_count (data));
      for (auto i = Volume_loop (data); i; ++i) {
        for (auto j = inner_loop (data); j; ++j) {
          calibrate (complex_type(data.value()).real());
          ++progress;
        }
      }
      calibrate.init (*hist_stream);
    }

    for (auto l = Volume_loop (data); l; ++l) {
      Stats::Stats stats (is_complex);

      if (dumpstream)
        stats.dump_to (*dumpstream);

      if (hist_stream)
        stats.generate_histogram (calibrate);

      for (auto j = inner_loop (data); j; ++j) {
        stats (data.value());
        if (position_stream) {
          for (size_t i = 0; i < data.ndim(); ++i)
            *position_stream << data.index(i) << " ";
          *position_stream << "\n";
        }
      }

      if (!header_shown)
        Stats::print_header (is_complex);
      header_shown = true;

      stats.print (data, fields);

      if (hist_stream)
        stats.write_histogram (*hist_stream);
    }

    return;
  }




  // voxels:

  std::vector<Eigen::Array3i> voxel (voxels.size());
  for (size_t i = 0; i < voxels.size(); ++i) {
    std::vector<int> x = parse_ints (voxels[i][0]);
    if (x.size() != 3)
      throw Exception ("vector positions must be supplied as x,y,z");
    if (x[0] < 0 || x[0] >= data.size (0) ||
        x[1] < 0 || x[1] >= data.size (1) ||
        x[2] < 0 || x[2] >= data.size (2))
      throw Exception ("voxel at [ " + str (x[0]) + " " + str (x[1])
                       + " " + str (x[2]) + " ] is out of bounds");
    voxel[i] = { x[0], x[1], x[2] };
  }

  if (hist_stream) {
    ProgressBar progress ("calibrating histogram", voxel.size());
    for (auto i = Volume_loop (data); i; ++i) {
      for (size_t i = 0; i < voxel.size(); ++i) {
        data.index(0) = voxel[i][0];
        data.index(1) = voxel[i][1];
        data.index(2) = voxel[i][2];
        calibrate (complex_type(data.value()).real());
        ++progress;
      }
    }
    calibrate.init (*hist_stream);
  }

  for (auto i = Volume_loop (data); i; ++i) {
    Stats::Stats stats (is_complex);

    if (dumpstream)
      stats.dump_to (*dumpstream);

    if (hist_stream)
      stats.generate_histogram (calibrate);

    for (size_t i = 0; i < voxel.size(); ++i) {
      data.index(0) = voxel[i][0];
      data.index(1) = voxel[i][1];
      data.index(2) = voxel[i][2];
      stats (data.value());
      if (position_stream) {
        for (size_t i = 0; i < data.ndim(); ++i)
          *position_stream << data.index(i) << " ";
        *position_stream << "\n";
      }
    }

    if (!header_shown)
      Stats::print_header (is_complex);
    header_shown = true;

    stats.print (data, fields);

    if (hist_stream)
      stats.write_histogram (*hist_stream);
  }
}

