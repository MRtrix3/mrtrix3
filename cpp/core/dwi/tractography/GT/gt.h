/* Copyright (c) 2008-2025 the MRtrix3 contributors.
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

#pragma once

#include <fstream>
#include <iostream>
#include <mutex>

#include <Eigen/Dense>

#include "math/math.h"
#include "progressbar.h"
#include "types.h"

namespace MR::DWI::Tractography::GT {

constexpr double M_4PI = 4.0 * Math::pi;
constexpr double M_sqrt4PI = 3.5449077018110320546;

constexpr ssize_t iter_bigstep = 10000;
constexpr default_type fraction_burnin = 0.1;
constexpr default_type fraction_phaseout = 0.1;

struct Properties {
  float p_birth;
  float p_death;
  float p_shift;
  float p_optshift;
  float p_connect;

  double density;
  double weight;
  int Lmax;

  double lam_ext;
  double lam_int;

  double beta;
  double ppot;

  Eigen::MatrixXf resp_WM;
  std::vector<Eigen::VectorXf> resp_ISO;
};

class Stats {
public:
  Stats(const double T0, const double T1, const uint64_t maxiter)
      : Text(T1),
        Tint(T0),
        EextTot(0.0),
        EintTot(0.0),
        n_iter(0),
        n_max(maxiter),
        progress("running MH sampler", n_max / iter_bigstep) {
    for (int k = 0; k != 5; k++)
      n_gen[k] = n_acc[k] = 0;
    alpha = std::pow(T1 / T0, //
                     static_cast<double>(iter_bigstep) /
                         (fraction_burnin * (n_max - n_max) - fraction_phaseout * n_max)); //
  }

  ~Stats() { out.close(); }

  void open_stream(std::string_view file) {
    out.close();
    out.open(std::string(file).c_str(), std::ofstream::out);
  }

  bool next() {
    std::lock_guard<std::mutex> lock(mutex);
    ++n_iter;
    if (n_iter % iter_bigstep == 0) {
      if ((n_iter >= fraction_burnin * n_max) && (n_iter < n_max - fraction_phaseout * n_max))
        Tint *= alpha;
      progress++;
      out << *this << std::endl;
    }
    return (n_iter < n_max);
  }

  // getters and setters ----------------------------------------------

  double getText() const { return Text; }

  double getTint() const { return Tint; }

  void setTint(double temp) {
    std::lock_guard<std::mutex> lock(mutex);
    Tint = temp;
  }

  double getEextTotal() const { return EextTot; }

  double getEintTotal() const { return EintTot; }

  void incEextTotal(double d) {
    std::lock_guard<std::mutex> lock(mutex);
    EextTot += d;
  }

  void incEintTotal(double d) {
    std::lock_guard<std::mutex> lock(mutex);
    EintTot += d;
  }

  unsigned int getN(const char p) const {
    switch (p) {
    case 'b':
      return n_gen[0];
    case 'd':
      return n_gen[1];
    case 'r':
      return n_gen[2];
    case 'o':
      return n_gen[3];
    case 'c':
      return n_gen[4];
    default:
      return 0;
    }
  }

  unsigned int getNa(const char p) const {
    switch (p) {
    case 'b':
      return n_acc[0];
    case 'd':
      return n_acc[1];
    case 'r':
      return n_acc[2];
    case 'o':
      return n_acc[3];
    case 'c':
      return n_acc[4];
    default:
      return 0;
    }
  }

  void incN(const char p, unsigned int i = 1) {
    std::lock_guard<std::mutex> lock(mutex);
    switch (p) {
    case 'b':
      n_gen[0] += i;
      break;
    case 'd':
      n_gen[1] += i;
      break;
    case 'r':
      n_gen[2] += i;
      break;
    case 'o':
      n_gen[3] += i;
      break;
    case 'c':
      n_gen[4] += i;
      break;
    default:
      return;
    }
  }

  void incNa(const char p, unsigned int i = 1) {
    std::lock_guard<std::mutex> lock(mutex);
    switch (p) {
    case 'b':
      n_acc[0] += i;
      break;
    case 'd':
      n_acc[1] += i;
      break;
    case 'r':
      n_acc[2] += i;
      break;
    case 'o':
      n_acc[3] += i;
      break;
    case 'c':
      n_acc[4] += i;
      break;
    }
  }

  double getAcceptanceRate(const char p) const {
    switch (p) {
    case 'b':
      return static_cast<double>(n_acc[0]) / static_cast<double>(n_gen[0]);
    case 'd':
      return static_cast<double>(n_acc[1]) / static_cast<double>(n_gen[1]);
    case 'r':
      return static_cast<double>(n_acc[2]) / static_cast<double>(n_gen[2]);
    case 'o':
      return static_cast<double>(n_acc[3]) / static_cast<double>(n_gen[3]);
    case 'c':
      return static_cast<double>(n_acc[4]) / static_cast<double>(n_gen[4]);
    default:
      return 0.0;
    }
  }

  friend std::ostream &operator<<(std::ostream &o, Stats const &stats);

protected:
  std::mutex mutex;
  double Text, Tint;
  double EextTot, EintTot;
  double alpha;

  unsigned long n_gen[5];
  unsigned long n_acc[5];
  unsigned long n_iter;
  const uint64_t n_max;

  ProgressBar progress;
  std::ofstream out;
};

} // namespace MR::DWI::Tractography::GT
