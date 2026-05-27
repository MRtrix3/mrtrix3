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

#include <string>
#include <vector>

#include "algo/threaded_loop.h"
#include "command.h"
#include "dwi/directions/set.h"
#include "enum.h"
#include "image.h"
#include "image_helpers.h"
#include "math/SH.h"
#include "math/entropy.h"
#include "mrtrix.h"

using namespace MR;
using namespace App;

constexpr size_t default_direction_set = 1281;

enum class metrics { ENTROPY, POWER };

enum class entropy_normalisation { NONE, NORM, INVNORM };

// clang-format off
void usage() {

  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com) and Robert E. Smith <robert.smith@florey.edu.au>";

  SYNOPSIS = "Compute voxel-wise metrics from one or more spherical harmonics images";

  DESCRIPTION
    + "Depending on the particular metric being computed,"
      " the command may only accept a single input SH image;"
      " wheras other metrics may accept multiple SH images as input (eg. ODFs)"
      " and compute a single scalar output image."

    + "The various metrics available are detailed individually below."

    + "\"entropy\":"
      " this metric computes the entropy (in nats, ie. logarithm base e)"
      " of one or more spherical harmonics functions."
      " This can be thought of as being inversely proportional to the overall \"complexity\""
      " of the (set of) spherical harmonics function(s)."
      " Used in conjunction with the -invnorm option,"
      " the result behaves somewhat akin to an anisotropy measure."

    + "\"power\":"
      " this metric computes the sum of squared SH coefficients,"
      " which equals the mean-squared amplitude of the spherical function it represents."

    + Math::SH::encoding_description;

  ARGUMENTS
    + Argument ("SH", "the input spherical harmonics coefficients image(s)").type_image_in().allow_multiple()
    + Argument ("metric", "the metrc to compute; one of: " + Enum::join<metrics>()).type_choice<metrics>()
    + Argument ("output", "the output metric image").type_image_out();

  OPTIONS
    + OptionGroup ("Options specific to the \"entropy\" metric")
    + Option ("normalised", "normalise the voxel-wise entropy measure to the range [0.0, 1.0]")
    + Option ("invnorm", "compute the complement of the normalised voxel-wise entropy measure"
                         " (ie. 1.0 - normalised),"
                         " such that values closer to 1.0 reflect greater concentration of the function")
    + Option ("directions", "specify the direction set to be used for SH amplitude sampling;"
                            " either an input file containing a set of directions,"
                            " or an integer corresponding to a built-in direction set")
      + Argument ("spec").type_file_in().type_integer(1)

    + OptionGroup ("Options specific to the \"power\" metric")
    + Option ("spectrum", "output the power spectrum,"
                          " i.e., the power contained within each harmonic degree (l=0, 2, 4, ...)"
                          " as a 4D image.");

}
// clang-format on

const DWI::Directions::Set get_directions() {
  auto opt = get_options("directions");
  if (!opt.empty()) {
    try {
      return DWI::Directions::Set(static_cast<std::string>(opt[0][0]));
    } catch (Exception &) {
      DEBUG("Unable to import direction set interpreting \"" + opt[0][0] + "\" as filesystem path");
    }
    try {
      return DWI::Directions::Set(static_cast<size_t>(opt[0][0]));
    } catch (Exception &) {
      throw Exception("Unable to interpret user input \"" + opt[0][0] + "\" as a direction set");
    }
  }
  return DWI::Directions::Set(default_direction_set);
}

void run_entropy() {
  std::vector<Image<float>> SH_images;
  size_t max_lmax = 0;
  Header H_out;
  for (size_t i = 0; i != argument.size() - 2; ++i) {
    Header header(Header::open(argument[i]));
    Math::SH::check(header);
    max_lmax = std::max(max_lmax, Math::SH::LforN(static_cast<int>(header.size(3))));
    if (i == 0) {
      H_out = header;
    } else {
      if (!voxel_grids_match_in_scanner_space(header, H_out))
        throw Exception("All input SH images must have matching voxel grids");
      H_out.merge_keyval(header.keyval());
    }
    SH_images.emplace_back(header.get_image<float>());
  }
  H_out.ndim() = 3;

  const DWI::Directions::Set dirs(get_directions());
  const bool opt_normalised = !get_options("normalised").empty();
  const bool opt_invnorm = !get_options("invnorm").empty();
  if (opt_normalised && opt_invnorm)
    throw Exception("Options \"-normalised\" and \"-invnorm\" are mutually exclusive");
  const entropy_normalisation norm_mode =
      opt_normalised ? entropy_normalisation::NORM
                     : (opt_invnorm ? entropy_normalisation::INVNORM : entropy_normalisation::NONE);
  const Image<float> image_out(Image<float>::create(argument[argument.size() - 1], H_out));

  class Processor {
  public:
    using vector_type = Eigen::Matrix<default_type, Eigen::Dynamic, 1>;
    Processor(const std::vector<Image<float>> &SH_images,
              const DWI::Directions::Set &dirs,
              const Image<float> &output_image,
              const entropy_normalisation norm_mode)
        : out(output_image),
          concat_amps(SH_images.size() * dirs.size()),
          shared(new Shared(SH_images, dirs, norm_mode)) {
      for (const auto &i : SH_images)
        images.emplace_back(i);
    }

    Processor(const Processor &that)
        : out(that.out), SH_coefs(that.SH_coefs.size()), concat_amps(that.concat_amps.size()), shared(that.shared) {
      for (const auto &i : that.images)
        images.emplace_back(i);
    }

    bool operator()(Iterator &pos) {
      ssize_t offset = 0;
      for (auto &image : images) {
        assign_pos_of(pos).to(image);
        image.index(3) = 0;
        SH_coefs.resize(image.size(3));
        for (auto l = Loop(3)(image); l; ++l)
          SH_coefs[image.index(3)] = image.value();
        concat_amps.segment(offset, shared->get_num_dirs()) = (*shared)(SH_coefs);
        offset += shared->get_num_dirs();
      }
      assign_pos_of(pos).to(out);
      try {
        out.value() = static_cast<float>(shared->normalise(Math::Entropy::nats(concat_amps)));
      } catch (Exception &) { // NOLINT(bugprone-empty-catch)
        out.value() = std::numeric_limits<float>::quiet_NaN();
      }
      return true;
    }

  private:
    std::vector<Image<float>> images;
    Image<float> out;
    vector_type SH_coefs;
    vector_type concat_amps;

    // Construct the requisite SH2A transforms given the set of input images
    // Note that multiple ODFs may have the same lmax,
    //   and can therefore make use of the same transform
    class Shared {
    public:
      using transform_type = Eigen::Matrix<default_type, Eigen::Dynamic, Eigen::Dynamic>;
      Shared(const std::vector<Image<float>> &SH_images,
             const DWI::Directions::Set &dirs,
             const entropy_normalisation norm_mode)
          : num_dirs(dirs.size()) {
        Eigen::Matrix<default_type, Eigen::Dynamic, 3> dirs_as_matrix(dirs.size(), 3);
        for (ssize_t row = 0; row != dirs.size(); ++row)
          dirs_as_matrix.row(row) = dirs[row];
        size_t max_lmax = 0;
        for (const auto &i : SH_images) {
          const size_t lmax = Math::SH::LforN(static_cast<int>(i.size(3)));
          if (transforms.find(lmax) == transforms.end())
            transforms.insert(
                std::make_pair(lmax, Math::SH::init_transform_cart(dirs_as_matrix, static_cast<int>(lmax))));
          max_lmax = std::max(max_lmax, lmax);
        }
        if (norm_mode != entropy_normalisation::NONE)
          normalisation.initialise(norm_mode, SH_images.size(), transforms[max_lmax]);
      }
      vector_type operator()(vector_type &SH_coefs) const {
        const size_t lmax = Math::SH::LforN(static_cast<int>(SH_coefs.size()));
        auto transform_it = transforms.find(lmax);
        assert(transform_it != transforms.end());
        return (transform_it->second * SH_coefs).eval();
      }
      [[nodiscard]] size_t get_num_dirs() const { return num_dirs; }
      [[nodiscard]] default_type normalise(const default_type in) const { return normalisation(in); }

    private:
      const size_t num_dirs;
      std::map<size_t, transform_type> transforms;

      class Normalisation {
      public:
        Normalisation()
            : lower(std::numeric_limits<default_type>::quiet_NaN()),
              upper(std::numeric_limits<default_type>::quiet_NaN()) {}
        void initialise(const entropy_normalisation normalise_mode,
                        const size_t num_images,
                        const transform_type &transform) {
          mode = normalise_mode;
          Eigen::Matrix<default_type, Eigen::Dynamic, 1> delta_coefs;
          Math::SH::delta(delta_coefs,
                          Eigen::Matrix<default_type, 3, 1>({0.0, 0.0, 1.0}),
                          static_cast<int>(Math::SH::LforN(static_cast<int>(transform.cols()))));
          // Get amplitude samples of this delta function,
          //   and pad out zeroes to simulate all other input SH functions being zero
          Eigen::Matrix<default_type, Eigen::Dynamic, 1> amps = transform * delta_coefs;
          amps.conservativeResizeLike(Eigen::Matrix<default_type, Eigen::Dynamic, 1>::Zero(
              static_cast<Eigen::Index>(num_images) * transform.rows()));
          lower = Math::Entropy::nats(amps);
          upper = Math::Entropy::nats(Eigen::Matrix<default_type, Eigen::Dynamic, 1>::Constant(
              static_cast<Eigen::Index>(num_images) * transform.rows(), 1.0));
        }
        default_type operator()(const default_type in) const {
          if (mode == entropy_normalisation::NONE)
            return in;
          const default_type scaled = std::max(0.0, std::min(1.0, (in - lower) / (upper - lower)));
          return (mode == entropy_normalisation::INVNORM) ? (1.0 - scaled) : scaled;
        }

      private:
        entropy_normalisation mode{entropy_normalisation::NONE};
        default_type lower;
        default_type upper;
      } normalisation;
    };
    std::shared_ptr<Shared> shared;

  } processor(SH_images, dirs, image_out, norm_mode);

  ThreadedLoop("computing entropy", H_out).run(processor);
}

void run_power() {
  auto SH_data = Image<float>::open(argument[0]);
  Math::SH::check(SH_data);

  Header power_header(SH_data);

  const bool spectrum = !get_options("spectrum").empty();

  const size_t lmax = Math::SH::LforN(static_cast<int>(SH_data.size(3)));
  INFO("calculating spherical harmonic power up to degree " + str(lmax));

  if (spectrum)
    power_header.size(3) = static_cast<ssize_t>(1 + lmax / 2);
  else
    power_header.ndim() = 3;
  power_header.datatype() = DataType::Float32;

  auto power_data = Image<float>::create(argument[2], power_header);

  auto f1 = [&](decltype(power_data) &P, decltype(SH_data) &SH) {
    P.index(3) = 0;
    // Use signed type to enable initialisation of m
    for (int l = 0; l <= static_cast<int>(lmax); l += 2) {
      default_type power = 0.0;
      for (int m = -l; m <= l; ++m) {
        SH.index(3) = static_cast<ssize_t>(Math::SH::index(l, m));
        const float val = SH.value();
        power += Math::pow2(static_cast<default_type>(val));
      }
      P.value() = static_cast<float>(power / (Math::pi * 4));
      ++P.index(3);
    }
  };

  auto f2 = [&](decltype(power_data) &P, decltype(SH_data) &SH) {
    float power = 0.0;
    for (int l = 0; l <= static_cast<int>(lmax); l += 2) {
      for (int m = -l; m <= l; ++m) {
        SH.index(3) = static_cast<ssize_t>(Math::SH::index(l, m));
        const float val = SH.value();
        power += Math::pow2(val);
      }
    }
    P.value() = static_cast<float>(power / (Math::pi * 4));
  };

  auto loop = ThreadedLoop("calculating SH power", SH_data, 0, 3);
  if (spectrum)
    loop.run(f1, power_data, SH_data);
  else
    loop.run(f2, power_data, SH_data);
}

void run() {
  switch (Enum::from_name<metrics>(argument[argument.size() - 2])) {
  case metrics::ENTROPY:
    run_entropy();
    return;
  case metrics::POWER:
    run_power();
    return;
  }
}
