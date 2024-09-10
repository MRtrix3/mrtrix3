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

#include <memory>

#include "command.h"
#include "dwi/directions/file.h"
#include "file/utils.h"
#include "math/rng.h"
#include "mutexprotected.h"
#include "progressbar.h"
#include "thread.h"

constexpr size_t default_number = 1e8;

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
    " by requesting only a single rotation.";

  ARGUMENTS
    + Argument ("in", "the input direction file").type_file_in()
    + Argument ("out", "the output direction file").type_file_out();


  OPTIONS
    + Option ("number", "number of rotations to try"
                        " (default: " + str(default_number) + ")")
    +   Argument ("num").type_integer(1)

    + Option ("cartesian", "Output the directions in Cartesian coordinates [x y z]"
                           " instead of [az el].");
}
// clang-format on

using axis_type = Eigen::Matrix<default_type, 3, 1>;
using cartesian_matrix_type = Eigen::Matrix<default_type, Eigen::Dynamic, 3>;
using rotation_type = Eigen::AngleAxis<default_type>;
using rotation_transform_type = Eigen::Transform<default_type, 3, Eigen::Isometry>;

class Shared {
public:
  Shared(const std::shared_ptr<const cartesian_matrix_type> directions, size_t total_num_rotations)
      : directions(directions),                                                         //
        protected_content(total_num_rotations, directions->array().abs().maxCoeff()) {} //

  bool operator()(default_type peak, const rotation_type &rotation) {
    auto guard = protected_content.lock();
    return (*guard)(peak, rotation);
  }

  default_type peak(const rotation_type &rotation) const {
    return (rotation_transform_type(rotation).linear() * directions->transpose()).array().abs().maxCoeff();
  }

  rotation_type get_best_rotation() {
    auto guard = protected_content.lock();
    return guard->best();
  }

private:
  const std::shared_ptr<const cartesian_matrix_type> directions;

  class ProtectedContent {
  public:
    ProtectedContent(const size_t total_num_rotations, const default_type original_peak)
        : total_num_rotations(total_num_rotations),
          original_peak(original_peak),
          progress(total_num_rotations == 1 ? "randomising direction set orientation"
                                            : "optimising directions for peak gradient load",
                   total_num_rotations),
          count(0),
          best_rotation(0.0, axis_type{0.0, 0.0, 0.0}),
          min_peak(1.0) {}
    bool operator()(default_type peak, const rotation_type &rotation) {
      if (peak < min_peak) {
        min_peak = peak;
        best_rotation = rotation;
        progress.set_text(std::string(total_num_rotations == 1 ? "randomising direction set orientation"
                                                               : "optimising directions for peak gradient load") +
                          " (original = " + str(original_peak, 6) + "; " +
                          (total_num_rotations == 1 ? "rotated" : "best") + " = " + str(min_peak, 6) + ")");
      }
      ++count;
      ++progress;
      return count < total_num_rotations;
    }
    rotation_type best() const { return best_rotation; }

  private:
    const size_t total_num_rotations;
    const default_type original_peak;
    ProgressBar progress;
    size_t count;
    rotation_type best_rotation;
    default_type min_peak;
  };
  MutexProtected<ProtectedContent> protected_content;
};

class Processor {
public:
  Processor(const std::shared_ptr<Shared> shared)
      : shared(shared),
        rotation(0.0, axis_type{0.0, 0.0, 0.0}),
        angle_distribution(-Math::pi, Math::pi),
        axes_distribution(-1.0, 1.0) {}

  void execute() {
    while (eval())
      ;
  }

  bool eval() {
    rotation.angle() = angle_distribution(rng);
    do {
      axis[0] = axes_distribution(rng);
      axis[1] = axes_distribution(rng);
      axis[2] = axes_distribution(rng);
    } while (axis.squaredNorm() > 1.0);
    rotation.axis() = axis.normalized();

    return (*shared)(shared->peak(rotation), rotation);
  }

protected:
  std::shared_ptr<Shared> shared;
  rotation_type rotation;
  axis_type axis;
  Math::RNG rng;
  std::uniform_real_distribution<default_type> angle_distribution;
  std::uniform_real_distribution<default_type> axes_distribution;
};

void run() {
  const auto directions = std::make_shared<const cartesian_matrix_type>(DWI::Directions::load_cartesian(argument[0]));

  const size_t total_num_rotations = get_option_value<size_t>("number", default_number);

  rotation_type rotation;
  {
    const auto shared = std::make_shared<Shared>(directions, total_num_rotations);
    if (total_num_rotations == 1) {
      Processor processor(shared);
      processor.eval();
    } else {
      Thread::run(Thread::multi(Processor(shared)), "eval thread");
    }
    rotation = shared->get_best_rotation();
  }

  const cartesian_matrix_type result =
      (rotation_transform_type(rotation).linear() * directions->transpose()).transpose();

  DWI::Directions::save(result, argument[1], !get_options("cartesian").empty());
}
