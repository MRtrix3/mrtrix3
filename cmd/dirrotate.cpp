/* Copyright (c) 2008-2024 the MRtrix3 contributors.
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

#include "command.h"
#include "dwi/directions/file.h"
#include "file/utils.h"
#include "math/rng.h"
#include "progressbar.h"
#include "thread.h"

constexpr size_t default_permutations = 1e8;

using namespace MR;
using namespace App;

// clang-format off
void usage() {

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Apply a rotation to a direction set";

  DESCRIPTION
  + "The primary use case of this command is to find,"
    " for a given basis direction set,"
    " an appropriate rotation that preserves the homogeneity of coverage on the sphere"
    " but that minimises the maximal peak amplitude along the physical axes of the scanner,"
    " so as to minimise the peak gradient system demands."
    " It can alternatively be used to introduce a random rotation"
    " to hopefully prevent any collinearity between directions in different shells,"
    " by requesting only a single permutation.";

  ARGUMENTS
    + Argument ("in", "the input direction file").type_file_in()
    + Argument ("out", "the output direction file").type_file_out();


  OPTIONS
    + Option ("permutations", "number of permutations to try"
                              " (default: " + str(default_permutations) + ")")
    +   Argument ("num").type_integer(1)

    + Option ("cartesian", "Output the directions in Cartesian coordinates [x y z]"
                           " instead of [az el].");
}
// clang-format on

using axis_type = Eigen::Matrix<default_type, 3, 1>;
using rotation_type = Eigen::AngleAxis<default_type>;
using rotation_transform_type = Eigen::Transform<default_type, 3, Eigen::Isometry>;

class Shared {
public:
  Shared(const Eigen::MatrixXd &directions, size_t target_num_permutations)
      : directions(directions),
        target_num_permutations(target_num_permutations),
        num_permutations(0),
        progress(target_num_permutations == 1 ? "randomising direction set orientation" : "optimising directions for peak gradient load", target_num_permutations),
        best_rotation(0.0, axis_type{0.0, 0.0, 0.0}),
        min_peak(1.0),
        original_peak(directions.array().abs().maxCoeff()) {}

  bool update(default_type peak, const rotation_type &rotation) {
    std::lock_guard<std::mutex> lock(mutex);
    if (peak < min_peak) {
      min_peak = peak;
      best_rotation = rotation;
      progress.set_text(
          std::string(target_num_permutations == 1 ? "randomising direction set orientation" : "optimising directions for peak gradient load")
          + " (original = " + str(original_peak, 6) + "; "
          + (target_num_permutations == 1 ? "rotated" : "best")
          + " = " + str(min_peak, 6) + ")");
    }
    ++num_permutations;
    ++progress;
    return num_permutations < target_num_permutations;
  }

  default_type peak(const rotation_type &rotation) const {
    return (rotation_transform_type(rotation).linear() * directions.transpose()).array().abs().maxCoeff();
  }

  const rotation_type &get_best_rotation() const { return best_rotation; }

protected:
  const Eigen::MatrixXd &directions;
  const size_t target_num_permutations;
  size_t num_permutations;
  ProgressBar progress;
  rotation_type best_rotation;
  default_type min_peak;
  default_type original_peak;
  std::mutex mutex;
};

class Processor {
public:
  Processor(Shared &shared) :
      shared(shared),
      rotation(0.0, axis_type{0.0, 0.0, 0.0}),
      angle_distribution (-Math::pi, Math::pi),
      axes_distribution (-1.0, 1.0) {}

  void execute() {
    while (eval());
  }

  bool eval() {
    rotation.angle() = angle_distribution(rng);
    do {
      axis[0] = axes_distribution(rng);
      axis[1] = axes_distribution(rng);
      axis[2] = axes_distribution(rng);
    } while (axis.squaredNorm() > 1.0);
    rotation.axis() = axis.normalized();

    return shared.update(shared.peak(rotation), rotation);
  }

protected:
  Shared &shared;
  rotation_type rotation;
  axis_type axis;
  Math::RNG rng;
  std::uniform_real_distribution<default_type> angle_distribution;
  std::uniform_real_distribution<default_type> axes_distribution;
};

void run() {
  auto directions = DWI::Directions::load_cartesian(argument[0]);

  size_t num_permutations = get_option_value<size_t>("permutations", default_permutations);

  rotation_type rotation;
  {
    Shared shared(directions, num_permutations);
    if (num_permutations == 1) {
      Processor processor(shared);
      processor.eval();
    } else {
      Thread::run(Thread::multi(Processor(shared)), "eval thread");
    }
    rotation = shared.get_best_rotation();
  }

  directions = (rotation_transform_type(rotation).linear() * directions.transpose()).transpose();

  DWI::Directions::save(directions, argument[1], !get_options("cartesian").empty());
}
