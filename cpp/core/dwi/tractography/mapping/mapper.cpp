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

#include "dwi/tractography/mapping/mapper.h"

#include "dwi/tractography/curvature.h"
#include "file/matrix.h"

namespace MR::DWI::Tractography::Mapping {

void TrackMapperBase::voxelise(const Streamline<> &tck, SetVoxel &voxels) const {
  Eigen::Vector3i vox;
  for (const auto &i : tck) {
    vox = round(scanner2voxel * i);
    if (check(vox, info))
      voxels.std::set<Voxel>::insert(vox);
  }
}

void TrackMapperTWI::set_factor(const Streamline<> &tck, SetVoxelExtras &out) const {

  size_t count = 0;

  switch (contrast) {

  case contrast_t::TDI:
    out.factor = 1.0;
    break;
  case contrast_t::LENGTH:
    out.factor = Tractography::length(tck, Tractography::LengthMethod::CHORD);
    break;
  case contrast_t::INVLENGTH:
    out.factor = 1.0 / Tractography::length(tck, Tractography::LengthMethod::CHORD);
    break;

  case contrast_t::SCALAR_MAP:
  case contrast_t::SCALAR_MAP_COUNT:
  case contrast_t::FOD_AMP:
  case contrast_t::CURVATURE:

    factors.clear();
    factors.reserve(tck.size());
    load_factors(tck);

    switch (track_statistic) {

    case tck_stat_t::SUM:
      out.factor = 0.0;
      for (const auto &i : factors)
        if (std::isfinite(i))
          out.factor += i;
      break;

    case tck_stat_t::MIN:
      out.factor = Inf;
      for (const auto &i : factors) {
        if (std::isfinite(i))
          out.factor = std::min(out.factor, i);
      }
      break;

    case tck_stat_t::MEAN:
      out.factor = 0.0;
      for (const auto &i : factors) {
        if (std::isfinite(i)) {
          out.factor += i;
          ++count;
        }
      }
      out.factor = (count ? (out.factor / static_cast<default_type>(count)) : 0.0);
      break;

    case tck_stat_t::MAX:
      out.factor = -Inf;
      for (const auto &i : factors) {
        if (std::isfinite(i))
          out.factor = std::max(out.factor, i);
      }
      break;

    case tck_stat_t::MEDIAN:
      if (factors.empty()) {
        out.factor = 0.0;
      } else {
        nth_element(factors.begin(), factors.begin() + (factors.size() / 2), factors.end());
        out.factor = *(factors.begin() + (factors.size() / 2));
      }
      break;

    case tck_stat_t::MEAN_NONZERO:
      out.factor = 0.0;
      for (const auto &i : factors) {
        if (std::isfinite(i) && i) {
          out.factor += i;
          ++count;
        }
      }
      out.factor = (count ? (out.factor / static_cast<default_type>(count)) : 0.0);
      break;

    case tck_stat_t::GAUSSIAN:
      throw Exception("Gaussian track-wise statistic should not be used in TrackMapperTWI class; use "
                      "Mapping::Gaussian::TrackMapper instead");

    case tck_stat_t::ENDS_MIN:
      assert(factors.size() == 2);
      out.factor = (std::fabs(factors[0]) < std::fabs(factors[1])) ? factors[0] : factors[1];
      break;

    case tck_stat_t::ENDS_MEAN:
      assert(factors.size() == 2);
      out.factor = 0.5 * (factors[0] + factors[1]);
      break;

    case tck_stat_t::ENDS_MAX:
      assert(factors.size() == 2);
      out.factor = (std::fabs(factors[0]) > std::fabs(factors[1])) ? factors[0] : factors[1];
      break;

    case tck_stat_t::ENDS_PROD:
      assert(factors.size() == 2);
      if ((factors[0] < 0.0 && factors[1] < 0.0) || (factors[0] > 0.0 && factors[1] > 0.0))
        out.factor = factors[0] * factors[1];
      else
        out.factor = 0.0;
      break;

    case tck_stat_t::ENDS_CORR:
      assert(factors.size() == 1);
      out.factor = factors.front();
      break;

    default:
      throw Exception("FIXME: Undefined / unsupported track statistic in TrackMapperTWI::get_factor()");
    }
    break;

  case contrast_t::VECTOR_FILE:
    assert(vector_data);
    assert(tck.get_index() < static_cast<size_t>(vector_data->size()));
    out.factor = (*vector_data)[tck.get_index()];
    break;

  default:
    throw Exception("FIXME: Undefined / unsupported contrast mechanism in TrackMapperTWI::get_factor()");
  }

  if (contrast == contrast_t::SCALAR_MAP_COUNT)
    out.factor = (out.factor != 0.0 ? 1.0 : 0.0);

  if (!std::isfinite(out.factor))
    out.factor = 0.0;
}

void TrackMapperTWI::add_scalar_image(const std::filesystem::path &path) {
  if (image_plugin)
    throw Exception("Cannot add more than one associated image to TWI");
  if (contrast != contrast_t::SCALAR_MAP && contrast != contrast_t::SCALAR_MAP_COUNT)
    throw Exception("Cannot add a scalar image to TWI unless the contrast depends on it");
  image_plugin.reset(new TWIScalarImagePlugin(path, track_statistic));
}

void TrackMapperTWI::set_backtrack() {
  if (!image_plugin)
    throw Exception("Cannot backtrack if no TWI associated image provided");
  const TWIImagePluginBase *const base = image_plugin.get();
  if (typeid(*base) != typeid(TWIScalarImagePlugin))
    throw Exception("Backtracking is only applicable to scalar image TWI plugins");
  TWIScalarImagePlugin *const ptr = dynamic_cast<TWIScalarImagePlugin *>(image_plugin.get());
  ptr->set_backtrack();
}

void TrackMapperTWI::add_fod_image(const std::filesystem::path &path) {
  if (image_plugin)
    throw Exception("Cannot add more than one associated image to TWI");
  if (contrast != contrast_t::FOD_AMP)
    throw Exception("Cannot add an FOD image to TWI unless the FOD_AMP contrast is used");
  image_plugin.reset(new TWIFODImagePlugin(path, track_statistic));
}

void TrackMapperTWI::add_twdfc_static_image(Image<float> &image) {
  if (image_plugin)
    throw Exception("Cannot add more than one associated image to TWI");
  if (contrast != contrast_t::SCALAR_MAP)
    throw Exception("For fMRI correlation mapping, mapper must be set to SCALAR_MAP contrast");
  if (track_statistic != tck_stat_t::ENDS_CORR)
    throw Exception("For fMRI correlation mapping, only the endpoint correlation track-wise statistic is valid");
  image_plugin.reset(new TWDFCStaticImagePlugin(image));
}

void TrackMapperTWI::add_twdfc_dynamic_image(Image<float> &image,
                                             const std::vector<float> &kernel,
                                             const ssize_t timepoint) {
  if (image_plugin)
    throw Exception("Cannot add more than one associated image to TWI");
  if (contrast != contrast_t::SCALAR_MAP)
    throw Exception("For sliding time-window fMRI mapping, mapper must be set to SCALAR_MAP contrast");
  if (track_statistic != tck_stat_t::ENDS_CORR)
    throw Exception(
        "For sliding time-window fMRI mapping, only the endpoint correlation track-wise statistic is valid");
  image_plugin.reset(new TWDFCDynamicImagePlugin(image, kernel, timepoint));
}

void TrackMapperTWI::add_vector_data(const std::filesystem::path &path) {
  if (image_plugin)
    throw Exception("Cannot add both an associated image and a vector data file to TWI");
  if (contrast != contrast_t::VECTOR_FILE)
    throw Exception("Cannot add a vector data file to TWI unless the VECTOR_FILE contrast is used");
  vector_data.reset(new Eigen::VectorXf(File::Matrix::load_vector<float>(path)));
}

void TrackMapperTWI::load_factors(const Streamline<> &tck) const {

  if (contrast == contrast_t::SCALAR_MAP || contrast == contrast_t::SCALAR_MAP_COUNT) {
    assert(image_plugin);
    image_plugin->load_factors(tck, factors);
    return;
  }
  if (contrast == contrast_t::FOD_AMP) {
    assert(image_plugin);
    image_plugin->load_factors(tck, factors);
    return;
  }
  if (contrast != contrast_t::CURVATURE)
    throw Exception("Unsupported contrast in function TrackMapperTWI::load_factors()");

  // Per-vertex curvature (1/mm) is produced by a dedicated, robust, arc-length-smoothed estimator.
  // The per-vertex factor remains "curvature at this vertex in 1/mm"; only its quality changes.
  const std::vector<default_type> kappa = Tractography::curvature(tck);
  factors.assign(kappa.begin(), kappa.end());
}

} // namespace MR::DWI::Tractography::Mapping
