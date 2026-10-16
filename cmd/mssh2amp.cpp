/* Copyright (c) 2017-2019 Daan Christiaens
 *
 * MRtrix and this add-on module are distributed in the hope
 * that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 */

#include <sstream>

#include "command.h"
#include "dwi/gradient.h"
#include "file/matrix.h"
#include "image.h"
#include "math/SH.h"

using namespace MR;
using namespace App;

// clang-format off
void usage ()
{
  AUTHOR = "Daan Christiaens (daan.christiaens@kcl.ac.uk) and "
           "David Raffelt (david.raffelt@florey.edu.au)";

  SYNOPSIS = "Evaluate the amplitude of a 5-D image of multi-shell "
             "spherical harmonic functions along specified directions.";

  ARGUMENTS
    + Argument ("input",
                "the input image consisting of spherical harmonic (SH) "
                "coefficients.").type_image_in ()
    + Argument ("gradient",
                "the gradient encoding along which the SH functions will "
                "be sampled (directions + shells)").type_file_in ()
    + Argument ("output",
                "the output image consisting of the amplitude of the SH "
                "functions along the specified directions.").type_image_out ();

  OPTIONS
    + Option ("transform",
              "rigid transformation, applied to the gradient table.")
      + Argument("T").type_file_in()

    + Option ("nonnegative",
              "cap all negative amplitudes to zero")

    + Stride::Options
    + DataType::options();
}
// clang-format on

using value_type = float;

class MSSH2Amp {
public:
  template <class MatrixType>
  MSSH2Amp(const MatrixType &dirs, const size_t lmax, const std::vector<size_t> &idx, bool nonneg)
      : SHT(Math::SH::init_transform_cart(dirs.template cast<value_type>(), lmax)),
        bidx(idx),
        nonnegative(nonneg),
        sh(SHT.cols()),
        amp(SHT.rows()) {}

  void operator()(Image<value_type> &in, Image<value_type> &out) {
    sh = in.row(4);
    amp = SHT * sh;
    if (nonnegative)
      amp = amp.cwiseMax(value_type(0.0));
    for (size_t j = 0; j < amp.size(); j++) {
      out.index(3) = bidx[j];
      out.value() = amp[j];
    }
  }

private:
  const Eigen::Matrix<value_type, Eigen::Dynamic, Eigen::Dynamic> SHT;
  const std::vector<size_t> &bidx;
  const bool nonnegative;
  Eigen::Matrix<value_type, Eigen::Dynamic, 1> sh, amp;
};

template <class VectorType> inline std::vector<size_t> get_indices(const VectorType &blist, const value_type bval) {
  std::vector<size_t> indices;
  for (size_t j = 0; j < blist.size(); j++)
    if ((blist[j] > bval - DWI_SHELLS_EPSILON) && (blist[j] < bval + DWI_SHELLS_EPSILON))
      indices.push_back(j);
  return indices;
}

void run() {
  auto mssh = Image<value_type>::open(argument[0]);
  if (mssh.ndim() != 5)
    throw Exception("5-D MSSH image expected.");

  Header header(mssh);
  auto bvals = parse_floats(header.keyval().find("shells")->second);

  Eigen::Matrix<double, Eigen::Dynamic, 4> grad;
  grad = File::Matrix::load_matrix(argument[1]).leftCols<4>();
  // copied from core/dwi/gradient.cpp; refactor upon merge
  Eigen::Array<default_type, Eigen::Dynamic, 1> squared_norms = grad.leftCols(3).rowwise().squaredNorm();
  for (ssize_t row = 0; row != grad.rows(); ++row) {
    if (squared_norms[row])
      grad.row(row).template head<3>().array() /= std::sqrt(squared_norms[row]);
  }

  // Apply rigid rotation to gradient table.
  transform_type T;
  T.setIdentity();
  auto opt = get_options("transform");
  if (!opt.empty())
    T = File::Matrix::load_transform(opt[0][0]);

  grad.leftCols<3>() = grad.leftCols<3>() * T.rotation().transpose();

  // Save output
  header.ndim() = 4;
  header.size(3) = grad.rows();
  DWI::set_DW_scheme(header, grad);
  Stride::set_from_command_line(header, Stride::contiguous_along_axis(3));
  header.datatype() = DataType::from_command_line(DataType::Float32);

  auto amp_data = Image<value_type>::create(argument[2], header);

  // Loop through shells
  for (size_t k = 0; k < bvals.size(); k++) {
    mssh.index(3) = k;
    auto idx = get_indices(grad.col(3), bvals[k]);
    if (idx.empty())
      continue;
    Eigen::MatrixXd directions(idx.size(), 3);
    for (size_t i = 0; i < idx.size(); i++) {
      directions.row(i) = grad.row(idx[i]).template head<3>();
    }
    MSSH2Amp mssh2amp(directions, Math::SH::LforN(mssh.size(4)), idx, get_options("nonnegative").size());
    ThreadedLoop("computing amplitudes", mssh, 0, 3, 2).run(mssh2amp, mssh, amp_data);
  }
}
