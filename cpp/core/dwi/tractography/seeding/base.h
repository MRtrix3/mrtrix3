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

#include <map>

#include "algo/threaded_loop.h"
#include "file/path.h"
#include "image.h"

namespace MR::DWI::Tractography::Seeding {

template <class ImageType> uint32_t get_count(ImageType &data) {
  std::atomic<uint32_t> count(0);
  ThreadedLoop(data).run(
      [&](ImageType &v) {
        if (v.value())
          count.fetch_add(1, std::memory_order_relaxed);
      },
      data);
  return count;
}

template <class ImageType> float get_volume(ImageType &data) {
  std::atomic<default_type> volume(0.0);
  ThreadedLoop(data).run(
      [&](decltype(data) &v) {
        const typename ImageType::value_type value = v.value();
        if (value) {
          default_type current = volume.load(std::memory_order_relaxed);
          default_type target;
          do {
            target = current + value;
          } while (!volume.compare_exchange_weak(current, target, std::memory_order_relaxed));
        }
      },
      data);
  return volume;
}

// Common interface for providing streamline seeds
class Base {

public:
  Base(const std::string &in, const std::string &desc, const size_t attempts)
      : volume(0.0), count(0), type(desc), name(Path::exists(in) ? Path::basename(in) : in), max_attempts(attempts) {}

  virtual ~Base() {}

  default_type vol() const { return volume; }
  uint32_t num() const { return count; }
  bool is_finite() const { return count; }
  const std::string &get_type() const { return type; }
  const std::string &get_name() const { return name; }
  size_t get_max_attempts() const { return max_attempts; }

  virtual bool get_seed(Eigen::Vector3f &) const = 0;
  virtual bool get_seed(Eigen::Vector3f &p, Eigen::Vector3f &) { return get_seed(p); }

  friend inline std::ostream &operator<<(std::ostream &stream, const Base &B) {
    stream << B.name;
    return (stream);
  }

protected:
  // Finite seeds are defined by the number of seeds; non-limited are defined by volume
  float volume;
  uint32_t count;
  mutable std::mutex mutex;
  const std::string type; // Text describing the type of seed this is

private:
  const std::string name;    // Could be an image path, or spherical coordinates
  const size_t max_attempts; // Maximum number of times the tracking algorithm should attempt to start from each
                             // provided seed point
};

} // namespace MR::DWI::Tractography::Seeding
