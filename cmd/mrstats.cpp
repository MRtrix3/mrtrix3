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

#include "app.h"
#include "image/voxel.h"
#include "dataset/loop.h"

using namespace std; 
using namespace MR; 

SET_VERSION_DEFAULT;
SET_AUTHOR (NULL);
SET_COPYRIGHT (NULL);

DESCRIPTION = {
  "compute images statistics.",
  NULL
};

ARGUMENTS = {

  Argument ("image", 
      "the input image from which statistics will be computed.")
    .type_image_in (),

  Argument ()
};


OPTIONS = { 

  Option ("mask",
      "only perform computation within the specified binary brain mask image.")
    + Argument ("image").type_image_in (),

  Option ("histogram", 
      "generate histogram of intensities and store in specified text file. Note "
      "that the first line of the histogram gives the centre of the bins.")
    + Argument ("file").type_file (),

  Option ("bins", 
      "the number of bins to use to generate the histogram (default = 100).")
    + Argument ("num").type_integer (2, 100, std::numeric_limits<int>::max()),

  Option ("dump",
      "dump the voxel intensities to a text file.")
    + Argument ("file").type_file (),

  Option ("position",
      "dump the position of the voxels in the mask to a text file.")
    + Argument ("file").type_file (),

  Option ()
};


typedef float value_type;


class CalibrateHistogram {
  public:
    CalibrateHistogram (int nbins) : min (INFINITY), max (-INFINITY), width (0.0), bins (nbins) { }

    value_type min, max, width;
    int bins;

    void operator() (value_type val) 
    {
      if (finite (val)) {
        if (val < min) min = val;
        if (val > max) max = val;
      }
    }

    void init (std::ostream& stream) 
    {
      width = (max - min) / float (bins+1);
      for (int i = 0; i < bins; i++)
        stream << (min + width/2.0) + i*width << " ";
      stream << "\n";
    }
};



class Stats 
{
  public:
    Stats () : mean (0.0), std (0.0), min (INFINITY), max (-INFINITY), count (0), dump (NULL) { }

    void generate_histogram (const CalibrateHistogram& cal)
    {
      hmin = cal.min;
      hwidth = cal.width;
      hist.resize (cal.bins);
    }

    void dump_to (std::ostream& stream) { dump = &stream; }

    void write_histogram (std::ostream& stream)
    {
      for (size_t i = 0; i < hist.size(); ++i)
        stream << hist[i] << " ";
      stream << "\n";
    }


    void operator() (value_type val) 
    {
      if (finite (val)) {
        mean += val;
        std += val*val;
        if (min > val) min = val;
        if (max < val) max = val;
        count++;

        if (dump) 
          *dump << val << "\n";
        
        if (hist.size()) {
          int bin = int ((val-hmin) / hwidth);
          if (bin < 0) 
            bin = 0;
          else if (bin >= int(hist.size())) 
            bin = hist.size()-1;
          hist[bin]++;
        }
      }
    }

    void print (Image::Voxel<value_type>& ima) 
    {
      if (count == 0) 
        throw Exception ("no voxels in mask - aborting");

      mean /= double(count);
      std = sqrt(std/double(count) - mean*mean);

      std::string s = "[ ";
      for (size_t n = 3; n < ima.ndim(); n++) 
        s += str(ima[n]) + " ";
      s += "] ";

      MR::print (MR::printf ("%-15s %-11g %-11g %-11g %-11g %-11d\n", 
            s.c_str(), mean, std, min, max, count));
    }

  private:
    double mean, std;
    value_type min, max;
    size_t count;
    value_type hmin, hwidth;
    std::vector<size_t> hist;
    std::ostream* dump;
};



const char* header_string = "channel         mean        std. dev.   min         max         count\n";


EXECUTE {
  Image::Header header (argument[0]);
  Image::Voxel<value_type> vox (header);

  DataSet::Loop inner_loop (0, 3);
  DataSet::Loop outer_loop (3);

  bool header_shown (!App::log_level);
  Ptr<std::ostream> dumpstream, hist_stream, position_stream;

  Options opt = get_options ("histogram");
  if (opt.size()) {
    hist_stream = new std::ofstream (opt[0][0]);
    if (!*hist_stream) 
      throw Exception (std::string("error opening histogram file \"") + opt[0][0] + "\": " + strerror (errno));
  }

  int nbins = 100;
  opt = get_options ("bins");
  if (opt.size())
    nbins = to<int>(opt[0][0]);
  CalibrateHistogram calibrate (nbins);

  
  opt = get_options ("dump");
  if (opt.size()) {
    dumpstream = new std::ofstream (opt[0][0]);
    if (!*dumpstream) 
      throw Exception (std::string("error opening dump file \"") + opt[0][0] + "\": " + strerror (errno));
  }
  
  opt = get_options ("position");
  if (opt.size()) {
    position_stream = new std::ofstream (opt[0][0]);
    if (!*position_stream) 
      throw Exception (std::string("error opening positions file \"") + opt[0][0] + "\": " + strerror (errno));
  }


  opt = get_options ("mask");
  if (opt.size()) {

    Image::Header mask_header (opt[0][0]);

    if (mask_header.dim(0) != header.dim(0) || 
        mask_header.dim(1) != header.dim(1) || 
        mask_header.dim(2) != header.dim(2)) 
      throw Exception ("dimensions of mask image do not match that of data image - aborting");

    Image::Voxel<value_type> mask (mask_header);

    if (hist_stream) {
      ProgressBar progress ("calibrating histogram...", DataSet::voxel_count (vox));
      for (outer_loop.start (vox); outer_loop.ok(); outer_loop.next (vox)) {
        for (inner_loop.start (mask, vox); inner_loop.ok(); inner_loop.next (mask, vox)) {
          if (mask.value() > 0.5) 
            calibrate (vox.value());
          ++progress;
        }
      }
      calibrate.init (*hist_stream);
    }

    for (outer_loop.start (vox); outer_loop.ok(); outer_loop.next (vox)) {
      Stats stats;

      if (dumpstream) 
        stats.dump_to (*dumpstream);

      if (hist_stream)
        stats.generate_histogram (calibrate);

      for (inner_loop.start (mask, vox); inner_loop.ok(); inner_loop.next (mask, vox)) {
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
        print (header_string);
      header_shown = true;

      stats.print (vox);

      if (hist_stream)
        stats.write_histogram (*hist_stream);
    }

  }
  else {

    if (hist_stream) {
      ProgressBar progress ("calibrating histogram...", DataSet::voxel_count (vox));
      for (outer_loop.start (vox); outer_loop.ok(); outer_loop.next (vox)) {
        for (inner_loop.start (vox); inner_loop.ok(); inner_loop.next (vox)) {
          calibrate (vox.value());
          ++progress;
        }
      }
      calibrate.init (*hist_stream);
    }

    for (outer_loop.start (vox); outer_loop.ok(); outer_loop.next (vox)) {
      Stats stats;

      if (dumpstream) 
        stats.dump_to (*dumpstream);

      if (hist_stream)
        stats.generate_histogram (calibrate);

      for (inner_loop.start (vox); inner_loop.ok(); inner_loop.next (vox)) {
        stats (vox.value());
        if (position_stream) {
          for (size_t i = 0; i < vox.ndim(); ++i)
            *position_stream << vox[i] << " ";
          *position_stream << "\n";
        }
      }

      if (!header_shown) 
        print (header_string);
      header_shown = true;

      stats.print (vox);

      if (hist_stream)
        stats.write_histogram (*hist_stream);
    }

  }

}

