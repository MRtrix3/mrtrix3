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
#include "image.h"

#include <Eigen/Dense>
#include <Eigen/Eigenvalues>

using namespace MR;
using namespace App;

const std::vector<std::string> dtypes = {"float32", "float64"};
const std::vector<std::string> estimators = {"exp1", "exp2"};
enum class estimator_type { EXP1, EXP2 };

const std::vector<std::string> shapes = {"cuboid", "sphere"};
enum class shape_type { CUBOID, SPHERE };
constexpr default_type sphere_multiplier_default = 1.0 / 0.85;

const std::vector<std::string> filters = {"truncate", "frobenius"};
enum class filter_type { TRUNCATE, FROBENIUS };

// clang-format off
void usage() {

  SYNOPSIS = "dMRI noise level estimation and denoising using Marchenko-Pastur PCA";

  DESCRIPTION
  + "DWI data denoising and noise map estimation"
    " by exploiting data redundancy in the PCA domain"
    " using the prior knowledge that the eigenspectrum of random covariance matrices"
    " is described by the universal Marchenko-Pastur (MP) distribution."
    " Fitting the MP distribution to the spectrum of patch-wise signal matrices"
    " hence provides an estimator of the noise level 'sigma',"
    " as was first shown in Veraart et al. (2016)"
    " and later improved in Cordero-Grande et al. (2019)."
    " This noise level estimate then determines the optimal cut-off for PCA denoising."

  + "Important note:"
    " image denoising must be performed as the first step of the image processing pipeline."
    " The routine will fail if interpolation or smoothing has been applied to the data prior to denoising."

  + "Note that this function does not correct for non-Gaussian noise biases"
    " present in magnitude-reconstructed MRI images."
    " If available, including the MRI phase data can reduce such non-Gaussian biases,"
    " and the command now supports complex input data."

  + "The sliding spatial window behaves differently at the edges of the image FoV "
    "depending on the shape / size selected for that window. "
    "The default behaviour is to use a spherical kernel centred at the voxel of interest, "
    "whose size is some multiple of the number of input volumes; "
    "where some such voxels lie outside of the image FoV, "
    "the radius of the kernel will be increased until the requisite number of voxels are used. "
    "For a spherical kernel of a fixed radius, "
    "no such expansion will occur, "
    "and so for voxels near the image edge a reduced number of voxels will be present in the kernel. "
    "For a cuboid kernel, "
    "the centre of the kernel will be offset from the voxel being processed "
    "such that the entire volume of the kernel resides within the image FoV."

  + "The size of the default spherical kernel is set to select a number of voxels that is "
    "1.1 times the number of volumes in the input series. "
    "If a cuboid kernel is requested, "
    "but the -extent option is not specified, "
    "the command will select the smallest isotropic patch size "
    "that exceeds the number of DW images in the input data; "
    "e.g., 5x5x5 for data with <= 125 DWI volumes, "
    "7x7x7 for data with <= 343 DWI volumes, etc."

  + "By default, optimal value shrinkage based on minimisation of the Frobenius norm "
    "will be used to attenuate eigenvectors based on the estimated noise level. "
    "Hard truncation of sub-threshold components"
    "---which was the behaviour of the dwidenoise command in version 3.0.x---"
    "can be activated using -filter truncate.";

  AUTHOR = "Daan Christiaens (daan.christiaens@kcl.ac.uk)"
           " and Jelle Veraart (jelle.veraart@nyumc.org)"
           " and J-Donald Tournier (jdtournier@gmail.com)"
           " and Robert E. Smith (robert.smith@florey.edu.au)";

  REFERENCES
  + "Veraart, J.; Novikov, D.S.; Christiaens, D.; Ades-aron, B.; Sijbers, J. & Fieremans, E. " // Internal
    "Denoising of diffusion MRI using random matrix theory. "
    "NeuroImage, 2016, 142, 394-406, doi: 10.1016/j.neuroimage.2016.08.016"

  + "Veraart, J.; Fieremans, E. & Novikov, D.S. " // Internal
    "Diffusion MRI noise mapping using random matrix theory. "
    "Magn. Res. Med., 2016, 76(5), 1582-1593, doi: 10.1002/mrm.26059"

  + "Cordero-Grande, L.; Christiaens, D.; Hutter, J.; Price, A.N.; Hajnal, J.V. " // Internal
    "Complex diffusion-weighted image estimation via matrix recovery under general noise models. "
    "NeuroImage, 2019, 200, 391-404, doi: 10.1016/j.neuroimage.2019.06.039";

  ARGUMENTS
  + Argument("dwi", "the input diffusion-weighted image.").type_image_in()
  + Argument("out", "the output denoised DWI image.").type_image_out();

  OPTIONS
  + OptionGroup("Options for modifying the application of PCA denoising")
  + Option("mask",
           "Only process voxels within the specified binary brain mask image.")
    + Argument("image").type_image_in()
  + Option("datatype",
           "Datatype for the eigenvalue decomposition"
           " (single or double precision). "
           "For complex input data,"
           " this will select complex float32 or complex float64 datatypes.")
    + Argument("float32/float64").type_choice(dtypes)
  + Option("estimator",
           "Select the noise level estimator"
           " (default = Exp2),"
           " either: \n"
           "* Exp1: the original estimator used in Veraart et al. (2016), or \n"
           "* Exp2: the improved estimator introduced in Cordero-Grande et al. (2019).")
    + Argument("Exp1/Exp2").type_choice(estimators)
  + Option("filter",
           "Modulate how components are filtered based on their eigenvalues; "
           "options are: " + join(filters, ",") + "; default: frobenius")
    + Argument("choice").type_choice(filters)

  + OptionGroup("Options for exporting additional data regarding PCA behaviour")
  + Option("noise",
           "The output noise map,"
           " i.e., the estimated noise level 'sigma' in the data. "
           "Note that on complex input data,"
           " this will be the total noise level across real and imaginary channels,"
           " so a scale factor sqrt(2) applies.")
    + Argument("image").type_image_out()
  + Option("rank",
           "The selected signal rank of the output denoised image.")
    + Argument("image").type_image_out()
  + Option("sumweights",
           "the sum of eigenvector weights contributed to the output image")
    + Argument("image").type_image_out()
  + Option("max_dist",
           "The maximum distance between a voxel and another voxel that was included in the local denoising patch")
    + Argument("image").type_image_out()
  + Option("voxels",
           "The number of voxels that contributed to the PCA for processing of each voxel")
    + Argument("image").type_image_out()

  + OptionGroup("Options for controlling the sliding spatial window")
  + Option("shape",
           "Set the shape of the sliding spatial window. "
           "Options are: " + join(shapes, ",") + "; default: sphere")
    + Argument("choice").type_choice(shapes)
  + Option("radius_mm",
           "Set an absolute spherical kernel radius in mm")
    + Argument("value").type_float(0.0)
  + Option("radius_ratio",
           "Set the spherical kernel size as a ratio of number of voxels to number of input volumes "
           "(default: 1.0/0.85 ~= 1.18)")
    + Argument("value").type_float(0.0)
  // TODO Command-line option that allows user to specify minimum absolute number of voxels in kernel
  + Option("extent",
           "Set the patch size of the cuboid kernel; "
           "can be either a single odd integer or a comma-separated triplet of odd integers")
    + Argument("window").type_sequence_int();

  COPYRIGHT =
      "Copyright (c) 2016 New York University, University of Antwerp, and the MRtrix3 contributors \n \n"
      "Permission is hereby granted, free of charge, to any non-commercial entity ('Recipient') obtaining a copy of "
      "this software and "
      "associated documentation files (the 'Software'), to the Software solely for non-commercial research, including "
      "the rights to "
      "use, copy and modify the Software, subject to the following conditions: \n \n"
      "\t 1. The above copyright notice and this permission notice shall be included by Recipient in all copies or "
      "substantial portions of "
      "the Software. \n \n"
      "\t 2. THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT "
      "LIMITED TO THE WARRANTIES"
      "OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR "
      "COPYRIGHT HOLDERS BE"
      "LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING "
      "FROM, OUT OF OR"
      "IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. \n \n"
      "\t 3. In no event shall NYU be liable for direct, indirect, special, incidental or consequential damages in "
      "connection with the Software. "
      "Recipient will defend, indemnify and hold NYU harmless from any claims or liability resulting from the use of "
      "the Software by recipient. \n \n"
      "\t 4. Neither anything contained herein nor the delivery of the Software to recipient shall be deemed to grant "
      "the Recipient any right or "
      "licenses under any patents or patent application owned by NYU. \n \n"
      "\t 5. The Software may only be used for non-commercial research and may not be used for clinical care. \n \n"
      "\t 6. Any publication by Recipient of research involving the Software shall cite the references listed below.";
}
// clang-format on

using real_type = float;
using voxel_type = Eigen::Array<int, 3, 1>;
using vector_type = Eigen::VectorXd;

class KernelVoxel {
public:
  KernelVoxel(const voxel_type &offset, const default_type sq_distance) : offset(offset), sq_distance(sq_distance) {}
  KernelVoxel(const KernelVoxel &) = default;
  KernelVoxel(KernelVoxel &&) = default;
  ~KernelVoxel() {}
  KernelVoxel &operator=(const KernelVoxel &that) {
    offset = that.offset;
    sq_distance = that.sq_distance;
    return *this;
  }
  KernelVoxel &operator=(KernelVoxel &&that) noexcept {
    offset = that.offset;
    sq_distance = that.sq_distance;
    return *this;
  }
  bool operator<(const KernelVoxel &that) const { return sq_distance < that.sq_distance; }
  default_type distance() const { return std::sqrt(sq_distance); }
  voxel_type offset;
  default_type sq_distance;
};

// Class to encode return information from kernel
class KernelData {
public:
  KernelData() : centre_index(-1), max_distance(-std::numeric_limits<default_type>::infinity()) {}
  KernelData(const ssize_t i) : centre_index(i), max_distance(-std::numeric_limits<default_type>::infinity()) {}
  std::vector<KernelVoxel> voxels;
  ssize_t centre_index;
  default_type max_distance;
};

class KernelBase {
public:
  KernelBase(const Header &H) : H(H) {}
  KernelBase(const KernelBase &) = default;
  virtual ~KernelBase() = default;
  // This is just for pre-allocating matrices
  virtual ssize_t estimated_size() const = 0;
  // This is the interface that kernels must provide
  virtual KernelData operator()(const voxel_type &) const = 0;

protected:
  const Header H;
};

class KernelCube : public KernelBase {
public:
  KernelCube(const Header &header, const std::vector<uint32_t> &extent)
      : KernelBase(header),
        half_extent({int(extent[0] / 2), int(extent[1] / 2), int(extent[2] / 2)}),
        size(ssize_t(extent[0]) * ssize_t(extent[1]) * ssize_t(extent[2])),
        centre_index(size / 2) {
    for (auto e : extent) {
      if (!(e % 2))
        throw Exception("Size of cubic kernel must be an odd integer");
    }
  }
  KernelCube(const KernelCube &) = default;
  ~KernelCube() final = default;
  KernelData operator()(const voxel_type &pos) const override {
    KernelData result(centre_index);
    voxel_type voxel;
    voxel_type offset;
    for (offset[2] = -half_extent[2]; offset[2] <= half_extent[2]; ++offset[2]) {
      voxel[2] = wrapindex(pos[2], offset[2], half_extent[2], H.size(2));
      for (offset[1] = -half_extent[1]; offset[1] <= half_extent[1]; ++offset[1]) {
        voxel[1] = wrapindex(pos[1], offset[1], half_extent[1], H.size(1));
        for (offset[0] = -half_extent[0]; offset[0] <= half_extent[0]; ++offset[0]) {
          voxel[0] = wrapindex(pos[0], offset[0], half_extent[0], H.size(0));
          const default_type sq_distance = Math::pow2((pos[0] - voxel[0]) * H.spacing(0)) +
                                           Math::pow2((pos[1] - voxel[1]) * H.spacing(1)) +
                                           Math::pow2((pos[2] - voxel[2]) * H.spacing(2));
          result.voxels.push_back(KernelVoxel(voxel, sq_distance));
          result.max_distance = std::max(result.max_distance, sq_distance);
        }
      }
    }
    result.max_distance = std::sqrt(result.max_distance);
    return result;
  }
  ssize_t estimated_size() const override { return size; }

private:
  const std::vector<int> dimensions;
  const std::vector<int> half_extent;
  const ssize_t size;
  const ssize_t centre_index;

  // patch handling at image edges
  inline size_t wrapindex(int p, int r, int e, int max) const {
    int rr = p + r;
    if (rr < 0)
      rr = e - r;
    if (rr >= max)
      rr = (max - 1) - e - r;
    return rr;
  }
};

class KernelSphereBase : public KernelBase {
public:
  KernelSphereBase(const Header &voxel_grid, const default_type max_radius)
      : KernelBase(voxel_grid), shared(new Shared(voxel_grid, max_radius)) {}
  KernelSphereBase(const KernelSphereBase &) = default;
  virtual ~KernelSphereBase() override {}

protected:
  class Shared {
  public:
    using TableType = std::vector<KernelVoxel>;
    Shared(const Header &voxel_grid, const default_type max_radius) {
      const default_type max_radius_sq = Math::pow2(max_radius);
      const voxel_type half_extents({int(std::ceil(max_radius / voxel_grid.spacing(0))),   //
                                     int(std::ceil(max_radius / voxel_grid.spacing(1))),   //
                                     int(std::ceil(max_radius / voxel_grid.spacing(2)))}); //
      // Build the searchlight
      data.reserve(size_t(2 * half_extents[0] + 1) * size_t(2 * half_extents[1] + 1) * size_t(2 * half_extents[2] + 1));
      voxel_type offset({-1, -1, -1});
      for (offset[2] = -half_extents[2]; offset[2] <= half_extents[2]; ++offset[2]) {
        for (offset[1] = -half_extents[1]; offset[1] <= half_extents[1]; ++offset[1]) {
          for (offset[0] = -half_extents[0]; offset[0] <= half_extents[0]; ++offset[0]) {
            const default_type squared_distance = Math::pow2(offset[0] * voxel_grid.spacing(0))    //
                                                  + Math::pow2(offset[1] * voxel_grid.spacing(1))  //
                                                  + Math::pow2(offset[2] * voxel_grid.spacing(2)); //
            if (squared_distance <= max_radius_sq)
              data.emplace_back(KernelVoxel(offset, squared_distance));
          }
        }
      }
      std::sort(data.begin(), data.end());
    }
    TableType::const_iterator begin() const { return data.begin(); }
    TableType::const_iterator end() const { return data.end(); }

  private:
    TableType data;
  };
  std::shared_ptr<Shared> shared;
};

class KernelSphereRatio : public KernelSphereBase {
public:
  KernelSphereRatio(const Header &voxel_grid, const default_type min_ratio)
      : KernelSphereBase(voxel_grid, compute_max_radius(voxel_grid, min_ratio)),
        min_size(std::ceil(voxel_grid.size(3) * min_ratio)) {}
  KernelSphereRatio(const KernelSphereRatio &) = default;
  ~KernelSphereRatio() final = default;
  KernelData operator()(const voxel_type &pos) const override {
    KernelData result(0);
    auto table_it = shared->begin();
    while (table_it != shared->end()) {
      // If there's a tie in distances, want to include all such offsets in the kernel,
      //   even if the size of the utilised kernel extends beyond the minimum size
      if (result.voxels.size() >= min_size && table_it->sq_distance != result.max_distance)
        break;
      const voxel_type voxel({pos[0] + table_it->offset[0],   //
                              pos[1] + table_it->offset[1],   //
                              pos[2] + table_it->offset[2]}); //
      if (!is_out_of_bounds(H, voxel, 0, 3)) {
        result.voxels.push_back(KernelVoxel(voxel, table_it->sq_distance));
        result.max_distance = table_it->sq_distance;
      }
      ++table_it;
    }
    if (table_it == shared->end()) {
      throw Exception(                                                                   //
          std::string("Inadequate spherical kernel initialisation ")                     //
          + "(lookup table " + str(std::distance(shared->begin(), shared->end())) + "; " //
          + "min size " + str(min_size) + "; "                                           //
          + "read size " + str(result.voxels.size()) + ")");                             //
    }
    result.max_distance = std::sqrt(result.max_distance);
    return result;
  }
  ssize_t estimated_size() const override { return min_size; }

private:
  ssize_t min_size;
  // Determine an appropriate bounding box from which to generate the search table
  // Find the radius for which 7/8 of the sphere will contain the minimum number of voxels, then round up
  // This is only for setting the maximal radius for generation of the lookup table
  default_type compute_max_radius(const Header &voxel_grid, const default_type min_ratio) const {
    const size_t num_volumes = voxel_grid.size(3);
    const default_type voxel_volume = voxel_grid.spacing(0) * voxel_grid.spacing(1) * voxel_grid.spacing(2);
    const default_type sphere_volume = 8.0 * num_volumes * min_ratio * voxel_volume;
    const default_type approx_radius = std::sqrt(sphere_volume * 0.75 / Math::pi);
    const voxel_type half_extents({int(std::ceil(approx_radius / voxel_grid.spacing(0))),   //
                                   int(std::ceil(approx_radius / voxel_grid.spacing(1))),   //
                                   int(std::ceil(approx_radius / voxel_grid.spacing(2)))}); //
    return std::max({half_extents[0] * voxel_grid.spacing(0),
                     half_extents[1] * voxel_grid.spacing(1),
                     half_extents[2] * voxel_grid.spacing(2)});
  }
};

class KernelSphereFixedRadius : public KernelSphereBase {
public:
  KernelSphereFixedRadius(const Header &voxel_grid, const default_type radius)
      : KernelSphereBase(voxel_grid, radius),                         //
        maximum_size(std::distance(shared->begin(), shared->end())) { //
    INFO("Maximum number of voxels in " + str(radius) + "mm fixed-radius kernel is " + str(maximum_size));
  }
  KernelSphereFixedRadius(const KernelSphereFixedRadius &) = default;
  ~KernelSphereFixedRadius() final = default;
  KernelData operator()(const voxel_type &pos) const {
    KernelData result(0);
    result.voxels.reserve(maximum_size);
    for (auto map_it = shared->begin(); map_it != shared->end(); ++map_it) {
      const voxel_type voxel({pos[0] + map_it->offset[0],   //
                              pos[1] + map_it->offset[1],   //
                              pos[2] + map_it->offset[2]}); //
      if (!is_out_of_bounds(H, voxel, 0, 3)) {
        result.voxels.push_back(KernelVoxel(voxel, map_it->sq_distance));
        result.max_distance = map_it->sq_distance;
      }
    }
    result.max_distance = std::sqrt(result.max_distance);
    return result;
  }
  ssize_t estimated_size() const override { return maximum_size; }

private:
  const ssize_t maximum_size;
};

template <typename F> class DenoisingFunctor {

public:
  using MatrixType = Eigen::Matrix<F, Eigen::Dynamic, Eigen::Dynamic>;

  DenoisingFunctor(int ndwi,
                   std::shared_ptr<KernelBase> kernel,
                   filter_type filter,
                   Image<bool> &mask,
                   Image<real_type> &noise,
                   Image<uint16_t> &rank,
                   Image<float> &sum_weights,
                   Image<float> &max_dist,
                   Image<uint16_t> &voxels,
                   estimator_type estimator)
      : kernel(kernel),
        filter(filter),
        m(ndwi),
        estimator(estimator),
        X(ndwi, kernel->estimated_size()),
        XtX(std::min(m, kernel->estimated_size()), std::min(m, kernel->estimated_size())),
        eig(std::min(m, kernel->estimated_size())),
        s(std::min(m, kernel->estimated_size())),
        clam(std::min(m, kernel->estimated_size())),
        w(std::min(m, kernel->estimated_size())),
        mask(mask),
        noise(noise),
        rankmap(rank),
        sumweightsmap(sum_weights),
        maxdistmap(max_dist),
        voxelsmap(voxels) {}

  template <typename ImageType> void operator()(ImageType &dwi, ImageType &out) {
    // Process voxels in mask only
    if (mask.valid()) {
      assign_pos_of(dwi, 0, 3).to(mask);
      if (!mask.value())
        return;
    }

    // Load list of voxels from which to load data
    const KernelData neighbourhood = (*kernel)({int(dwi.index(0)), int(dwi.index(1)), int(dwi.index(2))});
    const ssize_t n = neighbourhood.voxels.size();
    const ssize_t r = std::min(m, n);
    const ssize_t q = std::max(m, n);

    // Expand local storage if necessary
    if (n > X.cols()) {
      DEBUG("Expanding data matrix storage from " + str(m) + "x" + str(X.cols()) + " to " + str(m) + "x" + str(n));
      X.resize(m, n);
    }
    if (r > XtX.cols()) {
      DEBUG("Expanding decomposition matrix storage from " + str(X.rows()) + " to " + str(r));
      XtX.resize(r, r);
      s.resize(r);
      clam.resize(r);
      w.resize(r);
    }

    // Fill matrices with NaN when in debug mode;
    //   make sure results from one voxel are not creeping into another
    //   due to use of block oberations to prevent memory re-allocation
    //   in the presence of variation in kernel sizes
#ifndef NDEBUG
    X.fill(std::numeric_limits<F>::signaling_NaN());
    XtX.fill(std::numeric_limits<F>::signaling_NaN());
    s.fill(std::numeric_limits<default_type>::signaling_NaN());
    clam.fill(std::numeric_limits<default_type>::signaling_NaN());
    w.fill(std::numeric_limits<default_type>::signaling_NaN());
#endif

    load_data(dwi, neighbourhood.voxels);

    // Compute Eigendecomposition:
    if (m <= n)
      XtX.topLeftCorner(r, r).template triangularView<Eigen::Lower>() = X.leftCols(n) * X.leftCols(n).adjoint();
    else
      XtX.topLeftCorner(r, r).template triangularView<Eigen::Lower>() = X.leftCols(n).adjoint() * X.leftCols(n);
    eig.compute(XtX.topLeftCorner(r, r));
    // eigenvalues sorted in increasing order:
    s.head(r) = eig.eigenvalues().template cast<double>();

    // Marchenko-Pastur optimal threshold
    const double lam_r = std::max(s[0], 0.0) / q;
    double sigma2 = 0.0;
    ssize_t cutoff_p = 0;
    for (ssize_t p = 0; p < r; ++p) // p+1 is the number of noise components
    {                               // (as opposed to the paper where p is defined as the number of signal components)
      const double lam = std::max(s[p], 0.0) / q;
      clam[p] = (p == 0 ? 0.0 : clam[p - 1]) + lam;
      double denominator = std::numeric_limits<double>::signaling_NaN();
      switch (estimator) {
      case estimator_type::EXP1:
        denominator = q;
        break;
      case estimator_type::EXP2:
        denominator = q - (r - p - 1);
        break;
      default:
        assert(false);
      }
      const double gam = double(p + 1) / denominator;
      const double sigsq1 = clam[p] / double(p + 1);
      const double sigsq2 = (lam - lam_r) / (4.0 * std::sqrt(gam));
      // sigsq2 > sigsq1 if signal else noise
      if (sigsq2 < sigsq1) {
        sigma2 = sigsq1;
        cutoff_p = p + 1;
      }
    }

    // Generate weights vector
    double sum_weights = 0.0;
    switch (filter) {
    case filter_type::TRUNCATE:
      w.head(cutoff_p).setZero();
      w.segment(cutoff_p, r - cutoff_p).setOnes();
      sum_weights = r - cutoff_p;
      break;
    case filter_type::FROBENIUS: {
      const double beta = r / q;
      const double threshold = 1.0 + std::sqrt(beta);
      for (ssize_t i = 0; i != r; ++i) {
        const double y = clam[i] / (sigma2 * (i + 1));
        const double nu = y > threshold ? std::sqrt(Math::pow2(Math::pow2(y) - beta - 1.0) - (4.0 * beta)) / y : 0.0;
        w[i] = nu / y;
        sum_weights += w[i];
      }
    } break;
    default:
      assert(false);
    }

    // recombine data using only eigenvectors above threshold:
    if (m <= n)
      X.col(neighbourhood.centre_index) =
          eig.eigenvectors() *
          (w.head(r).cast<F>().asDiagonal() * (eig.eigenvectors().adjoint() * X.col(neighbourhood.centre_index)));
    else
      X.col(neighbourhood.centre_index) =
          X.leftCols(n) * (eig.eigenvectors() * (w.head(r).cast<F>().asDiagonal() *
                                                 eig.eigenvectors().adjoint().col(neighbourhood.centre_index)));

    // Store output
    assign_pos_of(dwi).to(out);
    out.row(3) = X.col(neighbourhood.centre_index);

    // Store additional output maps if requested
    if (noise.valid()) {
      assign_pos_of(dwi, 0, 3).to(noise);
      noise.value() = real_type(std::sqrt(sigma2));
    }
    if (rankmap.valid()) {
      assign_pos_of(dwi, 0, 3).to(rankmap);
      rankmap.value() = uint16_t(r - cutoff_p);
    }
    if (sumweightsmap.valid()) {
      assign_pos_of(dwi, 0, 3).to(sumweightsmap);
      sumweightsmap.value() = sum_weights;
    }
    if (maxdistmap.valid()) {
      assign_pos_of(dwi, 0, 3).to(maxdistmap);
      maxdistmap.value() = float(neighbourhood.max_distance);
    }
    if (voxelsmap.valid()) {
      assign_pos_of(dwi, 0, 3).to(voxelsmap);
      voxelsmap.value() = n;
    }
  }

private:
  // Denoising configuration
  std::shared_ptr<KernelBase> kernel;
  filter_type filter;
  const ssize_t m;
  const estimator_type estimator;

  // Reusable memory
  MatrixType X;
  MatrixType XtX;
  Eigen::SelfAdjointEigenSolver<MatrixType> eig;
  vector_type s;
  vector_type clam;
  vector_type w;

  Image<bool> mask;

  // Export images
  // TODO Group these into a class?
  Image<real_type> noise;
  Image<uint16_t> rankmap;
  Image<float> sumweightsmap;
  Image<float> maxdistmap;
  Image<uint16_t> voxelsmap;

  template <typename ImageType> void load_data(ImageType &image, const std::vector<KernelVoxel> &voxels) {
    const voxel_type pos({int(image.index(0)), int(image.index(1)), int(image.index(2))});
    for (ssize_t i = 0; i != voxels.size(); ++i) {
      assign_pos_of(voxels[i].offset, 0, 3).to(image);
      X.col(i) = image.row(3);
    }
    assign_pos_of(pos, 0, 3).to(image);
  }
};

template <typename T>
void run(Header &data,
         Image<bool> &mask,
         Image<real_type> &noise,
         Image<uint16_t> &rank,
         Image<float> &sum_weights,
         Image<float> &max_dist,
         Image<uint16_t> &voxels,
         const std::string &output_name,
         std::shared_ptr<KernelBase> kernel,
         filter_type filter,
         estimator_type estimator) {
  auto input = data.get_image<T>().with_direct_io(3);
  // create output
  Header header(data);
  header.datatype() = DataType::from<T>();
  auto output = Image<T>::create(output_name, header);
  // run
  DenoisingFunctor<T> func(data.size(3), kernel, filter, mask, noise, rank, sum_weights, max_dist, voxels, estimator);
  ThreadedLoop("running MP-PCA denoising", data, 0, 3).run(func, input, output);
}

void run() {
  auto dwi = Header::open(argument[0]);

  if (dwi.ndim() != 4 || dwi.size(3) <= 1)
    throw Exception("input image must be 4-dimensional");

  Image<bool> mask;
  auto opt = get_options("mask");
  if (!opt.empty()) {
    mask = Image<bool>::open(opt[0][0]);
    check_dimensions(mask, dwi, 0, 3);
  }

  estimator_type estimator = estimator_type::EXP2; // default: Exp2 (unbiased estimator)
  opt = get_options("estimator");
  if (!opt.empty())
    estimator = estimator_type(int(opt[0][0]));

  filter_type filter = filter_type::FROBENIUS;
  opt = get_options("filter");
  if (!opt.empty())
    filter = filter_type(int(opt[0][0]));

  Image<real_type> noise;
  opt = get_options("noise");
  if (!opt.empty()) {
    Header header(dwi);
    header.ndim() = 3;
    header.datatype() = DataType::Float32;
    noise = Image<real_type>::create(opt[0][0], header);
  }

  Image<uint16_t> rank;
  opt = get_options("rank");
  if (!opt.empty()) {
    Header header(dwi);
    header.ndim() = 3;
    header.datatype() = DataType::UInt16;
    header.reset_intensity_scaling();
    rank = Image<uint16_t>::create(opt[0][0], header);
  }

  Image<float> sum_weights;
  opt = get_options("sumweights");
  if (!opt.empty()) {
    Header header(dwi);
    header.ndim() = 3;
    header.datatype() = DataType::Float32;
    header.datatype().set_byte_order_native();
    header.reset_intensity_scaling();
    sum_weights = Image<float>::create(opt[0][0], header);
    if (filter == filter_type::TRUNCATE) {
      WARN("Note that with a truncation filter, "
           "output image from -sumweights option will be equivalent to rank");
    }
  }

  Image<float> max_dist;
  opt = get_options("max_dist");
  if (!opt.empty()) {
    Header header(dwi);
    header.ndim() = 3;
    header.datatype() = DataType::Float32;
    header.datatype().set_byte_order_native();
    header.reset_intensity_scaling();
    max_dist = Image<float>::create(opt[0][0], header);
  }

  Image<uint16_t> voxels;
  opt = get_options("voxels");
  if (!opt.empty()) {
    Header header(dwi);
    header.ndim() = 3;
    header.datatype() = DataType::UInt16;
    header.reset_intensity_scaling();
    voxels = Image<uint16_t>::create(opt[0][0], header);
  }

  opt = get_options("shape");
  const shape_type shape = opt.empty() ? shape_type::SPHERE : shape_type((int)(opt[0][0]));
  std::shared_ptr<KernelBase> kernel;

  switch (shape) {
  case shape_type::SPHERE: {
    // TODO Could infer that user wants a cuboid kernel if -extent is used, even if -shape is not
    if (!get_options("extent").empty())
      throw Exception("-extent option does not apply to spherical kernel");
    opt = get_options("radius_mm");
    if (opt.empty())
      kernel = std::make_shared<KernelSphereRatio>(dwi, get_option_value("radius_ratio", sphere_multiplier_default));
    else
      kernel = std::make_shared<KernelSphereFixedRadius>(dwi, opt[0][0]);
  } break;
  case shape_type::CUBOID: {
    if (!get_options("radius_mm").empty() || !get_options("radius_ratio").empty())
      throw Exception("-radius_* options are inapplicable if cuboid kernel shape is selected");
    opt = get_options("extent");
    std::vector<uint32_t> extent;
    if (!opt.empty()) {
      extent = parse_ints<uint32_t>(opt[0][0]);
      if (extent.size() == 1)
        extent = {extent[0], extent[0], extent[0]};
      if (extent.size() != 3)
        throw Exception("-extent must be either a scalar or a list of length 3");
      for (int i = 0; i < 3; i++) {
        if ((extent[i] & 1) == 0)
          throw Exception("-extent must be a (list of) odd numbers");
        if (extent[i] > dwi.size(i))
          throw Exception("-extent must not exceed the image dimensions");
      }
    } else {
      uint32_t e = 1;
      while (Math::pow3(e) < dwi.size(3))
        e += 2;
      extent = {std::min(e, uint32_t(dwi.size(0))),  //
                std::min(e, uint32_t(dwi.size(1))),  //
                std::min(e, uint32_t(dwi.size(2)))}; //
    }
    INFO("selected patch size: " + str(extent[0]) + " x " + str(extent[1]) + " x " + str(extent[2]) + ".");

    if (std::min<uint32_t>(dwi.size(3), extent[0] * extent[1] * extent[2]) < 15) {
      WARN("The number of volumes or the patch size is small. "
           "This may lead to discretisation effects in the noise level "
           "and cause inconsistent denoising between adjacent voxels.");
    }

    kernel = std::make_shared<KernelCube>(dwi, extent);
  } break;
  default:
    assert(false);
  }
  assert(kernel);

  int prec = get_option_value("datatype", 0); // default: single precision
  if (dwi.datatype().is_complex())
    prec += 2; // support complex input data
  switch (prec) {
  case 0:
    INFO("select real float32 for processing");
    run<float>(dwi, mask, noise, rank, sum_weights, max_dist, voxels, argument[1], kernel, filter, estimator);
    break;
  case 1:
    INFO("select real float64 for processing");
    run<double>(dwi, mask, noise, rank, sum_weights, max_dist, voxels, argument[1], kernel, filter, estimator);
    break;
  case 2:
    INFO("select complex float32 for processing");
    run<cfloat>(dwi, mask, noise, rank, sum_weights, max_dist, voxels, argument[1], kernel, filter, estimator);
    break;
  case 3:
    INFO("select complex float64 for processing");
    run<cdouble>(dwi, mask, noise, rank, sum_weights, max_dist, voxels, argument[1], kernel, filter, estimator);
    break;
  }
}
