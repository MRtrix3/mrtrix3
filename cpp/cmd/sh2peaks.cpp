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

#include <array>

#include "algo/loop.h"
#include "command.h"
#include "dwi/directions/predefined.h"
#include "file/matrix.h"
#include "image.h"
#include "math/SH.h"
#include "memory.h"
#include "progressbar.h"
#include "thread_queue.h"

using namespace MR;
using namespace App;

using value_type = float;

constexpr value_type dotproduct_threshold = 0.99F;
constexpr ssize_t default_npeaks = 3;

// clang-format off
void usage() {

  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)";

  SYNOPSIS = "Extract the peaks of a spherical harmonic function in each voxel";

  DESCRIPTION
  + "Peaks of the spherical harmonic function in each voxel are located"
    " by commencing a Newton search along each of a set of pre-specified directions."

  + "Within the output image,"
    " each successive triplet of volumes encodes the x, y & z components of a 3-vector;"
    " their directions in 3D space encode the orientation of the identified peaks,"
    " while the norm of each vector encodes the magnitude of the peaks.";

  DESCRIPTION
  + Math::SH::encoding_description;

  ARGUMENTS
  + Argument ("SH", "the input image of SH coefficients.").type_image_in()
  + Argument ("output", "the output peaks image").type_image_out();

  OPTIONS
  + Option ("num", "the number of peaks to extract"
                   " (default: " + str(default_npeaks) + ").")
    + Argument ("peaks").type_integer (0)

  + Option ("direction",
            "the direction of a peak to estimate."
            " The algorithm will attempt to find the same number of peaks"
            " as have been specified using this option.").allow_multiple()
    + Argument ("phi").type_float()
    + Argument ("theta").type_float()

  + Option ("peaks",
            "the program will try to find the peaks that most closely match those"
            " in the image provided.")
    + Argument ("image").type_image_in()

  + Option ("threshold",
            "only peak amplitudes greater than the threshold will be considered.")
    + Argument ("value").type_float(0.0)

  + Option ("seeds",
            "specify a set of directions from which to start the multiple restarts of the optimisation"
            " (by default, the built-in 60 direction set is used)")
    + Argument ("file").type_file_in()

  + Option ("mask",
            "only perform computation within the specified binary brain mask image.")
    + Argument ("image").type_image_in()

  + Option ("fast",
            "use lookup table to compute associated Legendre polynomials"
            " (faster, but approximate).");

  REFERENCES
  + "Jeurissen, B.; Leemans, A.; Tournier, J.-D.; Jones, D.K.; Sijbers, J. "
    "Investigating the prevalence of complex fiber configurations in white matter tissue"
    " with diffusion magnetic resonance imaging. "
    "Human Brain Mapping, 2013, 34(11), 2747-2766";
}
// clang-format on

class Direction {
public:
  Direction() : a(NaNF) {}
  Direction(const Direction &d) : a(d.a), v(d.v) {}
  Direction(value_type phi, value_type theta)
      : a(1.0), v(std::cos(phi) * std::sin(theta), std::sin(phi) * std::sin(theta), std::cos(theta)) {}
  value_type a;
  Eigen::Vector3f v;
  bool operator<(const Direction &d) const { return (a > d.a); }
};

class Item {
public:
  Eigen::VectorXf data;
  std::array<ssize_t, 3> pos;
};

class DataLoader {
public:
  DataLoader(Image<value_type> &sh_data, const Image<bool> &mask_data)
      : sh(sh_data), mask(mask_data), loop(Loop("estimating peak directions", 0, 3)(sh)) {}

  bool operator()(Item &item) {
    if (loop) {
      item.data.resize(sh.size(3));
      item.pos[0] = sh.index(0);
      item.pos[1] = sh.index(1);
      item.pos[2] = sh.index(2);

      if (mask.valid())
        assign_pos_of(sh, 0, 3).to(mask);
      if (mask.valid() && !mask.value()) {
        for (auto l = Loop(3)(sh); l; ++l)
          item.data[sh.index(3)] = NaNF;
      } else {
        // iterates over SH coefficients
        for (auto l = Loop(3)(sh); l; ++l)
          item.data[sh.index(3)] = sh.value();
      }

      loop++;

      return true;
    }
    return false;
  }

private:
  Image<value_type> sh;
  Image<bool> mask;
  LoopAlongAxisRangeProgress::Run<Image<value_type>> loop;
};

class Processor {
public:
  Processor(Image<value_type> &dirs_data,
            Eigen::Matrix<value_type, Eigen::Dynamic, 2> &directions,
            int lmax,
            int npeaks,
            std::vector<Direction> true_peaks,
            value_type threshold,
            Image<value_type> ipeaks_data,
            bool use_precomputer)
      : dirs_vox(dirs_data),
        dirs(directions),
        lmax(lmax),
        npeaks(npeaks),
        true_peaks(true_peaks),
        threshold(threshold),
        peaks_out(npeaks),
        ipeaks_vox(ipeaks_data),
        precomputer(use_precomputer ? std::make_shared<Math::SH::PrecomputedAL<value_type>>(lmax) : nullptr) {}

  bool operator()(const Item &item) {

    dirs_vox.index(0) = item.pos[0];
    dirs_vox.index(1) = item.pos[1];
    dirs_vox.index(2) = item.pos[2];

    if (check_input(item)) {
      for (auto l = Loop(3)(dirs_vox); l; ++l)
        dirs_vox.value() = NaNF;
      return true;
    }

    std::vector<Direction> all_peaks;

    for (decltype(dirs)::Index i = 0; i < dirs.rows(); i++) {
      Direction p(dirs(i, 0), dirs(i, 1));
      p.a = Math::SH::get_peak(item.data, lmax, p.v, precomputer.get());
      if (std::isfinite(p.a)) {
        for (size_t j = 0; j < all_peaks.size(); j++) {
          if (std::fabs(p.v.dot(all_peaks[j].v)) > dotproduct_threshold) {
            p.a = NaNF;
            break;
          }
        }
      }
      if (std::isfinite(p.a) && p.a >= threshold)
        all_peaks.push_back(p);
    }

    if (ipeaks_vox.valid()) {
      ipeaks_vox.index(0) = item.pos[0];
      ipeaks_vox.index(1) = item.pos[1];
      ipeaks_vox.index(2) = item.pos[2];

      for (int i = 0; i < npeaks; i++) {
        Eigen::Vector3f p;
        ipeaks_vox.index(3) = 3 * i;
        for (int n = 0; n < 3; n++) {
          p[n] = ipeaks_vox.value();
          ipeaks_vox.index(3)++;
        }
        p.normalize();

        value_type mdot = 0.0;
        for (size_t n = 0; n < all_peaks.size(); n++) {
          value_type f = std::fabs(p.dot(all_peaks[n].v));
          if (f > mdot) {
            mdot = f;
            peaks_out[i] = all_peaks[n];
          }
        }
      }
    } else if (!true_peaks.empty()) {
      for (int i = 0; i < npeaks; i++) {
        value_type mdot = 0.0;
        for (size_t n = 0; n < all_peaks.size(); n++) {
          value_type f = std::fabs(all_peaks[n].v.dot(true_peaks[i].v));
          if (f > mdot) {
            mdot = f;
            peaks_out[i] = all_peaks[n];
          }
        }
      }
    } else {
      std::partial_sort_copy(all_peaks.begin(), all_peaks.end(), peaks_out.begin(), peaks_out.end());
    }

    int actual_npeaks = std::min(npeaks, static_cast<int>(all_peaks.size()));
    dirs_vox.index(3) = 0;
    for (int n = 0; n < actual_npeaks; n++) {
      dirs_vox.value() = peaks_out[n].a * peaks_out[n].v[0];
      dirs_vox.index(3)++;
      dirs_vox.value() = peaks_out[n].a * peaks_out[n].v[1];
      dirs_vox.index(3)++;
      dirs_vox.value() = peaks_out[n].a * peaks_out[n].v[2];
      dirs_vox.index(3)++;
    }
    for (; dirs_vox.index(3) < 3 * npeaks; dirs_vox.index(3)++)
      dirs_vox.value() = NaNF;

    return true;
  }

private:
  Image<value_type> dirs_vox;
  Eigen::Matrix<value_type, Eigen::Dynamic, 2> dirs;
  int lmax, npeaks;
  std::vector<Direction> true_peaks;
  value_type threshold;
  std::vector<Direction> peaks_out;
  Image<value_type> ipeaks_vox;
  std::shared_ptr<Math::SH::PrecomputedAL<value_type>> precomputer;

  bool check_input(const Item &item) {
    if (ipeaks_vox.valid()) {
      ipeaks_vox.index(0) = item.pos[0];
      ipeaks_vox.index(1) = item.pos[1];
      ipeaks_vox.index(2) = item.pos[2];
      ipeaks_vox.index(3) = 0;
      if (std::isnan(static_cast<float>(ipeaks_vox.value())))
        return true;
    }

    bool no_peaks = true;
    for (decltype(item.data)::Index i = 0; i < item.data.size(); i++) {
      if (std::isnan(item.data[i]))
        return true;
      if (no_peaks)
        if (i > 0 && item.data[i] != 0.0)
          no_peaks = false;
    }

    return no_peaks;
  }
};

void run() {
  auto SH_data = Image<value_type>::open(argument[0]).with_direct_io(3);
  Math::SH::check(SH_data);

  auto opt = get_options("mask");
  Image<bool> mask_data;
  if (!opt.empty())
    mask_data = Image<bool>::open(opt[0][0]);

  opt = get_options("seeds");
  Eigen::Matrix<value_type, Eigen::Dynamic, 2> dirs;
  if (opt.empty())
    dirs = DWI::Directions::electrostatic_repulsion_60().cast<value_type>();
  else
    dirs = File::Matrix::load_matrix<value_type>(opt[0][0]);
  if (dirs.cols() != 2)
    throw Exception("expecting 2 columns for search directions matrix");

  int npeaks = get_option_value("num", default_npeaks);

  opt = get_options("direction");
  std::vector<Direction> true_peaks;
  for (size_t n = 0; n < opt.size(); ++n) {
    Direction p(Math::pi * static_cast<default_type>(opt[n][0]) / 180.0,
                Math::pi * static_cast<default_type>(opt[n][1]) / 180.0);
    true_peaks.push_back(p);
  }
  if (!true_peaks.empty())
    npeaks = true_peaks.size();

  value_type threshold = get_option_value("threshold", -InfF);

  auto header = Header(SH_data);
  header.datatype() = DataType::Float32;

  opt = get_options("peaks");
  Image<value_type> ipeaks_data;
  if (!opt.empty()) {
    if (!true_peaks.empty())
      throw Exception("you can't specify both a peaks file and orientations to be estimated at the same time");
    if (!opt.empty())
      ipeaks_data = Image<value_type>::open(opt[0][0]);

    check_dimensions(SH_data, ipeaks_data, 0, 3);
    npeaks = ipeaks_data.size(3) / 3;
  }
  header.size(3) = 3 * npeaks;
  auto peaks = Image<value_type>::create(argument[1], header);

  DataLoader loader(SH_data, mask_data);
  Processor processor(peaks,
                      dirs,
                      Math::SH::LforN(SH_data.size(3)),
                      npeaks,
                      true_peaks,
                      threshold,
                      ipeaks_data,
                      !get_options("fast").empty());

  Thread::run_queue(loader, Thread::batch(Item()), Thread::multi(processor));
}
