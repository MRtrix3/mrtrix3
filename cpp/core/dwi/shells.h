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

#pragma once

#include <fstream>
#include <limits>

#include "app.h"
#include "dwi/gradient.h"
#include "file/config.h"
#include "types.h"

namespace MR::App {
class OptionGroup;
}

namespace MR::DWI {

constexpr default_type default_shellclustering_epsilon = 80.0;
constexpr ssize_t default_shellclustering_mindirections = 6;
constexpr ssize_t default_shellclustering_minlinkage = 3;

extern const App::OptionGroup ShellsOption;

class Shell {

public:
  Shell() : mean(0.0), stdev(0.0), min(0.0), max(0.0) {}
  Shell(const Eigen::MatrixXd &grad, const std::vector<size_t> &indices);

  const std::vector<size_t> &get_volumes() const { return volumes; }
  size_t count() const { return volumes.size(); }
  size_t size() const { return volumes.size(); }

  default_type get_mean() const { return mean; }
  default_type get_stdev() const { return stdev; }
  default_type get_min() const { return min; }
  default_type get_max() const { return max; }

  bool is_bzero() const { return (mean < MR::DWI::bzero_threshold()); }

  bool operator<(const Shell &rhs) const { return (mean < rhs.mean); }

  friend std::ostream &operator<<(std::ostream &stream, const Shell &S) {
    stream << "Shell: " << S.volumes.size() << " volumes, b-value " << S.mean << " +- " << S.stdev << " (range ["
           << S.min << " - " << S.max << "])";
    return stream;
  }

protected:
  std::vector<size_t> volumes;
  default_type mean, stdev, min, max;
};

class Shells {
public:
  Shells(const Eigen::MatrixXd &grad);

  const Shell &operator[](const size_t i) const { return shells[i]; }
  const Shell &smallest() const { return shells.front(); }
  const Shell &largest() const { return shells.back(); }
  size_t count() const { return shells.size(); }
  size_t size() const { return shells.size(); }
  size_t volumecount() const {
    size_t count = 0;
    for (const auto &it : shells)
      count += it.count();
    return count;
  }

  std::vector<size_t> get_counts() const {
    std::vector<size_t> c(count());
    for (size_t n = 0; n < count(); ++n)
      c[n] = shells[n].count();
    return c;
  }

  std::vector<size_t> get_bvalues() const {
    std::vector<size_t> b(count());
    for (size_t n = 0; n < count(); ++n)
      b[n] = shells[n].get_mean();
    return b;
  }

  Shells &select_shells(const bool force_singleshell, const bool force_with_bzero, const bool force_without_bzero);

  Shells &reject_small_shells(const size_t min_volumes = default_shellclustering_mindirections);

  bool is_single_shell() const {
    // only if exactly 1 non-bzero shell
    return ((count() == 1 && !has_bzero()) || (count() == 2 && has_bzero()));
  }

  bool has_bzero() const { return smallest().is_bzero(); }

  friend std::ostream &operator<<(std::ostream &stream, const Shells &S) {
    stream << "Total of " << S.count() << " DWI shells:" << std::endl;
    for (const auto &it : S.shells)
      stream << it << std::endl;
    return stream;
  }

protected:
  std::vector<Shell> shells;

private:
  using BValueList = decltype(std::declval<const Eigen::MatrixXd>().col(0));

  // Functions for current b-value clustering implementation
  size_t clusterBvalues(const BValueList &, std::vector<size_t> &) const;
  void regionQuery(const BValueList &, const default_type, std::vector<size_t> &) const;
};

} // namespace MR::DWI
