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

#include <filesystem>
#include <optional>
#include <string>
#include <tcb/span.hpp>
#include <vector>

#include "command.h"
#include "dwi/tractography/field_registry.h"
#include "dwi/tractography/formats/base.h"
#include "dwi/tractography/formats/list.h"
#include "dwi/tractography/formats/tsf.h"
#include "dwi/tractography/mapping/mapper.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/sidecar.h"
#include "dwi/tractography/sidecar_embed.h"
#include "dwi/tractography/sidecar_value.h"
#include "dwi/tractography/tractogram.h"
#include "dwi/tractography/tractogram_item.h"
#include "enum.h"
#include "file/matrix.h"
#include "file/ofstream.h"
#include "file/path.h"
#include "image.h"
#include "image_helpers.h"
#include "interp/linear.h"
#include "interp/nearest.h"
#include "math/SH.h"
#include "math/math.h"
#include "math/median.h"
#include "memory.h"
#include "ordered_thread_queue.h"
#include "thread.h"

using namespace MR;
using namespace App;

enum class Statistic { MEAN, MEDIAN, MIN, MAX };
enum class interp_type { NEAREST, LINEAR, PRECISE };
enum class contrast_type { SCALAR, SH };

// clang-format off
void usage ()
{
  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Sample values of an associated image along tracks";

  DESCRIPTION
  + "By default, the value of the underlying image at each point along the track "
    "is written to either an ASCII file (with all values for each track on the same "
    "line), or a track scalar file (.tsf). Alternatively, some statistic can be "
    "taken from the values along each streamline and written to a vector file, "
    "which can either be in the NumPy .npy format or a numerical text file."

  + "In the circumstance where a per-streamline statistic is requested, the input "
    "image can be 4D rather than 3D; in that circumstance, each volume will be sampled "
    "independently, and the output (whether in .npy or text format) will be a matrix, "
    "with one row per streamline and one column per metric."

  + "If the input image is 4D, "
    "and the number of volumes corresponds to an antipodally symmetric spherical harmonics function, "
    "then the -sh option must be specified, "
    "indicating whether the input image should be interpreted as such a function "
    "or whether the input volumes should be sampled individually."

  + "The sampled values may instead be embedded into a tractography dataset as a named "
    "sidecar field, using the qualified \"DATASET::NAME\" form for the output argument. "
    "Per-vertex sampling is stored as a per-vertex (data-per-vertex) field, and a "
    "per-streamline statistic as a per-streamline (data-per-streamline) field (with one "
    "column per metric for a 4D image). If DATASET does not yet exist it is created as a "
    "copy of the input tractogram carrying the new field, generated within the same pass "
    "that performs the sampling. If DATASET already exists and its format supports adding "
    "a field in place (a TRX directory or uncompressed archive), the field is appended "
    "without rewriting the streamline data; the -force option is then required only if a "
    "field named NAME is already present. If DATASET already exists but cannot be augmented "
    "in place (e.g. \".trk\", or a compressed TRX archive), the -force option is required "
    "and the dataset is rewritten with the field added.";

  ARGUMENTS
  + Argument ("tracks", "the input track file").type_tracks_in()
  + Argument ("image",  "the image to be sampled").type_image_in()
  + Argument ("values", "the output sampled values")
    .type_tractogram_data_out(TractogramDataOutMode::MayCreateDataset);

  OPTIONS
  + Option ("stat_tck", "compute some statistic from the values along each streamline;"
                        " options are: " + MR::Enum::join<Statistic>())
    + Argument ("statistic").type_choice<Statistic>()

  + Option ("nointerp", "do not use trilinear interpolation when sampling image values")

  + Option ("precise", "use the precise mechanism for mapping streamlines to voxels "
                       "(obviates the need for trilinear interpolation) "
                       "(only applicable if some per-streamline statistic is requested)")

  + Option ("use_tdi_fraction",
            "each streamline is assigned a fraction of the image intensity "
            "in each voxel based on the fraction of the track density "
            "contributed by that streamline (this is only appropriate for "
            "processing a whole-brain tractogram, and images for which the "
            "quantiative parameter is additive)")

  + Option ("sh",
            "Interpret a 4D image input as representing coefficients of a spherical harmonic function, "
            "and sample the amplitudes of that function along the streamline")
    + Argument ("value").type_bool()

  + Option ("deliberate_vertex_mismatch",
            "(for testing only) deliberately emit one fewer scalar than the number of vertices"
            " for each streamline, to verify that the write-time per-vertex consistency check"
            " (one scalar per vertex) raises a clean error");

  // TODO add support for reading from fixel image
  //   (this would supersede fixel2tsf when used without -precise or -stat_tck options)
  //   (wait until fixel_twi is merged; should simplify)

  REFERENCES
    + "* If using -precise option: " // Internal
    "Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. "
    "SIFT: Spherical-deconvolution informed filtering of tractograms. "
    "NeuroImage, 2013, 67, 298-312";

}
// clang-format on

using value_type = float;
using vector_type = Eigen::VectorXf;
using matrix_type = Eigen::MatrixXf;

struct OnePerStreamline {
  value_type value = std::numeric_limits<value_type>::quiet_NaN();
  size_t index = size_t(-1);
};
struct ManyPerStreamline {
  vector_type values;
  size_t index = size_t(-1);
};

class TDI {
public:
  TDI(Image<value_type> &image, const size_t num_tracks)
      : image(image), progress("Generating initial TDI", num_tracks) {}
  ~TDI() { progress.done(); }

  bool operator()(const DWI::Tractography::Mapping::SetVoxel &in) {
    for (const auto &v : in) {
      assign_pos_of(v, 0, 3).to(image);
      image.value() += v.get_length();
    }
    ++progress;
    return true;
  }

private:
  Image<value_type> &image;
  ProgressBar progress;
};

//! \brief Queue pipe adapting a composite item to the precise track mapper (TDI pre-pass).
/*! Feeds each TractogramItem's streamline to a precise TrackMapperBase, so the
 * TDI pre-pass reads through the same Tractogram framework as the main sampling
 * pass rather than a separate legacy reader. */
class StreamlineMapper {
public:
  StreamlineMapper(const Header &header) : mapper(header) { mapper.set_use_precise_mapping(true); }
  StreamlineMapper(const StreamlineMapper &) = default;
  bool operator()(const DWI::Tractography::TractogramItem<value_type> &in, DWI::Tractography::Mapping::SetVoxel &out) {
    return mapper(in.streamline, out);
  }

private:
  DWI::Tractography::Mapping::TrackMapperBase mapper;
};

class SamplerBase {

public:
  SamplerBase(const contrast_type contrast, const std::optional<Statistic> &statistic) //
      : _contrast(contrast),                                                           //
        _statistic(statistic) {}                                                       //

  SamplerBase(const SamplerBase &that) = default;

  virtual ~SamplerBase() {}

  // Note: While these are shown as virtual,
  //   in the current implementation these are not executed using inheritance,
  //   due to the combination of wanting these classes to execute in multiple threads
  //   but also the functors not being const
  virtual bool operator()(const DWI::Tractography::Streamline<value_type> &tck, //
                          DWI::Tractography::TrackScalar<value_type> &out) = 0; //
  virtual bool operator()(const DWI::Tractography::Streamline<value_type> &tck, //
                          OnePerStreamline &out) = 0;                           //
  virtual bool operator()(const DWI::Tractography::Streamline<value_type> &tck, //
                          ManyPerStreamline &out) = 0;                          //

  virtual size_t num_contrasts() const = 0;
  contrast_type contrast() const { return _contrast; }
  const std::optional<Statistic> &statistic() const { return _statistic; }

protected:
  const contrast_type _contrast;
  const std::optional<Statistic> _statistic;
};

template <class Interp> class SamplerNonPreciseBase : public SamplerBase {
public:
  using BaseType = SamplerBase;
  SamplerNonPreciseBase(Image<value_type> &image,
                        const contrast_type contrast,
                        const std::optional<Statistic> &statistic)
      : BaseType(contrast, statistic), interp(image) {}
  SamplerNonPreciseBase(const SamplerNonPreciseBase &that) = default;
  virtual ~SamplerNonPreciseBase() = default;

  bool operator()(const DWI::Tractography::Streamline<value_type> &tck, OnePerStreamline &out) override {
    assert(statistic().has_value());
    out.index = tck.get_index();
    DWI::Tractography::TrackScalar<value_type> values;
    (*this)(tck, values);
    const std::vector<value_type> weights(statistic() == Statistic::MEAN ? compute_weights(tck)
                                                                         : std::vector<value_type>());
    out.value = compute_statistic(values, weights);
    return true;
  }

  bool operator()(const DWI::Tractography::Streamline<value_type> &tck, ManyPerStreamline &out) override {
    assert(statistic().has_value());
    out.index = tck.get_index();
    matrix_type values;
    (*this)(tck, values);
    const std::vector<value_type> weights(statistic() == Statistic::MEAN ? compute_weights(tck)
                                                                         : std::vector<value_type>());
    out.values.resize(interp.size(3));
    for (size_t i = 0; i != interp.size(3); ++i)
      out.values[i] = compute_statistic(values.col(i), weights);
    return true;
  }

protected:
  Interp interp;

  bool operator()(const DWI::Tractography::Streamline<value_type> &tck,
                  DWI::Tractography::TrackScalar<value_type> &out) override = 0;

  virtual bool operator()(const DWI::Tractography::Streamline<value_type> &tck, matrix_type &out) = 0;

private:
  // Take distance between points into account in mean calculation
  //   (Should help down-weight endpoints)
  std::vector<value_type> compute_weights(const DWI::Tractography::Streamline<value_type> &tck) const {
    std::vector<value_type> weights;
    weights.reserve(tck.size());
    for (size_t i = 0; i != tck.size(); ++i) {
      value_type length = value_type(0);
      if (i > 0)
        length += (tck[i] - tck[i - 1]).norm();
      if (i < tck.size() - 1)
        length += (tck[i + 1] - tck[i]).norm();
      weights.push_back(0.5 * length);
    }
    return weights;
  }

  template <class VectorType>
  value_type compute_statistic(const VectorType &data, const std::vector<value_type> &weights) const {
    if (!statistic().has_value()) {
      assert(false);
      return std::numeric_limits<value_type>::quiet_NaN();
    }
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    switch (statistic().value()) {
    case Statistic::MEAN: {
      value_type integral = value_type(0);
      value_type sum_weights = value_type(0);
      for (size_t i = 0; i != data.size(); ++i) {
        if (!std::isnan(data[i])) {
          integral += data[i] * weights[i];
          sum_weights += weights[i];
        }
      }
      return sum_weights > value_type(0) ? (integral / sum_weights) : std::numeric_limits<value_type>::quiet_NaN();
    }
    case Statistic::MEDIAN: {
      // Don't bother with a weighted median here
      std::vector<value_type> finite_data;
      finite_data.reserve(data.size());
      for (size_t i = 0; i != data.size(); ++i) {
        if (!std::isnan(data[i]))
          finite_data.push_back(data[i]);
      }
      return finite_data.empty() ? std::numeric_limits<value_type>::quiet_NaN() : Math::median(finite_data);
    } break;
    case Statistic::MIN: {
      value_type value = std::numeric_limits<value_type>::infinity();
      bool cast_to_nan = true;
      for (size_t i = 0; i != data.size(); ++i) {
        if (!std::isnan(data[i])) {
          value = std::min(value, data[i]);
          cast_to_nan = false;
        }
      }
      return cast_to_nan ? std::numeric_limits<value_type>::quiet_NaN() : value;
    } break;
    case Statistic::MAX: {
      value_type value = -std::numeric_limits<value_type>::infinity();
      bool cast_to_nan = true;
      for (size_t i = 0; i != data.size(); ++i) {
        if (!std::isnan(data[i])) {
          value = std::max(value, data[i]);
          cast_to_nan = false;
        }
      }
      return cast_to_nan ? std::numeric_limits<value_type>::quiet_NaN() : value;
    } break;
    default:
      assert(false);
      return std::numeric_limits<value_type>::quiet_NaN();
    }
  }
};

class SamplerPreciseBase : public SamplerBase {
public:
  using BaseType = SamplerBase;
  SamplerPreciseBase(Image<value_type> &image, const contrast_type contrast, const std::optional<Statistic> &statistic)
      : BaseType(contrast, statistic), image(image), mapper(new DWI::Tractography::Mapping::TrackMapperBase(image)) {
    assert(statistic.has_value());
    mapper->set_use_precise_mapping(true);
  }
  SamplerPreciseBase(const SamplerPreciseBase &that) = default;
  virtual ~SamplerPreciseBase() = default;

  bool operator()(const DWI::Tractography::Streamline<value_type> & /*tck*/,
                  DWI::Tractography::TrackScalar<value_type> & /*out*/) override {
    throw Exception("Implementation error: No meaningful implementation"
                    " for combining per-vertex output with precise mapping");
    return false;
  }

protected:
  Image<value_type> image;
  std::shared_ptr<DWI::Tractography::Mapping::TrackMapperBase> mapper;

  class ValueLength {
  public:
    ValueLength(const float value, const float length) : value(value), length(length) {}
    bool operator<(const ValueLength &that) const { return value < that.value; }
    float value;
    float length;
  };

  value_type compute_statistic(std::vector<ValueLength> &data) const {
    if (!statistic().has_value()) {
      assert(false);
      return std::numeric_limits<value_type>::quiet_NaN();
    }
    if (data.empty())
      return std::numeric_limits<value_type>::quiet_NaN();
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    switch (statistic().value()) {
    case Statistic::MEAN: {
      value_type integral = value_type(0);
      value_type sum_lengths = value_type(0);
      for (const auto &v : data) {
        if (std::isfinite(v.value)) {
          integral += v.length * v.value;
          sum_lengths += v.length;
        }
      }
      return sum_lengths > value_type(0) ? (integral / sum_lengths) : std::numeric_limits<value_type>::quiet_NaN();
    }
    case Statistic::MEDIAN: {
      std::sort(data.begin(), data.end());
      value_type sum_lengths(value_type(0));
      for (const auto &d : data) {
        if (std::isfinite(d.value))
          sum_lengths += d.length;
      }
      const value_type target_length = 0.5F * sum_lengths;
      sum_lengths = value_type(0);
      value_type prev_value = data.front().value;
      for (const auto &d : data) {
        sum_lengths += d.length;
        if (sum_lengths > target_length)
          return prev_value;
        prev_value = d.value;
      }
      assert(false);
      return std::numeric_limits<value_type>::signaling_NaN();
    }
    case Statistic::MIN: {
      value_type minvalue = std::numeric_limits<value_type>::infinity();
      bool cast_to_nan = true;
      for (const auto &d : data) {
        if (!std::isnan(d.value)) {
          minvalue = std::min(minvalue, d.value);
          cast_to_nan = false;
        }
      }
      return cast_to_nan ? std::numeric_limits<value_type>::quiet_NaN() : minvalue;
    }
    case Statistic::MAX: {
      value_type maxvalue = -std::numeric_limits<value_type>::infinity();
      bool cast_to_nan = true;
      for (const auto &d : data) {
        if (!std::isnan(d.value)) {
          maxvalue = std::max(maxvalue, d.value);
          cast_to_nan = false;
        }
      }
      return cast_to_nan ? std::numeric_limits<value_type>::quiet_NaN() : maxvalue;
    }
    default:
      assert(false);
      return std::numeric_limits<value_type>::quiet_NaN();
    }
  }
};

class SamplerPreciseScalar : public SamplerPreciseBase {
public:
  using BaseType = SamplerPreciseBase;
  using SamplerPreciseBase::ValueLength;
  SamplerPreciseScalar(Image<value_type> &image,
                       const std::optional<Statistic> &statistic,
                       const Image<value_type> &precalc_tdi)
      : BaseType(image, contrast_type::SCALAR, statistic), tdi(precalc_tdi) {}
  SamplerPreciseScalar(const SamplerPreciseScalar &that) = default;

  bool operator()(const DWI::Tractography::Streamline<value_type> & /*tck*/,
                  DWI::Tractography::TrackScalar<value_type> & /*out*/) override {
    throw Exception("Implementation error: No meaningful implementation"
                    "for combining per-vertex output with vertex-wise output");
    return false;
  }

  bool operator()(const DWI::Tractography::Streamline<value_type> &tck, OnePerStreamline &out) override {
    out.index = tck.get_index();
    std::vector<ValueLength> data;
    DWI::Tractography::Mapping::SetVoxel voxels;
    (*mapper)(tck, voxels);
    data = sample(voxels);
    out.value = compute_statistic(data);
    return true;
  }

  bool operator()(const DWI::Tractography::Streamline<value_type> &tck, ManyPerStreamline &out) override {
    out.index = tck.get_index();
    DWI::Tractography::Mapping::SetVoxel voxels;
    (*mapper)(tck, voxels);
    out.values.resize(image.size(3));
    std::vector<ValueLength> data;
    for (auto l = Loop(3)(image); l; ++l) {
      data = sample(voxels);
      out.values[image.index(3)] = compute_statistic(data);
    }
    return true;
  }

  size_t num_contrasts() const override { return BaseType::image.ndim() == 4 ? BaseType::image.size(3) : 1; }

private:
  Image<value_type> tdi;

  value_type get_tdi_multiplier(const DWI::Tractography::Mapping::Voxel &v) {
    if (!tdi.valid())
      return value_type(1);
    assign_pos_of(v).to(tdi);
    assert(!is_out_of_bounds(tdi));
    return v.get_length() / tdi.value();
  }

  std::vector<ValueLength> sample(const DWI::Tractography::Mapping::SetVoxel &voxels) {
    std::vector<ValueLength> data;
    for (const auto &v : voxels) {
      assign_pos_of(v, 0, 3).to(image);
      data.emplace_back(ValueLength(image.value() * get_tdi_multiplier(v), v.get_length()));
    }
    return data;
  }
};

template <class Interp> class SamplerNonPreciseScalar : public SamplerNonPreciseBase<Interp> {
public:
  using BaseType = SamplerNonPreciseBase<Interp>;
  using BaseType::interp;
  SamplerNonPreciseScalar(Image<value_type> &image, const std::optional<Statistic> &statistic)
      : BaseType(image, contrast_type::SCALAR, statistic) {}
  SamplerNonPreciseScalar(const SamplerNonPreciseScalar &that) = default;

  bool operator()(const DWI::Tractography::Streamline<value_type> &tck,
                  DWI::Tractography::TrackScalar<value_type> &out) override {
    out.set_index(tck.get_index());
    out.resize(tck.size());
    for (size_t i = 0; i != tck.size(); ++i) {
      if (interp.scanner(tck[i]))
        out[i] = interp.value();
      else
        out[i] = std::numeric_limits<value_type>::quiet_NaN();
    }
    return true;
  }

  bool operator()(const DWI::Tractography::Streamline<value_type> &tck, OnePerStreamline &out) override {
    return (*this).BaseType::operator()(tck, out);
  }

  bool operator()(const DWI::Tractography::Streamline<value_type> &tck, ManyPerStreamline &out) override {
    return (*this).BaseType::operator()(tck, out);
  }

  size_t num_contrasts() const override { return BaseType::interp.ndim() == 4 ? BaseType::interp.size(3) : 1; }

protected:
  bool operator()(const DWI::Tractography::Streamline<value_type> &tck, matrix_type &out) override {
    assert(interp.ndim() == 4);
    out.resize(tck.size(), interp.size(3));
    for (size_t i = 0; i != tck.size(); ++i) {
      if (interp.scanner(tck[i])) {
        for (auto l = Loop(3)(interp); l; ++l)
          out(i, static_cast<Eigen::Index>(interp.index(3))) = static_cast<value_type>(interp.value());
      } else {
        out.row(i).setConstant(std::numeric_limits<value_type>::quiet_NaN());
      }
    }
    return true;
  }
};

template <class Interp> class SamplerNonPreciseSH : public SamplerNonPreciseBase<Interp> {
public:
  using BaseType = SamplerNonPreciseBase<Interp>;
  using BaseType::interp;
  SamplerNonPreciseSH(Image<value_type> &image, const std::optional<Statistic> &statistic)
      : BaseType(image, contrast_type::SH, statistic),
        sh_precomputer(std::make_shared<Math::SH::PrecomputedAL<default_type>>()),
        sh_coeffs(image.size(3)) {
    Math::SH::check(image);
    sh_precomputer->init(Math::SH::LforN(image.size(3)));
  }
  SamplerNonPreciseSH(const SamplerNonPreciseSH &that) = default;

  bool operator()(const DWI::Tractography::Streamline<value_type> &tck,
                  DWI::Tractography::TrackScalar<value_type> &out) override {
    out.set_index(tck.get_index());
    out.resize(tck.size());
    switch (tck.size()) {
    case 0:
      return true;
    case 1:
      out[0] = std::numeric_limits<value_type>::quiet_NaN();
      return true;
    default:
      break;
    }
    for (size_t i = 0; i != tck.size(); ++i) {
      if (interp.scanner(tck[i])) {
        for (interp.index(3) = 0; interp.index(3) != interp.size(3); ++interp.index(3))
          sh_coeffs[interp.index(3)] = interp.value();
        const Eigen::Vector3f dir =
            (tck[(i == (tck.size() - 1)) ? i : (i + 1)] - tck[i > 0 ? (i - 1) : 0]).normalized();
        out[i] = sh_precomputer->value(sh_coeffs, dir);
      } else {
        out[i] = std::numeric_limits<value_type>::quiet_NaN();
      }
    }
    return true;
  }

  bool operator()(const DWI::Tractography::Streamline<value_type> &tck, OnePerStreamline &out) override {
    return (*this).BaseType::operator()(tck, out);
  }

  bool operator()(const DWI::Tractography::Streamline<value_type> &tck, ManyPerStreamline &out) override {
    return (*this).BaseType::operator()(tck, out);
  }

  size_t num_contrasts() const override { return 1; }

protected:
  bool operator()(const DWI::Tractography::Streamline<value_type> & /*tck*/, matrix_type & /*out*/) override {
    throw Exception("Implementation error:"
                    "No support for sampling multiple SH contrasts");
    return false;
  }

private:
  std::shared_ptr<Math::SH::PrecomputedAL<default_type>> sh_precomputer;
  vector_type sh_coeffs;
};

class SamplerPreciseSH : public SamplerPreciseBase {
public:
  using BaseType = SamplerPreciseBase;
  using SamplerPreciseBase::ValueLength;
  SamplerPreciseSH(Image<value_type> &image, const std::optional<Statistic> &statistic)
      : BaseType(image, contrast_type::SH, statistic),
        sh_precomputer(std::make_shared<Math::SH::PrecomputedAL<default_type>>()),
        sh_coeffs(image.size(3)) {
    assert(statistic.has_value());
    sh_precomputer->init(Math::SH::LforN(image.size(3)));
  }
  SamplerPreciseSH(const SamplerPreciseSH &that) = default;

  bool operator()(const DWI::Tractography::Streamline<value_type> & /*tck*/,
                  DWI::Tractography::TrackScalar<value_type> & /*out*/) override {
    throw Exception("Implementation error: No meaningful implementation"
                    "for combining per-vertex output with precise mapping");
    return false;
  }

  bool operator()(const DWI::Tractography::Streamline<value_type> &tck, OnePerStreamline &out) override {
    out.index = tck.get_index();
    std::vector<ValueLength> data;
    DWI::Tractography::Mapping::SetVoxelDir intersections;
    (*mapper)(tck, intersections);
    data = sample(intersections);
    out.value = compute_statistic(data);
    return true;
  }

  bool operator()(const DWI::Tractography::Streamline<value_type> &tck, ManyPerStreamline &out) override {
    throw Exception("Implementation error: unable to sample multiple SH contrasts");
    return false;
  }

  size_t num_contrasts() const override { return 1; }

private:
  std::shared_ptr<Math::SH::PrecomputedAL<default_type>> sh_precomputer;
  vector_type sh_coeffs;

  std::vector<ValueLength> sample(const DWI::Tractography::Mapping::SetVoxelDir &intersections) {
    std::vector<ValueLength> data;
    for (const auto &i : intersections) {
      assign_pos_of(i, 0, 3).to(image);
      sh_coeffs = image.row(3);
      data.emplace_back(ValueLength(sh_precomputer->value(sh_coeffs, i.get_dir()), i.get_length()));
    }
    return data;
  }
};

class ReceiverBase {
public:
  ReceiverBase(const size_t num_tracks, const bool ordered, const std::filesystem::path &path)
      : received(0),
        path(path),
        expected(num_tracks),
        process_ordered(ordered),
        progress("Sampling values underlying streamlines", num_tracks) {}

  ReceiverBase(const ReceiverBase &) = delete;

  virtual ~ReceiverBase() {
    if (received != expected)
      WARN("Track file reports " + str(expected) + " tracks, but contains " + str(received));
  }

  bool ordered() const { return process_ordered; }

protected:
  void operator++() {
    ++received;
    ++progress;
  }

  size_t received;
  const std::filesystem::path path;

private:
  const size_t expected;
  const bool process_ordered;
  ProgressBar progress;
};

class Receiver_OnePerStreamline : public ReceiverBase {
public:
  using InputType = OnePerStreamline;
  Receiver_OnePerStreamline(const size_t num_tracks, const std::filesystem::path &path)
      : ReceiverBase(num_tracks, false, path), data(vector_type::Zero(num_tracks)) {}
  Receiver_OnePerStreamline(const Receiver_OnePerStreamline &) = delete;
  ~Receiver_OnePerStreamline() { File::Matrix::save_vector(data, path); }

  bool operator()(InputType &in) {
    if (in.index >= static_cast<size_t>(data.size()))
      data.conservativeResizeLike(vector_type::Zero(in.index + 1));
    data[in.index] = in.value;
    ++(*this);
    return true;
  }

  void save(const std::filesystem::path &path) { File::Matrix::save_vector(data, path); }

private:
  vector_type data;
};

class Receiver_ManyPerStreamline : public ReceiverBase {
public:
  using InputType = ManyPerStreamline;
  Receiver_ManyPerStreamline(const size_t num_tracks, const size_t num_metrics, const std::filesystem::path &path)
      : ReceiverBase(num_tracks, false, path), data(matrix_type::Zero(num_tracks, num_metrics)) {}
  Receiver_ManyPerStreamline(const Receiver_ManyPerStreamline &) = delete;
  ~Receiver_ManyPerStreamline() { File::Matrix::save_matrix(data, path); }

  bool operator()(InputType &in) {
    // TODO Chance that this will be prohibitively slow if count is not indicated in track file header
    if (in.index >= static_cast<size_t>(data.rows()))
      data.conservativeResizeLike(matrix_type::Zero(in.index + 1, data.cols()));
    data.row(in.index) = in.values;
    ++(*this);
    return true;
  }

private:
  matrix_type data;
};

//! \brief A sampled per-vertex scalar sequence plus its source vertex count.
/*! Carries the number of vertices in the streamline that produced this scalar
 * sequence (step 7), so that the receiver can perform a cheap write-time
 * consistency check (scalar length == streamline vertex count) without a second
 * pass over the tractogram. */
struct PerVertexScalar : public DWI::Tractography::TrackScalar<value_type> {
  size_t source_vertices = 0;
};

//! \brief Queue pipe that runs a sampler on each TractogramItem's streamline.
/*! The single source for all standalone-output modes: the input is read through
 * the Tractogram framework as a composite TractogramItem (vertices + any existing
 * sidecar), and this worker feeds item.streamline to the chosen sampler and emits
 * the mode-specific result the receiver expects. The per-vertex overload also
 * records the streamline's vertex count so the receiver can perform its O(1)
 * write-time consistency check (one scalar per vertex) without a second pass. */
template <class Sampler> class SampleWorker {
public:
  SampleWorker(Sampler &&sampler, const bool deliberate_mismatch = false)
      : sampler(std::move(sampler)), deliberate_mismatch(deliberate_mismatch) {}
  SampleWorker(const SampleWorker &) = default;

  bool operator()(const DWI::Tractography::TractogramItem<value_type> &in, OnePerStreamline &out) {
    return sampler(in.streamline, out);
  }

  bool operator()(const DWI::Tractography::TractogramItem<value_type> &in, ManyPerStreamline &out) {
    return sampler(in.streamline, out);
  }

  bool operator()(const DWI::Tractography::TractogramItem<value_type> &in, PerVertexScalar &out) {
    DWI::Tractography::TrackScalar<value_type> scalar;
    sampler(in.streamline, scalar);
    out.clear();
    out.set_index(in.get_index());
    for (const value_type v : scalar)
      out.push_back(v);
    out.source_vertices = in.streamline.size();
    // Test-only fault injection: deliberately corrupt the scalar length so the
    //   receiver's write-time consistency check is exercised.
    if (deliberate_mismatch && !out.empty())
      out.pop_back();
    return true;
  }

private:
  Sampler sampler;
  bool deliberate_mismatch;
};

class Receiver_PerVertex : public ReceiverBase {
public:
  using InputType = PerVertexScalar;
  Receiver_PerVertex(const DWI::Tractography::Properties &properties,
                     const size_t num_tracks,
                     const std::filesystem::path &path)
      : ReceiverBase(num_tracks, true, path) {
    if (Path::has_suffix(path, ".tsf")) {
      tsf.reset(new DWI::Tractography::ScalarWriter<value_type>(path, properties));
    } else {
      ascii.reset(new File::OFStream(path));
      (*ascii) << "# " << App::command_history_string << "\n";
    }
  }
  Receiver_PerVertex(const Receiver_PerVertex &) = delete;

  bool operator()(const InputType &in) {
    // Requires preservation of order
    assert(in.get_index() == ReceiverBase::received);
    // Write-time consistency check (step 7): the emitted per-vertex scalar
    //   sequence must contain exactly one value per streamline vertex. This is a
    //   cheap O(1) invariant at the write boundary, not a second pass.
    if (in.size() != in.source_vertices)
      throw Exception("Inconsistent per-vertex output for streamline " + str(in.get_index()) + ":" + //
                      " produced " + str(in.size()) + " scalar value(s)" +                           //
                      " for a streamline of " + str(in.source_vertices) + " vertices");              //
    if (ascii) {
      if (!in.empty()) {
        auto i = in.begin();
        (*ascii) << *i;
        for (++i; i != in.end(); ++i)
          (*ascii) << " " << *i;
      }
      (*ascii) << "\n";
    } else {
      (*tsf)(in);
    }
    ++(*this);
    return true;
  }

private:
  std::unique_ptr<File::OFStream> ascii;
  std::unique_ptr<DWI::Tractography::ScalarWriter<value_type>> tsf;
};

//! \brief whether the per-vertex producer should be deliberately corrupted (test).
bool deliberate_vertex_mismatch() { return !App::get_options("deliberate_vertex_mismatch").empty(); }

//! \brief construct the sampler for the (contrast, interp) pair and invoke \a fn with it.
/*! Centralises the 2×3 sampler-type selection so every output path (standalone file
 * and tractogram embedding) shares one construction site. \a fn is a generic
 * callable receiving the freshly-constructed sampler by forwarding reference. */
template <class Fn>
void with_sampler(Image<value_type> &image,
                  const interp_type interp,
                  const contrast_type contrast,
                  const std::optional<Statistic> &statistic,
                  Image<value_type> &tdi,
                  Fn &&fn) {
  switch (contrast) {
  case contrast_type::SH:
    switch (interp) {
    case interp_type::NEAREST:
      fn(SamplerNonPreciseSH<Interp::Nearest<Image<value_type>>>(image, statistic));
      break;
    case interp_type::LINEAR:
      fn(SamplerNonPreciseSH<Interp::Linear<Image<value_type>>>(image, statistic));
      break;
    case interp_type::PRECISE:
      fn(SamplerPreciseSH(image, statistic));
      break;
    }
    break;
  case contrast_type::SCALAR:
    switch (interp) {
    case interp_type::NEAREST:
      fn(SamplerNonPreciseScalar<Interp::Nearest<Image<value_type>>>(image, statistic));
      break;
    case interp_type::LINEAR:
      fn(SamplerNonPreciseScalar<Interp::Linear<Image<value_type>>>(image, statistic));
      break;
    case interp_type::PRECISE:
      fn(SamplerPreciseScalar(image, statistic, tdi));
      break;
    }
    break;
  }
}

//! \brief run the sampling queue into a standalone-file receiver.
template <class ReceiverType>
void run_standalone(DWI::Tractography::Tractogram<value_type> &input,
                    Image<value_type> &image,
                    const interp_type interp,
                    const contrast_type contrast,
                    const std::optional<Statistic> &statistic,
                    Image<value_type> &tdi,
                    ReceiverType &receiver) {
  using Item = DWI::Tractography::TractogramItem<value_type>;
  using OutType = typename ReceiverType::InputType;
  const bool deliberate = deliberate_vertex_mismatch();
  with_sampler(image, interp, contrast, statistic, tdi, [&](auto &&sampler) {
    SampleWorker<std::decay_t<decltype(sampler)>> worker(std::move(sampler), deliberate);
    if (receiver.ordered())
      Thread::run_ordered_queue(
          input, Thread::batch(Item()), Thread::multi(worker), Thread::batch(OutType()), receiver);
    else
      Thread::run_queue(input, Thread::batch(Item()), Thread::multi(worker), Thread::batch(OutType()), receiver);
  });
}

//! \brief Queue pipe that samples each streamline and slots the result into a new
//!   sidecar field of the same composite item, carrying all other input data through.
/*! The output item is the input item plus one extra field at \a ordinal: a
 * per-vertex (dpv) scalar column when no statistic is requested, or a
 * per-streamline (dps) row of \a columns values otherwise. This lets the output
 * tractogram be generated by the sampling queue itself, with no second pass over
 * the input. */
template <class Sampler> class EmbedWorker {
public:
  EmbedWorker(Sampler &&sampler, const DWI::Tractography::FieldRole role, const size_t ordinal, const size_t columns)
      : sampler(std::move(sampler)), role(role), ordinal(ordinal), columns(columns) {}
  EmbedWorker(const EmbedWorker &) = default;

  bool operator()(const DWI::Tractography::TractogramItem<value_type> &in,
                  DWI::Tractography::TractogramItem<value_type> &out) {
    out = in;
    if (role == DWI::Tractography::FieldRole::DPV) {
      DWI::Tractography::TrackScalar<value_type> scalar;
      sampler(in.streamline, scalar);
      DWI::Tractography::VectorOrMatrix<value_type> column(static_cast<Eigen::Index>(scalar.size()), 1);
      for (size_t i = 0; i != scalar.size(); ++i)
        column(static_cast<Eigen::Index>(i), 0) = scalar[i];
      if (out.dpv.size() <= ordinal)
        out.dpv.resize(ordinal + 1);
      out.dpv[ordinal] = DWI::Tractography::make_dpv(std::move(column));
    } else {
      DWI::Tractography::ScalarOrVector<value_type> row(static_cast<Eigen::Index>(columns));
      if (columns == 1) {
        OnePerStreamline one;
        sampler(in.streamline, one);
        row(0, 0) = one.value;
      } else {
        ManyPerStreamline many;
        sampler(in.streamline, many);
        for (size_t i = 0; i != columns; ++i)
          row(0, static_cast<Eigen::Index>(i)) = many.values[static_cast<Eigen::Index>(i)];
      }
      if (out.dps.size() <= ordinal)
        out.dps.resize(ordinal + 1);
      out.dps[ordinal] = DWI::Tractography::make_dps(std::move(row));
    }
    return true;
  }

private:
  Sampler sampler;
  DWI::Tractography::FieldRole role;
  size_t ordinal;
  size_t columns;
};

//! \brief embed the sampled values into a tractography dataset via "DATASET::NAME".
/*! Writes the sampled column as a new per-streamline (dps) or per-vertex (dpv) field
 * named NAME within the dataset, choosing in-place append vs whole-dataset create/
 * rewrite from the format's capabilities (§2.7). The destination decision and the
 * three write forms are handled by the shared embed_sidecar_field() orchestration;
 * this command supplies only the sampling queue, so the output tractogram (input
 * streamlines + the new field) is produced by that queue without a second pass. */
void run_embed(const DWI::Tractography::SidecarReference &reference,
               DWI::Tractography::Tractogram<value_type> &input,
               DWI::Tractography::Properties &properties,
               const size_t num_tracks,
               Image<value_type> &image,
               const interp_type interp,
               const contrast_type contrast,
               const std::optional<Statistic> &statistic,
               Image<value_type> &tdi,
               const size_t num_metrics) {
  using namespace DWI::Tractography;
  using Item = TractogramItem<value_type>;
  const FieldRole role = statistic.has_value() ? FieldRole::DPS : FieldRole::DPV;
  const size_t columns = (role == FieldRole::DPS) ? num_metrics : 1;

  embed_sidecar_field<value_type>(
      reference, input, properties, role, columns, num_tracks, [&](auto &sink, const size_t ordinal) {
        with_sampler(image, interp, contrast, statistic, tdi, [&](auto &&sampler) {
          EmbedWorker<std::decay_t<decltype(sampler)>> worker(std::move(sampler), role, ordinal, columns);
          Thread::run_ordered_queue(input, Thread::batch(Item()), Thread::multi(worker), Thread::batch(Item()), sink);
        });
      });
}

void run() {
  auto H = Header::open(argument[1]);
  const bool plausibly_SH =
      H.ndim() > 3 && Math::SH::NforL(Math::SH::LforN(static_cast<int>(H.size(3)))) == static_cast<size_t>(H.size(3));
  auto opt = get_options("sh");
  contrast_type contrast(contrast_type::SCALAR);
  if (opt.empty()) {
    if (plausibly_SH) {
      // clang-format off
      throw Exception(
          std::string("Input image could plausibly be interpreted as spherical harmonics; " //
                      "must specify the -sh option to inform command"                       //
                      " whether to interpret image in this way") +                          //
          ((H.ndim() == 4 && H.size(3) == 1) ?                                              //
               " (this is due to being a 4D image with 1 volume rather than a 3D image)" :  //
               ""));                                                                        //
      // clang-format on
    }
  } else if (plausibly_SH) {
    if (static_cast<bool>(opt[0][0])) {
      DEBUG("User specified -sh true, "
            "and image can be interpreted as spherical harmonics; "
            "spherical harmonics sampling will be used");
      contrast = contrast_type::SH;
    } else {
      DEBUG("User specified -sh false, "
            "so even though image could be reasonably interpreted as spherical harmonics, "
            "it will instead be sampled as individual volumes");
    }
  } else {
    if (static_cast<bool>(opt[0][0]))
      throw Exception("Cannot sample spherical harmonic function amplitudes, "
                      "as input image cannot be interpreted as such");
    WARN("Specification of -sh false was unnecessary, "
         "as input image could not be interpreted as spherical harmonics functions");
  }

  const auto statistic = get_optional<Statistic>("stat_tck");

  if (H.ndim() == 4 && H.size(3) > 1 && contrast == contrast_type::SCALAR) {
    if (!statistic.has_value())
      throw Exception("Cannot export per-vertex values for more than one contrast");
    INFO("Input image is 4D; output will be 2D matrix");
  } else if (H.ndim() > 4) {
    throw Exception("Input image is of unsupported dimensionality");
  }

  const bool nointerp = !get_options("nointerp").empty();
  const bool precise = !get_options("precise").empty();
  if (nointerp && precise)
    throw Exception("Options -nointerp and -precise are mutually exclusive");
  const interp_type interp = nointerp ? interp_type::NEAREST : (precise ? interp_type::PRECISE : interp_type::LINEAR);
  if (!statistic.has_value() && interp == interp_type::PRECISE)
    throw Exception("Cannot combine per-vertex values with precise mapping mechanism");

  Image<value_type> tdi;
  if (!get_options("use_tdi_fraction").empty()) {
    if (!statistic.has_value())
      throw Exception("Cannot use -use_tdi_fraction option unless a per-streamline statistic is used");
    if (contrast == contrast_type::SH)
      throw Exception("Cannot use -use_tdi_fraction option in conjunction with SH function sampling");
    if (interp != interp_type::PRECISE)
      throw Exception("-use_tdi_fraction can only be used in conjunction with precise mapping");
    // Independent first pass over the input to accumulate the track density image;
    //   a Tractogram reader is single-pass, so the main sampling pass below opens
    //   the input afresh.
    DWI::Tractography::Properties tdi_properties;
    auto tdi_input = DWI::Tractography::Tractogram<value_type>::open(argument[0], tdi_properties);
    const size_t tdi_count =
        tdi_properties.find("count") == tdi_properties.end() ? 0 : to<size_t>(tdi_properties["count"]);
    StreamlineMapper mapper(H);
    tdi = Image<value_type>::scratch(H, "TDI scratch image");
    TDI tdi_fill(tdi, tdi_count);
    Thread::run_queue(tdi_input,
                      Thread::batch(DWI::Tractography::TractogramItem<value_type>()),
                      Thread::multi(mapper),
                      Thread::batch(DWI::Tractography::Mapping::SetVoxel()),
                      tdi_fill);
  }

  auto image = H.get_image<value_type>();

  const DWI::Tractography::SidecarReference reference =
      DWI::Tractography::parse_sidecar_reference(argument[2].as_text());

  DWI::Tractography::Properties properties;
  auto input = DWI::Tractography::Tractogram<value_type>::open(argument[0], properties);
  const size_t num_tracks = properties.find("count") == properties.end() ? 0 : to<size_t>(properties["count"]);
  const size_t num_metrics = image.ndim() == 4 && contrast == contrast_type::SCALAR ? image.size(3) : 1;

  // A bare path that no tractography handler recognises is a standalone output file
  //   (.tsf / ASCII / .csv / .npy), exactly as before; only the input now flows
  //   through the Tractogram framework rather than the legacy reader.
  if (DWI::Tractography::Formats::get_handler(reference.dataset) == nullptr) {
    if (reference.is_qualified())
      throw Exception("output \"" + std::string(argument[2].as_text()) + "\"" +
                      " uses the qualified \"DATASET::NAME\" sidecar form," + " but \"" + reference.dataset.string() +
                      "\" is not a recognised tractography format");
    if (!statistic.has_value()) {
      Receiver_PerVertex receiver(properties, num_tracks, reference.dataset);
      run_standalone(input, image, interp, contrast, statistic, tdi, receiver);
    } else if (num_metrics == 1) {
      Receiver_OnePerStreamline receiver(num_tracks, reference.dataset);
      run_standalone(input, image, interp, contrast, statistic, tdi, receiver);
    } else {
      Receiver_ManyPerStreamline receiver(num_tracks, num_metrics, reference.dataset);
      run_standalone(input, image, interp, contrast, statistic, tdi, receiver);
    }
    return;
  }

  run_embed(reference, input, properties, num_tracks, image, interp, contrast, statistic, tdi, num_metrics);
}
