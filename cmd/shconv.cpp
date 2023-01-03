/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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
#include "memory.h"
#include "progressbar.h"
#include "algo/threaded_loop.h"
#include "image.h"
#include "math/SH.h"
#include "math/ZSH.h"

using namespace MR;
using namespace App;


void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au) and J-Donald Tournier (jdtournier@gmail.com)";

  SYNOPSIS = "Perform spherical convolution";

  DESCRIPTION
    + "Provided with matching pairs of response function and ODF images "
      "(containing SH coefficients), perform spherical convolution to provide the "
      "corresponding SH coefficients of the signal."
    + "If multiple pairs of inputs are provided, their contributions will be "
       "summed into a single output."
    +  "If the responses are multi-shell (with one line of coefficients per "
       "shell), the output will be a 5-dimensional image, with the SH "
       "coefficients of the signal in each shell stored at different indices "
       "along the 5th dimension."
    + Math::SH::encoding_description;

  DESCRIPTION
  + Math::SH::encoding_description;

  ARGUMENTS
    + Argument ("odf response", "pairs of input ODF image and corresponding responses").allow_multiple()
    + Argument ("SH_out", "the output spherical harmonics coefficients image.").type_image_out ();

  OPTIONS
    + DataType::options()
    + Stride::Options;
}



using value_type = float;


class SConvFunctor { MEMALIGN(SConvFunctor)
  public:
  SConvFunctor (const vector<Eigen::MatrixXd>& responses, vector<Image<value_type>>& inputs) :
    responses (responses),
    inputs (inputs) { }

    void operator() (Image<value_type>& output)
    {
      for (size_t n = 0; n < inputs.size(); ++n) {
        assign_pos_of (output, 0, 3).to (inputs[n]);
        in = inputs[n].row (3);
        for (ssize_t s = 0; s < responses[n].rows(); ++s) {
          Math::SH::sconv (out, responses[n].row(s), in);
          if (output.ndim() > 4)
            output.index(4) = s;
          for (ssize_t k = 0; k < out.size(); ++k) {
            output.index(3) = k;
            output.value() += out[k];
          }
        }
      }
    }

  protected:
    const vector<Eigen::MatrixXd>& responses;
    vector<Image<value_type>> inputs;
    Eigen::VectorXd in, out;

};






void run()
{
  if (!(argument.size() & size_t(1U)))
    throw Exception ("unexpected number of arguments");

  vector<Image<value_type>> inputs ((argument.size() - 1) / 2);
  vector<Eigen::MatrixXd> responses (inputs.size());

  size_t lmax = 0;
  for (size_t n = 0; n < inputs.size(); ++n) {
    inputs[n] = Image<value_type>::open (argument[2*n]);
    Math::SH::check (inputs[n]);
    if (inputs[n].ndim() > 4 && inputs[n].size(4) > 1)
      throw Exception ("input ODF contains more than 4 dimensions");

    responses[n] = load_matrix (argument[2*n+1]);
    responses[n].conservativeResizeLike (Eigen::MatrixXd::Zero (responses[n].rows(), Math::ZSH::NforL (Math::SH::LforN (inputs[n].size (3)))));
    lmax = std::max (Math::ZSH::LforN (responses[n].cols()), lmax);

    for (ssize_t k = 0; k < responses[n].rows(); ++k)
      responses[n].row(k) = Math::ZSH::ZSH2RH (responses[n].row(k));

    if (n) {
      if (responses[n].rows() != responses[0].rows())
        throw Exception ("number of shells differs between response files");
      check_dimensions (inputs[n], inputs[0], 0, 3);
    }
  }


  Header header (inputs[0]);
  if (responses[0].rows() > 1) {
    header.ndim() = 5;
    header.size(4) = responses[0].rows();
  }
  else
    header.ndim() = 4;
  header.size(3) = Math::SH::NforL (lmax);
  Stride::set_from_command_line (header, Stride::contiguous_along_axis (3, header));
  header.datatype() = DataType::from_command_line (DataType::Float32);

  auto output = Image<value_type>::create (argument[argument.size()-1], header);

  SConvFunctor sconv (responses, inputs);
  ThreadedLoop ("performing spherical convolution", inputs[0], 0, 3).run (sconv, output);
}
