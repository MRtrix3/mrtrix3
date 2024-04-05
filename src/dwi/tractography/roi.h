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

#pragma once

#include "app.h"
#include "image.h"
#include "interp/linear.h"
#include "math/rng.h"
#include "misc/bitset.h"

namespace MR::DWI::Tractography {
class Properties;

extern const App::OptionGroup ROIOption;
void load_rois(Properties &properties);

class Mask : public Image<bool> {
public:
  using transform_type = Eigen::Transform<float, 3, Eigen::AffineCompact>;
  Mask(const Mask &) = default;
  Mask(const std::string &name)
      : Image<bool>(__get_mask(name)),
        scanner2voxel(new transform_type(Transform(*this).scanner2voxel.cast<float>())),
        voxel2scanner(new transform_type(Transform(*this).voxel2scanner.cast<float>())) {}

  std::shared_ptr<transform_type> scanner2voxel, voxel2scanner; // Ptr to prevent unnecessary copy-construction

private:
  static Image<bool> __get_mask(const std::string &name);
};

class ROI {
public:
  ROI(const Eigen::Vector3f &sphere_pos, float sphere_radius)
      : pos(sphere_pos), radius(sphere_radius), radius2(Math::pow2(radius)) {}

  ROI(const std::string &spec) : radius(NaN), radius2(NaN) {
    try {
      auto F = parse_floats(spec);
      if (F.size() != 4)
        throw 1;
      pos[0] = F[0];
      pos[1] = F[1];
      pos[2] = F[2];
      radius = F[3];
      radius2 = Math::pow2(radius);
    } catch (Exception &e_assphere) {
      try {
        mask.reset(new Mask(spec));
      } catch (Exception &e_asimage) {
        Exception e("Unable to parse text \"" + spec + "\" as a ROI");
        e.push_back("If interpreted as sphere:");
        for (size_t i = 0; i != e_assphere.num(); ++i)
          e.push_back("  " + e_assphere[i]);
        e.push_back("If interpreted as image:");
        for (size_t i = 0; i != e_asimage.num(); ++i)
          e.push_back("  " + e_asimage[i]);
        throw e;
      }
    }
  }

  std::string shape() const { return (mask ? "image" : "sphere"); }

  std::string parameters() const {
    return mask ? mask->name() : str(pos[0]) + "," + str(pos[1]) + "," + str(pos[2]) + "," + str(radius);
  }

  float min_featurelength() const {
    return mask ? std::min({mask->spacing(0), mask->spacing(1), mask->spacing(2)}) : radius;
  }

  bool contains(const Eigen::Vector3f &p) const {
    if (mask) {
      Eigen::Vector3f v = *(mask->scanner2voxel) * p;
      Mask temp(*mask); // Required for thread-safety
      temp.index(0) = std::round(v[0]);
      temp.index(1) = std::round(v[1]);
      temp.index(2) = std::round(v[2]);
      if (is_out_of_bounds(temp))
        return false;
      return temp.value();
    }
    return (pos - p).squaredNorm() <= radius2;
  }

  friend inline std::ostream &operator<<(std::ostream &stream, const ROI &roi) {
    stream << roi.shape() << " (" << roi.parameters() << ")";
    return stream;
  }

private:
  Eigen::Vector3f pos;
  float radius, radius2;
  std::shared_ptr<Mask> mask;
};

class ROISetBase {
public:
  ROISetBase() {}

  void clear() { R.clear(); }
  size_t size() const { return (R.size()); }
  const ROI &operator[](size_t i) const { return (R[i]); }
  void add(const ROI &roi) { R.push_back(roi); }

  friend inline std::ostream &operator<<(std::ostream &stream, const ROISetBase &R) {
    if (R.R.empty())
      return (stream);
    std::vector<ROI>::const_iterator i = R.R.begin();
    stream << *i;
    ++i;
    for (; i != R.R.end(); ++i)
      stream << ", " << *i;
    return stream;
  }

protected:
  std::vector<ROI> R;
};

class ROIUnorderedSet : public ROISetBase {
public:
  ROIUnorderedSet() {}
  bool contains(const Eigen::Vector3f &p) const {
    for (size_t n = 0; n < R.size(); ++n)
      if (R[n].contains(p))
        return (true);
    return false;
  }
  void contains(const Eigen::Vector3f &p, BitSet &retval) const {
    for (size_t n = 0; n < R.size(); ++n)
      if (R[n].contains(p))
        retval[n] = true;
  }
};

class ROIOrderedSet : public ROISetBase {
public:
  class LoopState {
  public:
    LoopState(const ROIOrderedSet &master) : size(master.size()), valid(true), next_index(0) {}
    LoopState(const size_t num_rois) : size(num_rois), valid(true), next_index(0) {}

    void reset() {
      valid = true;
      next_index = 0;
    }
    operator bool() const { return valid; }

    void operator()(const size_t roi_index) {
      assert(roi_index < size);
      if (roi_index == next_index)
        ++next_index;
      else if (roi_index != next_index - 1)
        valid = false;
    }

    bool all_entered() const { return (valid && (next_index == size)); }

  private:
    const size_t size;
    bool valid; // true if the order at which ROIs have been entered thus far is legal
    size_t next_index;
  };

  ROIOrderedSet() {}

  void contains(const Eigen::Vector3f &p, LoopState &loop_state) const {
    // do nothing if the series of coordinates have already performed something illegal
    if (!loop_state)
      return;
    for (size_t n = 0; n < R.size(); ++n) {
      if (R[n].contains(p)) {
        loop_state(n);
        break;
      }
    }
  }
};

class IncludeROIVisitation {
public:
  IncludeROIVisitation(const ROIUnorderedSet &unordered, const ROIOrderedSet &ordered)
      : unordered(unordered), ordered(ordered), visited(unordered.size()), state(ordered.size()) {}

  IncludeROIVisitation(const IncludeROIVisitation &) = default;
  IncludeROIVisitation &operator=(const IncludeROIVisitation &) = delete;

  void reset() {
    visited.clear();
    state.reset();
  }
  size_t size() const { return unordered.size() + ordered.size(); }

  void operator()(const Eigen::Vector3f &p) {
    unordered.contains(p, visited);
    ordered.contains(p, state);
  }

  operator bool() const { return (visited.full() && state.all_entered()); }
  bool operator!() const { return (!visited.full() || !state.all_entered()); }

protected:
  const ROIUnorderedSet &unordered;
  const ROIOrderedSet &ordered;
  BitSet visited;
  ROIOrderedSet::LoopState state;
};

} // namespace MR::DWI::Tractography
