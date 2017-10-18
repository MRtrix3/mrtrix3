/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#include <sstream>

#include "command.h"
#include "math/SH.h"
#include "image.h"
#include "dwi/gradient.h"


using namespace MR;
using namespace App;


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


using value_type = float;


class MSSH2Amp { MEMALIGN(MSSH2Amp)
  public:
    template <class MatrixType>
    MSSH2Amp (const MatrixType& dirs, const size_t lmax, const vector<size_t>& idx, bool nonneg) :
      transformer (dirs.template cast<value_type>(), lmax),
      bidx (idx),
      nonnegative (nonneg),
      sh (transformer.n_SH()),
      amp (transformer.n_amp()) { }
    
    void operator() (Image<value_type>& in, Image<value_type>& out) {
      sh = in.row (4);
      transformer.SH2A(amp, sh);
      if (nonnegative)
        amp = amp.cwiseMax(value_type(0.0));
      for (size_t j = 0; j < amp.size(); j++) {
        out.index(3) = bidx[j];
        out.value() = amp[j];
      }
    }

  private:
    const Math::SH::Transform<value_type> transformer;
    const vector<size_t>& bidx;
    const bool nonnegative;
    Eigen::Matrix<value_type, Eigen::Dynamic, 1> sh, amp;
};

template <class VectorType>
inline vector<size_t> get_indices(const VectorType& blist, const value_type bval)
{
  vector<size_t> indices;
  for (size_t j = 0; j < blist.size(); j++)
    if ((blist[j] > bval - DWI_SHELLS_EPSILON) && (blist[j] < bval + DWI_SHELLS_EPSILON))
      indices.push_back(j);
  return indices;
}


void run ()
{
  auto mssh = Image<value_type>::open(argument[0]);
  if (mssh.ndim() != 5)
    throw Exception("5-D MSSH image expected.");

  Header header (mssh);
  auto bvals = parse_floats (header.keyval().find("shells")->second);

  Eigen::MatrixXd grad;
  grad = load_matrix(argument[1]);

  // Apply rigid rotation to gradient table.
  transform_type T;
  T.setIdentity();
  auto opt = get_options("transform");
  if (opt.size())
    T = load_transform(opt[0][0]);

  grad.block(0,0,grad.rows(),3) = grad.block(0,0,grad.rows(),3) * T.rotation().transpose();

  // Save output
  header.ndim() = 4;
  header.size(3) = grad.rows();
  DWI::set_DW_scheme (header, grad);
  Stride::set_from_command_line (header, Stride::contiguous_along_axis (3));
  header.datatype() = DataType::from_command_line (DataType::Float32);

  auto amp_data = Image<value_type>::create(argument[2], header);

  // Loop through shells
  for (size_t k = 0; k < bvals.size(); k++) {
    mssh.index(3) = k;
    auto idx = get_indices(grad.col(3), bvals[k]);
    if (idx.empty())
      continue;
    auto directions = DWI::gen_direction_matrix (grad, idx);
    MSSH2Amp mssh2amp (directions, Math::SH::LforN (mssh.size(4)),
                       idx, get_options("nonnegative").size());
    ThreadedLoop("computing amplitudes", mssh, 0, 3, 2).run(mssh2amp, mssh, amp_data);
  }

}


