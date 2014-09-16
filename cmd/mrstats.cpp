/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

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

#include <iomanip>
#include <vector>

#include "command.h"
#include "point.h"
#include "ptr.h"
#include "file/ofstream.h"
#include "image/voxel.h"
#include "image/buffer.h"
#include "image/loop.h"
#include "math/median.h"


using namespace MR;
using namespace App;

const char * field_choices[] = { "mean", "median", "std", "min", "max", "count", NULL };

void usage () {
DESCRIPTION
  + "compute images statistics.";

ARGUMENTS
  + Argument ("image",
  "the input image from which statistics will be computed.")
  .type_image_in ();

OPTIONS
  + Option ("output",
      "output only the field specified. Multiple such options can be supplied if required. "
      "Choices are: " + join (field_choices, ", ") + ". Useful for use in scripts.").allow_multiple()
  +   Argument ("field").type_choice (field_choices)

  + Option ("mask",
  "only perform computation within the specified binary brain mask image.")
  + Argument ("image").type_image_in ()

  + Option ("voxel",
  "only perform computation within the specified voxel, supplied as a "
  "comma-separated vector of 3 integer values (multiple voxels can be included).")
  .allow_multiple()
  + Argument ("pos").type_sequence_int ()

  + Option ("histogram",
  "generate histogram of intensities and store in specified text file. Note "
  "that the first line of the histogram gives the centre of the bins.")
  + Argument ("file").type_file_out ()

  + Option ("bins",
  "the number of bins to use to generate the histogram (default = 100).")
  + Argument ("num").type_integer (2, 100, std::numeric_limits<int>::max())

  + Option ("dump",
  "dump the voxel intensities to a text file.")
  + Argument ("file").type_file_out ()

  + Option ("position",
  "dump the position of the voxels in the mask to a text file.")
  + Argument ("file").type_file_out ();

}


typedef float value_type;
typedef cfloat complex_type;


class CalibrateHistogram
{
  public:
    CalibrateHistogram (int nbins) : min (INFINITY), max (-INFINITY), width (0.0), bins (nbins) { }

    value_type min, max, width;
    int bins;

    void operator() (value_type val) {
      if (std::isfinite (val)) {
        if (val < min) min = val;
        if (val > max) max = val;
      }
    }

    void init (std::ostream& stream) {
      width = (max - min) / float (bins+1);
      for (int i = 0; i < bins; i++)
        stream << (min + width/2.0) + i* width << " ";
      stream << "\n";
    }
};



class Stats
{
  public:
    Stats (bool is_complex = false) :
      mean (0.0, 0.0),
      std (0.0, 0.0),
      min (INFINITY, INFINITY),
      max (-INFINITY, -INFINITY),
      count (0),
      dump (NULL),
      is_complex (is_complex) { }

    void generate_histogram (const CalibrateHistogram& cal) {
      hmin = cal.min;
      hwidth = cal.width;
      hist.resize (cal.bins);
    }

    void dump_to (std::ostream& stream) {
      dump = &stream;
    }

    void write_histogram (std::ostream& stream) {
      for (size_t i = 0; i < hist.size(); ++i)
        stream << hist[i] << " ";
      stream << "\n";
    }


    void operator() (complex_type val) {
      if (std::isfinite (val.real()) && std::isfinite (val.imag())) {
        mean += val;
        std += cdouble (val.real()*val.real(), val.imag()*val.imag());
        if (min.real() > val.real()) min = complex_type (val.real(), min.imag());
        if (min.imag() > val.imag()) min = complex_type (min.real(), val.imag());
        if (max.real() < val.real()) max = complex_type (val.real(), max.imag());
        if (max.imag() < val.imag()) max = complex_type (max.real(), val.imag());
        count++;

        if (dump)
          *dump << str(val) << "\n";

        if (!is_complex)
          values.push_back(val.real());


        if (hist.size()) {
          int bin = int ( (val.real()-hmin) / hwidth);
          if (bin < 0)
            bin = 0;
          else if (bin >= int (hist.size()))
            bin = hist.size()-1;
          hist[bin]++;
        }
      }
    }

    template <class Set> void print (Set& ima, const std::vector<std::string>& fields) {

      if (count) {
        mean /= double (count);
        std = complex_type (sqrt (std.real()/double(count) - mean.real()*mean.real()),
            sqrt (std.imag()/double(count) - mean.imag()*mean.imag()));
      }

      std::sort (values.begin(), values.end());

      if (fields.size()) {
        if (!count) 
          return;
        for (size_t n = 0; n < fields.size(); ++n) {
          if (fields[n] == "mean") std::cout << str(mean) << " ";
          else if (fields[n] == "median") std::cout << Math::median (values) << " ";
          else if (fields[n] == "std") std::cout << str(std) << " ";
          else if (fields[n] == "min") std::cout << str(min) << " ";
          else if (fields[n] == "max") std::cout << str(max) << " ";
          else if (fields[n] == "count") std::cout << count << " ";
        }
        std::cout << "\n";

      }
      else {

        std::string s = "[ ";
        if (ima.ndim() > 3) 
          for (size_t n = 3; n < ima.ndim(); n++)
            s += str (ima[n]) + " ";
        else 
          s += "0 ";
        s += "] ";

        int width = is_complex ? 24 : 12;
        std::cout << std::setw(15) << std::right << s << " ";

        std::cout << std::setw(width) << std::right << ( count ? str(mean) : "N/A" );

        if (!is_complex) {
          std::cout << " " << std::setw(width) << std::right << ( count ? str(Math::median (values)) : "N/A" );
        }
        std::cout << " " << std::setw(width) << std::right << ( count > 1 ? str(std) : "N/A" )
          << " " << std::setw(width) << std::right << ( count ? str(min) : "N/A" )
          << " " << std::setw(width) << std::right << ( count ? str(max) : "N/A" )
          << " " << std::setw(12) << std::right << count << "\n";
      }

    }

  private:
    cdouble mean, std;
    complex_type min, max;
    size_t count;
    value_type hmin, hwidth;
    std::vector<size_t> hist;
    std::ostream* dump;
    bool is_complex;
    std::vector<float> values;
};



void print_header (bool is_complex)
{
  int width = is_complex ? 24 : 12;
  std::cout << std::setw(15) << std::right << "channel"
    << " " << std::setw(width) << std::right << "mean";
  if (!is_complex)
    std::cout << " " << std::setw(width) << std::right << "median";
  std::cout  << " " << std::setw(width) << std::right << "std. dev."
    << " " << std::setw(width) << std::right << "min"
    << " " << std::setw(width) << std::right << "max"
    << " " << std::setw(12) << std::right << "count\n";
}


void run () {
  Image::Buffer<complex_type> data (argument[0]);
  Image::Buffer<complex_type>::voxel_type vox (data);

  Image::Loop inner_loop (0, 3);
  Image::Loop outer_loop (3);

  Ptr<File::OFStream> dumpstream, hist_stream, position_stream;

  Options opt = get_options ("histogram");
  if (opt.size()) {
    if (data.datatype().is_complex())
      throw Exception ("histogram generation not supported for complex data types");
    hist_stream = new File::OFStream (opt[0][0]);
  }

  int nbins = 100;
  opt = get_options ("bins");
  if (opt.size())
    nbins = opt[0][0];
  CalibrateHistogram calibrate (nbins);


  opt = get_options ("dump");
  if (opt.size())
    dumpstream = new File::OFStream (opt[0][0]);

  opt = get_options ("position");
  if (opt.size())
    position_stream = new File::OFStream (opt[0][0]);

  std::vector<std::string> fields;
  opt = get_options ("output");
  for (size_t n = 0; n < opt.size(); ++n) 
    fields.push_back (opt[n][0]);

  bool header_shown (!App::log_level || fields.size());

  Options voxels = get_options ("voxel");

  opt = get_options ("mask");
  if (opt.size()) { // within mask:

    if (voxels.size())
      throw Exception ("cannot use mask with -voxel option");

    Image::Buffer<bool> mask_data (opt[0][0]);
    check_dimensions (mask_data, data, 0, 3);
    Image::Buffer<bool>::voxel_type mask (mask_data);

    if (hist_stream) {
      ProgressBar progress ("calibrating histogram...", Image::voxel_count (vox));
      for (auto i = outer_loop (vox); i; ++i) {
        for (auto j = inner_loop (mask, vox); j; ++j) {
          if (mask.value())
            calibrate (complex_type(vox.value()).real());
          ++progress;
        }
      }
      calibrate.init (*hist_stream);
    }

    for (auto i = outer_loop (vox); i; ++i) {
      Stats stats (vox.datatype().is_complex());

      if (dumpstream)
        stats.dump_to (*dumpstream);

      if (hist_stream)
        stats.generate_histogram (calibrate);

      for (auto j = inner_loop (mask, vox); j; ++j) {
        if (mask.value() > 0.5) {
          stats (vox.value());
          if (position_stream) {
            for (size_t i = 0; i < vox.ndim(); ++i)
              *position_stream << vox[i] << " ";
            *position_stream << "\n";
          }
        }
      }

      if (!header_shown)
        print_header (vox.datatype().is_complex());
      header_shown = true;

      stats.print (vox, fields);

      if (hist_stream)
        stats.write_histogram (*hist_stream);
    }

    return;
  }




  if (!voxels.size()) { // whole data set:

    if (hist_stream) {
      ProgressBar progress ("calibrating histogram...", Image::voxel_count (vox));
      for (auto i = outer_loop (vox); i; ++i) {
        for (auto j = inner_loop (vox); j; ++j) {
          calibrate (complex_type(vox.value()).real());
          ++progress;
        }
      }
      calibrate.init (*hist_stream);
    }

    for (auto l = outer_loop (vox); l; ++l) {
      Stats stats (vox.datatype().is_complex());

      if (dumpstream)
        stats.dump_to (*dumpstream);

      if (hist_stream)
        stats.generate_histogram (calibrate);

      for (auto j = inner_loop (vox); j; ++j) {
        stats (vox.value());
        if (position_stream) {
          for (size_t i = 0; i < vox.ndim(); ++i)
            *position_stream << vox[i] << " ";
          *position_stream << "\n";
        }
      }

      if (!header_shown)
        print_header (vox.datatype().is_complex());
      header_shown = true;

      stats.print (vox, fields);

      if (hist_stream)
        stats.write_histogram (*hist_stream);
    }

    return;
  }




  // voxels:

  std::vector<Point<ssize_t> > voxel (voxels.size());
  for (size_t i = 0; i < voxels.size(); ++i) {
    std::vector<int> x = parse_ints (voxels[i][0]);
    if (x.size() != 3)
      throw Exception ("vector positions must be supplied as x,y,z");
    if (x[0] < 0 || x[0] >= vox.dim (0) ||
        x[1] < 0 || x[1] >= vox.dim (1) ||
        x[2] < 0 || x[2] >= vox.dim (2))
      throw Exception ("voxel at [ " + str (x[0]) + " " + str (x[1])
                       + " " + str (x[2]) + " ] is out of bounds");
    voxel[i].set (x[0], x[1], x[2]);
  }

  if (hist_stream) {
    ProgressBar progress ("calibrating histogram...", voxel.size());
    for (auto i = outer_loop (vox); i; ++i) {
      for (size_t i = 0; i < voxel.size(); ++i) {
        vox[0] = voxel[i][0];
        vox[1] = voxel[i][1];
        vox[2] = voxel[i][2];
        calibrate (complex_type(vox.value()).real());
        ++progress;
      }
    }
    calibrate.init (*hist_stream);
  }

  for (auto i = outer_loop (vox); i; ++i) {
    Stats stats (vox.datatype().is_complex());

    if (dumpstream)
      stats.dump_to (*dumpstream);

    if (hist_stream)
      stats.generate_histogram (calibrate);

    for (size_t i = 0; i < voxel.size(); ++i) {
      vox[0] = voxel[i][0];
      vox[1] = voxel[i][1];
      vox[2] = voxel[i][2];
      stats (vox.value());
      if (position_stream) {
        for (size_t i = 0; i < vox.ndim(); ++i)
          *position_stream << vox[i] << " ";
        *position_stream << "\n";
      }
    }

    if (!header_shown)
      print_header (vox.datatype().is_complex());
    header_shown = true;

    stats.print (vox, fields);

    if (hist_stream)
      stats.write_histogram (*hist_stream);
  }
}

