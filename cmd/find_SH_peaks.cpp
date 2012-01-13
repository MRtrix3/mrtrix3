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
#include "math/SH.h"
#include "progressbar.h"
#include "thread/exec.h"
#include "thread/queue.h"
#include "dataset/loop.h"
#include "image/data.h"
#include "image/voxel.h"

MRTRIX_APPLICATION

#define DOT_THRESHOLD 0.99

using namespace MR;
using namespace App;

void usage ()
{
  DESCRIPTION
  + "compute the amplitudes of a spherical harmonic function at each voxel, along the specified directions";

  ARGUMENTS
  + Argument ("SH", "the input image of SH coefficients.")
  .type_image_in ()

  + Argument ("ouput",
              "the output image. Each volume corresponds to the x, y & z component "
              "of each peak direction vector in turn.")
  .type_image_out ();

  OPTIONS
  + Option ("num", "the number of peaks to extract (default is 3).")
  + Argument ("peaks").type_integer (0, 3, std::numeric_limits<int>::max())

  + Option ("direction",
            "the direction of a peak to estimate. The algorithm will attempt to "
            "find the same number of peaks as have been specified using this option.")
  .allow_multiple()
  + Argument ("phi").type_float()
  + Argument ("theta").type_float()

  + Option ("peaks",
            "the program will try to find the peaks that most closely match those "
            "in the image provided.")
  + Argument ("image").type_image_in()

  + Option ("threshold",
            "only peak amplitudes greater than the threshold will be considered.")
  + Argument ("value").type_float()

  + Option ("seeds",
            "specify a set of directions from which to start the multiple restarts of "
            "the optimisation (by default, the built-in 60 direction set is used)")
  + Argument ("file").type_file()

  + Option ("mask",
            "only perform computation within the specified binary brain mask image.")
  + Argument ("image").type_image_in ();
}


typedef float value_type;



class Direction
{
  public:
    Direction () : a (NAN) { }
    Direction (const Direction& d) : a (d.a), v (d.v) { }
    Direction (value_type phi, value_type theta) : a (1.0), v (cos (phi) *sin (theta), sin (phi) *sin (theta), cos (theta)) { }
    value_type a;
    Point<value_type> v;
    bool operator< (const Direction& d) const {
      return (a > d.a);
    }
};




class Item
{
  public:
    Math::Vector<value_type> data;
    ssize_t pos[3];
};





class DataLoader
{
  public:
    DataLoader (Image::Header& sh_header,
                Image::Header* mask_header) :
      sh_data (sh_header),
      sh (sh_data),
      loop ("estimating peak directions...", 0, 3) { 
      if (mask_header) {
        mask_data = new Image::Data<bool> (*mask_header);
        DataSet::check_dimensions (*mask_data, sh, 0, 3);
        mask = new Image::Data<bool>::voxel_type (*mask_data);
        loop.start (*mask, sh);
      }
      else 
        loop.start (sh);
      }

    bool operator() (Item& item) {
      if (loop.ok()) {
        if (mask) {
          while (!mask->value()) {
            loop.next (*mask, sh);
            if (!loop.ok())
              return false;
          }
        }

        item.pos[0] = sh[0];
        item.pos[1] = sh[1];
        item.pos[2] = sh[2];

        item.data.allocate (sh.dim(3));

        DataSet::Loop inner (3); // iterates over SH coefficients
        for (inner.start (sh); inner.ok(); inner.next (sh))
          item.data[sh[3]] = sh.value();

        if (mask)
          loop.next (*mask, sh);
        else
          loop.next (sh);

        return true;
      }
      return false;
    }

  private:
    Image::Data<value_type> sh_data;
    Image::Data<value_type>::voxel_type  sh;
    Ptr<Image::Data<bool> > mask_data;
    Ptr<Image::Data<bool>::voxel_type> mask;
    DataSet::Loop loop;
};



class Processor
{
  public:
    Processor (Image::Header& dirs_header,
               Math::Matrix<value_type>& directions,
               int lmax,
               int npeaks,
               std::vector<Direction> true_peaks,
               value_type threshold,
               Image::Header* ipeaks_header) :
      dirs_data (dirs_header), 
      dirs_vox (dirs_data), 
      dirs (directions),
      lmax (lmax), 
      npeaks (npeaks),
      true_peaks (true_peaks), 
      threshold (threshold),
      ipeaks (ipeaks_header) { }

    bool operator() (const Item& item) {
      Math::Vector<value_type> amplitudes;
      std::vector<Direction> peaks_out (npeaks);

      dirs_vox[0] = item.pos[0];
      dirs_vox[1] = item.pos[1];
      dirs_vox[2] = item.pos[2];

      if (check_input (item)) {
        DataSet::Loop inner (3);
        for (inner.start (dirs_vox); inner.ok(); inner.next (dirs_vox))
          dirs_vox.value() = NAN;
        return true;
      }

      std::vector<Direction> all_peaks;

      for (size_t i = 0; i < dirs.rows(); i++) {
        Direction p (dirs (i,0), dirs (i,1));
        p.a = Math::SH::get_peak (item.data.ptr(), lmax, p.v);
        if (finite (p.a)) {
          for (size_t j = 0; j < all_peaks.size(); j++) {
            if (Math::abs (p.v.dot (all_peaks[j].v)) > DOT_THRESHOLD) {
              p.a = NAN;
              break;
            }
          }
        }
        if (finite (p.a) && p.a >= threshold) all_peaks.push_back (p);
      }

      if (ipeaks) {
        Image::Data<value_type> ipeaks_data (*ipeaks);
        Image::Data<value_type>::voxel_type ipeaks_vox (ipeaks_data);
        ipeaks_vox[0] = item.pos[0];
        ipeaks_vox[1] = item.pos[1];
        ipeaks_vox[2] = item.pos[2];

        for (int i = 0; i < npeaks; i++) {
          Point<value_type> p;
          ipeaks_vox[3] = 3*i;
          for (int n = 0; n < 3; n++) {
            p[n] = ipeaks_vox.value();
            ipeaks_vox[3]++;
          }
          p.normalise();

          value_type mdot = 0.0;
          for (size_t n = 0; n < all_peaks.size(); n++) {
            value_type f = Math::abs (p.dot (all_peaks[n].v));
            if (f > mdot) {
              mdot = f;
              peaks_out[i] = all_peaks[n];
            }
          }
        }
      }
      else if (true_peaks.size()) {
        for (int i = 0; i < npeaks; i++) {
          value_type mdot = 0.0;
          for (size_t n = 0; n < all_peaks.size(); n++) {
            value_type f = Math::abs (all_peaks[n].v.dot (true_peaks[i].v));
            if (f > mdot) {
              mdot = f;
              peaks_out[i] = all_peaks[n];
            }
          }
        }
      }
      else std::partial_sort_copy (all_peaks.begin(), all_peaks.end(), peaks_out.begin(), peaks_out.end());

      int actual_npeaks = MIN (npeaks, (int) all_peaks.size());
      dirs_vox[3] = 0;
      for (int n = 0; n < actual_npeaks; n++) {
        dirs_vox.value() = peaks_out[n].a*peaks_out[n].v[0];
        dirs_vox[3]++;
        dirs_vox.value() = peaks_out[n].a*peaks_out[n].v[1];
        dirs_vox[3]++;
        dirs_vox.value() = peaks_out[n].a*peaks_out[n].v[2];
        dirs_vox[3]++;
      }
      for (; dirs_vox[3] < 3*npeaks; dirs_vox[3]++) dirs_vox.value() = NAN;

      return true;
    }

  private:
    Image::Data<value_type> dirs_data;
    Image::Data<value_type>::voxel_type dirs_vox;
    Math::Matrix<value_type> dirs;
    int lmax, npeaks;
    std::vector<Direction> true_peaks;
    value_type threshold;
    Image::Header* ipeaks;

    bool check_input (const Item& item) {
      if (ipeaks) {
        Image::Data<value_type> ipeaks_data (*ipeaks);
        Image::Data<value_type>::voxel_type ipeaks_vox (ipeaks_data);
        ipeaks_vox[0] = item.pos[0];
        ipeaks_vox[1] = item.pos[1];
        ipeaks_vox[2] = item.pos[2];
        ipeaks_vox[3] = 0;
        if (isnan (ipeaks_vox.value()))
          return (true);
      }

      bool no_peaks = true;
      for (size_t i = 0; i < item.data.size(); i++) {
        if (isnan (item.data[i]))
          return (true);
        if (no_peaks)
          if (i && item.data[i] != 0.0) no_peaks = false;
      }

      return (no_peaks);
    }
};



extern value_type default_directions [];



void run ()
{
  Image::Header sh_header (argument[0]);
  assert (!sh_header.is_complex());

  if (sh_header.ndim() != 4)
    throw Exception ("spherical harmonic image should contain 4 dimensions");

  Options opt = get_options ("mask");

  Ptr<Image::Header> mask_header;
  if (opt.size())
    mask_header = new Image::Header (opt[0][0]);

  opt = get_options ("seeds");
  Math::Matrix<value_type> dirs;
  if (opt.size())
    dirs.load (opt[0][0]);
  else {
    dirs.allocate (60,2);
    dirs = Math::Matrix<value_type> (default_directions, 60, 2);
  }
  if (dirs.columns() != 2)
    throw Exception ("expecting 2 columns for search directions matrix");

  opt = get_options ("num");
  int npeaks = opt.size() ? opt[0][0] : 3;

  opt = get_options ("direction");
  std::vector<Direction> true_peaks;
  for (size_t n = 0; n < opt.size(); ++n) {
    Direction p (M_PI*to<float> (opt[n][0]) /180.0, M_PI*float (opt[n][1]) /180.0);
    true_peaks.push_back (p);
  }
  if (true_peaks.size()) npeaks = true_peaks.size();

  opt = get_options ("threshold");
  value_type threshold = -INFINITY;
  if (opt.size())
    threshold = opt[0][0];

  Image::Header directions_header (sh_header);
  directions_header.set_datatype (DataType::Float32);

  opt = get_options ("peaks");
  Ptr<Image::Header> ipeaks_header;
  if (opt.size()) {
    if (true_peaks.size())
      throw Exception ("you can't specify both a peaks file and orientations to be estimated at the same time");
    if (opt.size())
      ipeaks_header = new Image::Header (opt[0][0]);

    if (ipeaks_header->dim (0) != directions_header.dim (0) ||
        ipeaks_header->dim (1) != directions_header.dim (1) ||
        ipeaks_header->dim (2) != directions_header.dim (2))
      throw Exception ("dimensions of peaks image \"" + ipeaks_header->name() +
          "\" do not match that of SH coefficients image \"" + directions_header.name() + "\"");
    npeaks = ipeaks_header->dim (3) / 3;
  }
  directions_header.set_dim (3, 3 * npeaks);

  directions_header.create (argument[1]);

  DataLoader loader (sh_header, mask_header);
  Processor processor (directions_header, dirs, Math::SH::LforN (sh_header.dim (3)),
      npeaks, true_peaks, threshold, ipeaks_header);

  Thread::run_queue (loader, 1, Item(), processor, 0);
}


value_type default_directions [] = {
  0, 0,
  -3.14159, 1.3254,
  -2.58185, 1.50789,
  2.23616, 1.46585,
  0.035637, 0.411961,
  2.65836, 0.913741,
  0.780743, 1.23955,
  -0.240253, 1.58088,
  -0.955334, 1.08447,
  1.12534, 1.78765,
  1.12689, 1.30126,
  0.88512, 1.55615,
  2.08019, 1.16222,
  0.191423, 1.06076,
  1.29453, 0.707568,
  2.794, 1.24245,
  2.02138, 0.337172,
  1.59186, 1.30164,
  -2.83601, 0.910221,
  0.569095, 0.96362,
  3.05336, 1.00206,
  2.4406, 1.19129,
  0.437969, 1.30795,
  0.247623, 0.728643,
  -0.193887, 1.0467,
  -1.34638, 1.14233,
  1.35977, 1.54693,
  1.82433, 0.660035,
  -0.766769, 1.3685,
  -2.02757, 1.02063,
  -0.78071, 0.667313,
  -1.47543, 1.45516,
  -1.10765, 1.38916,
  -1.65789, 0.871848,
  1.89902, 1.44647,
  3.08122, 0.336433,
  -2.35317, 1.25244,
  2.54757, 0.586206,
  -2.14697, 0.338323,
  3.10764, 0.670594,
  1.75238, 0.991972,
  -1.21593, 0.82585,
  -0.259942, 0.71572,
  -1.51829, 0.549286,
  2.22968, 0.851973,
  0.979108, 0.954864,
  1.36274, 1.04186,
  -0.0104792, 1.33716,
  -0.891568, 0.33526,
  -2.0635, 0.68273,
  -2.41353, 0.917031,
  2.57199, 1.50166,
  0.965936, 0.33624,
  0.763244, 0.657346,
  -2.61583, 0.606725,
  -0.429332, 1.30226,
  -2.91118, 1.56901,
  -2.79822, 1.24559,
  -1.70453, 1.20406,
  -0.582782, 0.975235
};

