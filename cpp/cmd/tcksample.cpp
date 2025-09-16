/* Copyright (c) 2008-2022 the MRtrix3 contributors.
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

#include "command.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/mapping/mapper.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/scalar_file.h"
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

enum stat_tck { MEAN, MEDIAN, MIN, MAX, NONE };
const std::vector<std::string> statistics = {"mean", "median", "min", "max"};
enum interp_type { NEAREST, LINEAR, PRECISE };
enum contrast_type { SCALAR, SH };

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
    "or whether the input volumes should be sampled individually.";

  ARGUMENTS
  + Argument ("tracks", "the input track file").type_tracks_in()
  + Argument ("image",  "the image to be sampled").type_image_in()
  + Argument ("values", "the output sampled values").type_file_out();

  OPTIONS

  + Option ("stat_tck", "compute some statistic from the values along each streamline "
                        "(options are: " + join(statistics, ",") + ")")
    + Argument ("statistic").type_choice (statistics)

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
    + Argument ("value").type_bool();

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
  value_type value;
  size_t index;
};
struct ManyPerStreamline {
  vector_type values;
  size_t index;
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

protected:
  Image<value_type> &image;
  ProgressBar progress;
};

class SamplerBase {

public:
  SamplerBase(const contrast_type contrast, const stat_tck statistic) //
      : _contrast(contrast),                                          //
        _statistic(statistic) {}                                      //
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
  stat_tck statistic() const { return _statistic; }

protected:
  const contrast_type _contrast;
  const stat_tck _statistic;
};

template <class Interp> class SamplerNonPreciseBase : public SamplerBase {
public:
  using BaseType = SamplerBase;
  SamplerNonPreciseBase(Image<value_type> &image, const contrast_type contrast, const stat_tck statistic)
      : BaseType(contrast, statistic), interp(image) {}
  virtual ~SamplerNonPreciseBase() {}

  bool operator()(const DWI::Tractography::Streamline<value_type> &tck, OnePerStreamline &out) override {
    assert(statistic() != stat_tck::NONE);
    out.index = tck.get_index();
    DWI::Tractography::TrackScalar<value_type> values;
    (*this)(tck, values);
    std::vector<value_type> weights(statistic() == stat_tck::MEAN ? compute_weights(tck) : std::vector<value_type>());
    out.value = compute_statistic(values, weights);
    return true;
  }

  bool operator()(const DWI::Tractography::Streamline<value_type> &tck, ManyPerStreamline &out) override {
    assert(statistic() != stat_tck::NONE);
    out.index = tck.get_index();
    matrix_type values;
    (*this)(tck, values);
    std::vector<value_type> weights(statistic() == stat_tck::MEAN ? compute_weights(tck) : std::vector<value_type>());
    out.values.resize(interp.size(3));
    for (size_t i = 0; i != interp.size(3); ++i)
      out.values[i] = compute_statistic(values.col(i), weights);
    return true;
  }

protected:
  Interp interp;

  virtual bool operator()(const DWI::Tractography::Streamline<value_type> &tck,
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
      if (i)
        length += (tck[i] - tck[i - 1]).norm();
      if (i < tck.size() - 1)
        length += (tck[i + 1] - tck[i]).norm();
      weights.push_back(0.5 * length);
    }
    return weights;
  }

  template <class VectorType>
  value_type compute_statistic(const VectorType &data, const std::vector<value_type> &weights) const {
    switch (statistic()) {
    case stat_tck::MEAN: {
      value_type integral = value_type(0), sum_weights = value_type(0);
      for (size_t i = 0; i != data.size(); ++i) {
        if (!std::isnan(data[i])) {
          integral += data[i] * weights[i];
          sum_weights += weights[i];
        }
      }
      return sum_weights ? (integral / sum_weights) : std::numeric_limits<value_type>::quiet_NaN();
    }
    case stat_tck::MEDIAN: {
      // Don't bother with a weighted median here
      std::vector<value_type> finite_data;
      finite_data.reserve(data.size());
      for (size_t i = 0; i != data.size(); ++i) {
        if (!std::isnan(data[i]))
          finite_data.push_back(data[i]);
      }
      return finite_data.size() ? Math::median(finite_data) : std::numeric_limits<value_type>::quiet_NaN();
    } break;
    case stat_tck::MIN: {
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
    case stat_tck::MAX: {
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
      assert(0);
      return std::numeric_limits<value_type>::signaling_NaN();
    }
  }
};

class SamplerPreciseBase : public SamplerBase {
public:
  using BaseType = SamplerBase;
  SamplerPreciseBase(Image<value_type> &image, const contrast_type contrast, const stat_tck statistic)
      : BaseType(contrast, statistic), image(image), mapper(new DWI::Tractography::Mapping::TrackMapperBase(image)) {
    assert(statistic != stat_tck::NONE);
    mapper->set_use_precise_mapping(true);
  }
  virtual ~SamplerPreciseBase() {}

  bool operator()(const DWI::Tractography::Streamline<value_type> &tck,
                  DWI::Tractography::TrackScalar<value_type> &out) override {
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
    if (!data.size())
      return std::numeric_limits<value_type>::quiet_NaN();
    switch (statistic()) {
    case stat_tck::MEAN: {
      value_type integral = value_type(0), sum_lengths = value_type(0);
      for (const auto &v : data) {
        if (std::isfinite(v.value)) {
          integral += v.length * v.value;
          sum_lengths += v.length;
        }
      }
      return sum_lengths ? (integral / sum_lengths) : std::numeric_limits<value_type>::quiet_NaN();
    }
    case stat_tck::MEDIAN: {
      std::sort(data.begin(), data.end());
      value_type sum_lengths(value_type(0));
      for (const auto &d : data) {
        if (std::isfinite(d.value))
          sum_lengths += d.length;
      }
      const value_type target_length = 0.5 * sum_lengths;
      sum_lengths = value_type(0);
      value_type prev_value = data.front().value;
      for (const auto &d : data) {
        if ((sum_lengths += d.length) > target_length)
          return prev_value;
        prev_value = d.value;
      }
      assert(false);
      return std::numeric_limits<value_type>::signaling_NaN();
    }
    case stat_tck::MIN: {
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
    case stat_tck::MAX: {
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
    default: {
      assert(0);
      return std::numeric_limits<value_type>::signaling_NaN();
    }
    }
  }
};

class SamplerPreciseScalar : public SamplerPreciseBase {
public:
  using BaseType = SamplerPreciseBase;
  using SamplerPreciseBase::ValueLength;
  SamplerPreciseScalar(Image<value_type> &image, const stat_tck statistic, const Image<value_type> &precalc_tdi)
      : BaseType(image, contrast_type::SCALAR, statistic), tdi(precalc_tdi) {}

  bool operator()(const DWI::Tractography::Streamline<value_type> &tck,
                  DWI::Tractography::TrackScalar<value_type> &out) override {
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
  SamplerNonPreciseScalar(Image<value_type> &image, const stat_tck statistic)
      : BaseType(image, contrast_type::SCALAR, statistic) {}

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
          out(i, ssize_t(interp.index(3))) = value_type(interp.value());
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
  SamplerNonPreciseSH(Image<value_type> &image, const stat_tck statistic)
      : BaseType(image, contrast_type::SH, statistic),
        sh_precomputer(std::make_shared<Math::SH::PrecomputedAL<default_type>>()),
        sh_coeffs(image.size(3)) {
    Math::SH::check(image);
    sh_precomputer->init(Math::SH::LforN(image.size(3)));
  }

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
            (tck[(i == ssize_t(tck.size() - 1)) ? i : (i + 1)] - tck[i ? (i - 1) : 0]).normalized();
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
  bool operator()(const DWI::Tractography::Streamline<value_type> &tck, matrix_type &out) override {
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
  SamplerPreciseSH(Image<value_type> &image, const stat_tck statistic)
      : BaseType(image, contrast_type::SH, statistic),
        sh_precomputer(std::make_shared<Math::SH::PrecomputedAL<default_type>>()),
        sh_coeffs(image.size(3)) {
    assert(statistic != stat_tck::NONE);
    sh_precomputer->init(Math::SH::LforN(image.size(3)));
  }

  bool operator()(const DWI::Tractography::Streamline<value_type> &tck,
                  DWI::Tractography::TrackScalar<value_type> &out) override {
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
    throw Exception("Implementation error:"
                    " Unable to sample multiple SH contrasts");
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
  ReceiverBase(const size_t num_tracks, const bool ordered, const std::string &path)
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

protected:
  const std::string path;

private:
  const size_t expected;
  const bool process_ordered;
  ProgressBar progress;
};

class Receiver_OnePerStreamline : public ReceiverBase {
public:
  using InputType = OnePerStreamline;
  Receiver_OnePerStreamline(const size_t num_tracks, const std::string &path)
      : ReceiverBase(num_tracks, false, path), data(vector_type::Zero(num_tracks)) {}
  Receiver_OnePerStreamline(const Receiver_OnePerStreamline &) = delete;
  ~Receiver_OnePerStreamline() { File::Matrix::save_vector(data, path); }

  bool operator()(InputType &in) {
    if (in.index >= size_t(data.size()))
      data.conservativeResizeLike(vector_type::Zero(in.index + 1));
    data[in.index] = in.value;
    ++(*this);
    return true;
  }

  void save(const std::string &path) { File::Matrix::save_vector(data, path); }

private:
  vector_type data;
};

class Receiver_ManyPerStreamline : public ReceiverBase {
public:
  using InputType = ManyPerStreamline;
  Receiver_ManyPerStreamline(const size_t num_tracks, const size_t num_metrics, const std::string &path)
      : ReceiverBase(num_tracks, false, path), data(matrix_type::Zero(num_tracks, num_metrics)) {}
  Receiver_ManyPerStreamline(const Receiver_ManyPerStreamline &) = delete;
  ~Receiver_ManyPerStreamline() { File::Matrix::save_matrix(data, path); }

  bool operator()(InputType &in) {
    // TODO Chance that this will be prohibitively slow if count is not indicated in track file header
    if (in.index >= size_t(data.rows()))
      data.conservativeResizeLike(matrix_type::Zero(in.index + 1, data.cols()));
    data.row(in.index) = in.values;
    ++(*this);
    return true;
  }

private:
  matrix_type data;
};

class Receiver_PerVertex : public ReceiverBase {
public:
  using InputType = DWI::Tractography::TrackScalar<value_type>;
  Receiver_PerVertex(const DWI::Tractography::Properties &properties, const size_t num_tracks, const std::string &path)
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
    if (ascii) {
      if (in.size()) {
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

/*
template <class InterpType>
void execute_nostat (DWI::Tractography::Reader<value_type>& reader,
                     const DWI::Tractography::Properties& properties,
                     const size_t num_tracks,
                     Image<value_type>& image,
                     const bool sample_sh,
                     const std::string& path)
{
  SamplerNonPrecise<InterpType> sampler (image, sample_sh, stat_tck::NONE, Image<value_type>());
  Receiver_PerVertex receiver (path, num_tracks, properties);
  Thread::run_ordered_queue (reader,
                             Thread::batch (DWI::Tractography::Streamline<value_type>()),
                             Thread::multi (sampler),
                             Thread::batch (DWI::Tractography::TrackScalar<value_type>()),
                             receiver);
}

template <class SamplerType>
void execute (DWI::Tractography::Reader<value_type>& reader,
              const size_t num_tracks,
              Image<value_type>& image,
              const bool sample_sh,
              const stat_tck statistic,
              Image<value_type>& tdi,
              const std::string& path)
{
  SamplerType sampler (image, sample_sh, statistic, tdi);
  const size_t num_metrics = image.ndim() == 4 && !sample_sh ? image.size(3) : 1;
  if (num_metrics == 1) {
    Receiver_OnePerStreamline receiver (num_tracks);
    Thread::run_ordered_queue (reader,
                              Thread::batch (DWI::Tractography::Streamline<value_type>()),
                              Thread::multi (sampler),
                              Thread::batch (OnePerStreamline()),
                              receiver);
    receiver.save (path);
  } else {
    Receiver_ManyPerStreamline receiver (num_tracks, num_metrics);
    Thread::run_ordered_queue (reader,
                              Thread::batch (DWI::Tractography::Streamline<value_type>()),
                              Thread::multi (sampler),
                              Thread::batch (ManyPerStreamline()),
                              receiver);
    receiver.save (path);
  }
}
*/

template <class SamplerType, class ReceiverType>
void execute(DWI::Tractography::Reader<value_type> &reader, SamplerType &sampler, ReceiverType &receiver) {
  if (receiver.ordered())
    Thread::run_ordered_queue(reader,
                              Thread::batch(DWI::Tractography::Streamline<value_type>()),
                              Thread::multi(sampler),
                              Thread::batch(typename ReceiverType::InputType()),
                              receiver);
  else
    Thread::run_queue(reader,
                      Thread::batch(DWI::Tractography::Streamline<value_type>()),
                      Thread::multi(sampler),
                      Thread::batch(typename ReceiverType::InputType()),
                      receiver);
}

template <class ReceiverType>
void execute(DWI::Tractography::Reader<value_type> &reader,
             Image<value_type> &image,
             const interp_type interp,
             const bool sample_sh,
             const stat_tck statistic,
             Image<value_type> &tdi,
             ReceiverType &receiver) {
  if (sample_sh) {
    switch (interp) {
    case interp_type::NEAREST: {
      SamplerNonPreciseSH<Interp::Nearest<Image<value_type>>> sampler(image, statistic);
      execute(reader, sampler, receiver);
    } break;
    case interp_type::LINEAR: {
      SamplerNonPreciseSH<Interp::Linear<Image<value_type>>> sampler(image, statistic);
      execute(reader, sampler, receiver);
    } break;
    case interp_type::PRECISE: {
      SamplerPreciseSH sampler(image, statistic);
      execute(reader, sampler, receiver);
    } break;
    }
  } else {
    switch (interp) {
    case interp_type::NEAREST: {
      SamplerNonPreciseScalar<Interp::Nearest<Image<value_type>>> sampler(image, statistic);
      execute(reader, sampler, receiver);
    } break;
    case interp_type::LINEAR: {
      SamplerNonPreciseScalar<Interp::Linear<Image<value_type>>> sampler(image, statistic);
      execute(reader, sampler, receiver);
    } break;
    case interp_type::PRECISE: {
      SamplerPreciseScalar sampler(image, statistic, tdi);
      execute(reader, sampler, receiver);
    } break;
    }
  }
}

void execute(DWI::Tractography::Reader<value_type> &reader,
             DWI::Tractography::Properties &properties,
             const size_t num_tracks,
             Image<value_type> &image,
             const interp_type interp,
             const bool sample_sh,
             const stat_tck statistic,
             Image<value_type> &tdi,
             const std::string &path) {
  const size_t num_metrics = image.ndim() == 4 && !sample_sh ? image.size(3) : 1;
  if (statistic == stat_tck::NONE) {
    if (num_metrics != 1)
      throw Exception("Cannot export per-vertex values for more than one contrast");
    if (interp == interp_type::PRECISE)
      throw Exception("Cannot combine per-vertex values with precise mapping mechanism");
    Receiver_PerVertex receiver(properties, num_tracks, path);
    execute(reader, image, interp, sample_sh, statistic, tdi, receiver);
  } else if (num_metrics == 1) {
    Receiver_OnePerStreamline receiver(num_tracks, path);
    execute(reader, image, interp, sample_sh, statistic, tdi, receiver);
  } else {
    Receiver_ManyPerStreamline receiver(num_tracks, num_metrics, path);
    execute(reader, image, interp, sample_sh, statistic, tdi, receiver);
  }
}

/*
template <class ReceiverType>
void execute (DWI::Tractography::Reader<value_type>& reader,
              std::shared_ptr<SamplerBase> sampler,
              ReceiverType& receiver)
{
  if (receiver.ordered())
    Thread::run_ordered_queue (reader,
                               Thread::batch (DWI::Tractography::Streamline<value_type>()),
                               Thread::multi (*sampler),
                               Thread::batch (typename ReceiverType::InputType()),
                               receiver);
  else
    Thread::run_queue (reader,
                       Thread::batch (DWI::Tractography::Streamline<value_type>()),
                       Thread::multi (*sampler),
                       Thread::batch (typename ReceiverType::InputType()),
                       receiver);
}

void execute (DWI::Tractography::Reader<value_type>& reader,
              DWI::Tractography::Properties& properties,
              const size_t num_tracks,
              std::shared_ptr<SamplerBase> sampler,
              const std::string& path)
{
  if (sampler->statistic() == stat_tck::NONE) {
    Receiver_PerVertex receiver (properties, num_tracks, path);
    execute(reader, sampler, receiver);
  } else if (sampler->num_contrasts() == 1) {
    Receiver_OnePerStreamline receiver (num_tracks, path);
    execute(reader, sampler, receiver);
  } else {
    Receiver_ManyPerStreamline receiver (num_tracks, sampler->num_contrasts(), path);
    execute(reader, sampler, receiver);
  }
}

std::shared_ptr<SamplerBase> make_sampler(Image<value_type>& image,
                                          const interp_type interp,
                                          const contrast_type contrast,
                                          const stat_tck statistic,
                                          Image<value_type>& tdi)
{
  switch (contrast) {
    case contrast_type::SCALAR:
      switch (interp) {
        case interp_type::NEAREST:
          return std::make_shared<SamplerNonPreciseScalar<Interp::Nearest<Image<value_type>>>> (image, statistic);
        case interp_type::LINEAR:
          return std::make_shared<SamplerNonPreciseScalar<Interp::Linear<Image<value_type>>>> (image, statistic);
        case interp_type::PRECISE:
          return std::make_shared<SamplerPreciseScalar> (image, statistic, tdi);
      }
    case contrast_type::SH:
      switch (interp) {
        case interp_type::NEAREST:
          return std::make_shared<SamplerNonPreciseSH<Interp::Nearest<Image<value_type>>>> (image, statistic);
        case interp_type::LINEAR:
          return std::make_shared<SamplerNonPreciseSH<Interp::Linear<Image<value_type>>>> (image, statistic);
        case interp_type::PRECISE:
          return std::make_shared<SamplerPreciseSH> (image, statistic);
      }
  }
}
*/

void run() {
  DWI::Tractography::Properties properties;
  DWI::Tractography::Reader<value_type> reader(argument[0], properties);

  auto H = Header::open(argument[1]);
  const bool plausibly_SH = H.ndim() > 3 && Math::SH::NforL(Math::SH::LforN(H.size(3))) == H.size(3);
  auto opt = get_options("sh");
  contrast_type contrast(contrast_type::SCALAR);
  if (opt.empty()) {
    if (plausibly_SH) {
      // clang-format off
      throw Exception(
          std::string("Input image coupld plausibly be interpreted as spherical harmonics; " //
                      "must specify the -sh option to inform command"                        //
                      " whether to interpret image in this way") +                           //
          ((H.ndim() == 4 && H.size(3) == 1) ?                                               //
               " this may be due to being a 4D image with 1 volume rather than a 3D image" : //
               ""));                                                                         //
      // clang-format on
    }
  } else if (plausibly_SH) {
    if (bool(opt[0][0])) {
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
    if (bool(opt[0][0])) {
      throw Exception("Cannot sample spherical harmonic function amplitudes, "
                      "as input image cannot be interpreted as such");
    } else {
      WARN("Specification of -sh false was unnecessary, "
           "as input image could not be interpreted as spherical harmonics functions");
    }
  }

  opt = get_options("stat_tck");
  const stat_tck statistic = opt.empty() ? stat_tck::NONE : stat_tck(int(opt[0][0]));
  if (H.ndim() == 4 && H.size(3) != 1 && statistic != stat_tck::NONE) {
    INFO("Input image is 4D; output will be 2D matrix");
  } else if (H.ndim() != 3) {
    throw Exception("Input image is of unsupported dimensionality");
  }

  const bool nointerp = get_options("nointerp").size();
  const bool precise = get_options("precise").size();
  if (nointerp && precise)
    throw Exception("Options -nointerp and -precise are mutually exclusive");
  const interp_type interp = nointerp ? interp_type::NEAREST : (precise ? interp_type::PRECISE : interp_type::LINEAR);
  const size_t num_tracks = properties.find("count") == properties.end() ? 0 : to<size_t>(properties["count"]);

  if (statistic == stat_tck::NONE && interp == interp_type::PRECISE)
    throw Exception("Precise streamline mapping may only be used with per-streamline statistics");

  Image<value_type> tdi;
  if (get_options("use_tdi_fraction").size()) {
    if (statistic == stat_tck::NONE)
      throw Exception("Cannot use -use_tdi_fraction option unless a per-streamline statistic is used");
    if (contrast == contrast_type::SH)
      throw Exception("Cannot use -use_tdi_fraction option in conjunction with SH function sampling");
    DWI::Tractography::Reader<value_type> tdi_reader(argument[0], properties);
    DWI::Tractography::Mapping::TrackMapperBase mapper(H);
    mapper.set_use_precise_mapping(interp == interp_type::PRECISE);
    tdi = Image<value_type>::scratch(H, "TDI scratch image");
    TDI tdi_fill(tdi, num_tracks);
    Thread::run_queue(tdi_reader,
                      Thread::batch(DWI::Tractography::Streamline<value_type>()),
                      Thread::multi(mapper),
                      Thread::batch(DWI::Tractography::Mapping::SetVoxel()),
                      tdi_fill);
  }

  auto image = H.get_image<value_type>();

  // std::shared_ptr<SamplerBase> sampler = make_sampler(image, interp, contrast, statistic, tdi);
  // execute(reader, properties, num_tracks, sampler, argument[2]);
  execute(reader, properties, num_tracks, image, interp, contrast, statistic, tdi, argument[2]);

  /*
    if (statistic == stat_tck::NONE) {
      switch (interp) {
        case interp_type::NEAREST:
          execute_nostat<Interp::Nearest<Image<value_type>>> (reader, properties, num_tracks, image, sample_sh,
    argument[2]); break; case interp_type::LINEAR: execute_nostat<Interp::Linear<Image<value_type>>> (reader,
    properties, num_tracks, image, sample_sh, argument[2]); break; case interp_type::PRECISE: throw Exception
    ("Precise streamline mapping may only be used with per-streamline statistics");
      }
    } else {
      switch (interp) {
        case interp_type::NEAREST:
          execute<SamplerNonPrecise<Interp::Nearest<Image<value_type>>>> (reader, num_tracks, image, sample_sh,
    statistic, tdi, argument[2]); break; case interp_type::LINEAR:
          execute<SamplerNonPrecise<Interp::Linear<Image<value_type>>>> (reader, num_tracks, image, sample_sh,
    statistic, tdi, argument[2]); break; case interp_type::PRECISE: execute<SamplerPrecise> (reader, num_tracks,
    image, sample_sh, statistic, tdi, argument[2]); break;
      }
    }
  */
}
