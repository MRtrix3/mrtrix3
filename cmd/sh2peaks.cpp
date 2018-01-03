/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#include "command.h"
#include "math/SH.h"
#include "memory.h"
#include "progressbar.h"
#include "thread_queue.h"
#include "image.h"
#include "algo/loop.h"


#define DOT_THRESHOLD 0.99
#define DEFAULT_NPEAKS 3

using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)";

  SYNOPSIS = "Extract the peaks of a spherical harmonic function at each voxel, by commencing a Newton search along a set of specified directions";

  ARGUMENTS
  + Argument ("SH", "the input image of SH coefficients.")
  .type_image_in ()

  + Argument ("output",
              "the output image. Each volume corresponds to the x, y & z component "
              "of each peak direction vector in turn.")
  .type_image_out ();

  OPTIONS
  + Option ("num", "the number of peaks to extract (default: " + str(DEFAULT_NPEAKS) + ").")
  + Argument ("peaks").type_integer (0)

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
  + Argument ("value").type_float (0.0)

  + Option ("seeds",
            "specify a set of directions from which to start the multiple restarts of "
            "the optimisation (by default, the built-in 60 direction set is used)")
  + Argument ("file").type_file_in()

  + Option ("mask",
            "only perform computation within the specified binary brain mask image.")
  + Argument ("image").type_image_in()

  + Option ("fast",
            "use lookup table to compute associated Legendre polynomials (faster, but approximate).");
}


using value_type = float;



class Direction { MEMALIGN(Direction)
  public:
    Direction () : a (NaN) { }
    Direction (const Direction& d) : a (d.a), v (d.v) { }
    Direction (value_type phi, value_type theta) : a (1.0), v (std::cos (phi) *std::sin (theta), std::sin (phi) *std::sin (theta), std::cos (theta)) { }
    value_type a;
    Eigen::Vector3f v;
    bool operator< (const Direction& d) const {
      return (a > d.a);
    }
};




class Item { MEMALIGN(Item)
  public:
    Eigen::VectorXf data;
    ssize_t pos[3];
};





class DataLoader { MEMALIGN(DataLoader)
  public:
    DataLoader (Image<value_type>& sh_data,
                Image<bool>* mask_data) :
      sh (sh_data),
      loop (Loop("estimating peak directions", 0, 3) (sh)) { }

    bool operator() (Item& item) {
      if (loop) {
        item.data.resize (sh.size(3));
        item.pos[0] = sh.index(0);
        item.pos[1] = sh.index(1);
        item.pos[2] = sh.index(2);

        if (mask) {
          assign_pos_of(sh).to(*mask);
          if (!mask->value()) {
            for (auto l = Loop(3) (sh); l; ++l)
              item.data[sh.index(3)] = NaN;
          }
        } else {
          // iterates over SH coefficients
          for (auto l = Loop(3) (sh); l; ++l)
            item.data[sh.index(3)] = sh.value();
        }

        loop++;

        return true;
      }
      return false;
    }

  private:
    Image<value_type>  sh;
    std::unique_ptr<Image<bool> > mask;
    LoopAlongAxisRangeProgress::Run<Image<value_type> > loop;
};




class Processor { MEMALIGN(Processor)
  public:
    Processor (Image<value_type>& dirs_data,
               Eigen::Matrix<value_type, Eigen::Dynamic, 2>& directions,
               int lmax,
               int npeaks,
               vector<Direction> true_peaks,
               value_type threshold,
               Image<value_type>* ipeaks_data,
               bool use_precomputer) :
      dirs_vox (dirs_data),
      dirs (directions),
      lmax (lmax),
      npeaks (npeaks),
      true_peaks (true_peaks),
      threshold (threshold),
      peaks_out (npeaks),
      ipeaks_vox (ipeaks_data),
      precomputer (use_precomputer ? new Math::SH::PrecomputedAL<value_type> (lmax) :  nullptr) { }

    bool operator() (const Item& item) {

      dirs_vox.index(0) = item.pos[0];
      dirs_vox.index(1) = item.pos[1];
      dirs_vox.index(2) = item.pos[2];

      if (check_input (item)) {
        for (auto l = Loop(3) (dirs_vox); l; ++l)
          dirs_vox.value() = NaN;
        return true;
      }

      vector<Direction> all_peaks;

      for (size_t i = 0; i < size_t(dirs.rows()); i++) {
        Direction p (dirs (i,0), dirs (i,1));
        p.a = Math::SH::get_peak (item.data, lmax, p.v, precomputer);
        if (std::isfinite (p.a)) {
          for (size_t j = 0; j < all_peaks.size(); j++) {
            if (std::abs (p.v.dot (all_peaks[j].v)) > DOT_THRESHOLD) {
              p.a = NaN;
              break;
            }
          }
        }
        if (std::isfinite (p.a) && p.a >= threshold)
          all_peaks.push_back (p);
      }

      if (ipeaks_vox) {
        ipeaks_vox->index(0) = item.pos[0];
        ipeaks_vox->index(1) = item.pos[1];
        ipeaks_vox->index(2) = item.pos[2];

        for (int i = 0; i < npeaks; i++) {
          Eigen::Vector3f p;
          ipeaks_vox->index(3) = 3*i;
          for (int n = 0; n < 3; n++) {
            p[n] = ipeaks_vox->value();
            ipeaks_vox->index(3)++;
          }
          p.normalize();

          value_type mdot = 0.0;
          for (size_t n = 0; n < all_peaks.size(); n++) {
            value_type f = std::abs (p.dot (all_peaks[n].v));
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
            value_type f = std::abs (all_peaks[n].v.dot (true_peaks[i].v));
            if (f > mdot) {
              mdot = f;
              peaks_out[i] = all_peaks[n];
            }
          }
        }
      }
      else std::partial_sort_copy (all_peaks.begin(), all_peaks.end(), peaks_out.begin(), peaks_out.end());

      int actual_npeaks = std::min (npeaks, (int) all_peaks.size());
      dirs_vox.index(3) = 0;
      for (int n = 0; n < actual_npeaks; n++) {
        dirs_vox.value() = peaks_out[n].a*peaks_out[n].v[0];
        dirs_vox.index(3)++;
        dirs_vox.value() = peaks_out[n].a*peaks_out[n].v[1];
        dirs_vox.index(3)++;
        dirs_vox.value() = peaks_out[n].a*peaks_out[n].v[2];
        dirs_vox.index(3)++;
      }
      for (; dirs_vox.index(3) < 3*npeaks; dirs_vox.index(3)++) dirs_vox.value() = NaN;

      return true;
    }

  private:
    Image<value_type> dirs_vox;
    Eigen::Matrix<value_type, Eigen::Dynamic, 2> dirs;
    int lmax, npeaks;
    vector<Direction> true_peaks;
    value_type threshold;
    vector<Direction> peaks_out;
    copy_ptr<Image<value_type> > ipeaks_vox;
    Math::SH::PrecomputedAL<value_type>* precomputer;

    bool check_input (const Item& item) {
      if (ipeaks_vox) {
        ipeaks_vox->index(0) = item.pos[0];
        ipeaks_vox->index(1) = item.pos[1];
        ipeaks_vox->index(2) = item.pos[2];
        ipeaks_vox->index(3) = 0;
        if (std::isnan (value_type (ipeaks_vox->value())))
          return true;
      }

      bool no_peaks = true;
      for (size_t i = 0; i < size_t(item.data.size()); i++) {
        if (std::isnan (item.data[i]))
          return true;
        if (no_peaks)
          if (i && item.data[i] != 0.0)
            no_peaks = false;
      }

      return no_peaks;
    }
};


extern value_type default_directions [];



void run ()
{
  auto SH_data = Image<value_type>::open (argument[0]).with_direct_io (3);
  Math::SH::check (SH_data);

  auto opt = get_options ("mask");

  std::unique_ptr<Image<bool> > mask_data;
  if (opt.size())
    mask_data.reset (new Image<bool>(Image<bool>::open (opt[0][0])));

  opt = get_options ("seeds");
  Eigen::Matrix<value_type, Eigen::Dynamic, 2> dirs;
  if (opt.size())
    dirs = load_matrix<value_type> (opt[0][0]);
  else {
    dirs = Eigen::Map<Eigen::Matrix<value_type, 60, 2> > (default_directions, 60, 2);
  }
  if (dirs.cols() != 2)
    throw Exception ("expecting 2 columns for search directions matrix");

  int npeaks = get_option_value ("num", DEFAULT_NPEAKS);

  opt = get_options ("direction");
  vector<Direction> true_peaks;
  for (size_t n = 0; n < opt.size(); ++n) {
    Direction p (Math::pi*to<float> (opt[n][0]) /180.0, Math::pi*float (opt[n][1]) /180.0);
    true_peaks.push_back (p);
  }
  if (true_peaks.size())
    npeaks = true_peaks.size();

  value_type threshold = get_option_value("threshold", -INFINITY);

  auto header = Header(SH_data);
  header.datatype() = DataType::Float32;

  opt = get_options ("peaks");
  std::unique_ptr<Image<value_type> > ipeaks_data;
  if (opt.size()) {
    if (true_peaks.size())
      throw Exception ("you can't specify both a peaks file and orientations to be estimated at the same time");
    if (opt.size())
      ipeaks_data.reset (new Image<value_type> (Image<value_type>::open(opt[0][0])));

    check_dimensions (SH_data, *ipeaks_data, 0, 3);
    npeaks = ipeaks_data->size (3) / 3;
  }
  header.size(3) = 3 * npeaks;
  auto peaks = Image<value_type>::create (argument[1], header);

  DataLoader loader (SH_data, mask_data.get());
  Processor processor (peaks, dirs, Math::SH::LforN (SH_data.size (3)),
      npeaks, true_peaks, threshold, ipeaks_data.get(), get_options("fast").size());

  Thread::run_queue (loader, Thread::batch (Item()), Thread::multi (processor));
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

