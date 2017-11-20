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
#include "image.h"
#include "transform.h"
#include "phase_encoding.h"
#include "interp/linear.h"
#include "interp/cubic.h"


using namespace MR;
using namespace App;


void usage ()
{
  AUTHOR = "Daan Christiaens (daan.christiaens@kcl.ac.uk)";

  SYNOPSIS = "Unwarp an EPI image according to its susceptibility field.";

  ARGUMENTS
    + Argument ("input",
                "the input image.").type_image_in ()
    + Argument ("field",
                "the B0 field.").type_file_in ()
    + Argument ("output",
                "the output, field unwrapped, image.").type_image_out ();

  OPTIONS
    + Option ("motion",
              "rigid motion parameters per volume or slice, applied to the field.")
      + Argument("T").type_file_in()

    + PhaseEncoding::ImportOptions

    + DataType::options();
}


using value_type = float;


class FieldUnwarp {
  MEMALIGN(FieldUnwarp)
  public:

    FieldUnwarp (const Image<value_type>& data, const Image<value_type>& field,
                 const Eigen::MatrixXd& petable, const Eigen::MatrixXd& motion) :
      dinterp (data, 0.0f), finterp (field, 0.0f),
      PE (petable.leftCols<3>()), motion (motion.leftCols<6>()),
      T0 (data), Tf (field),
      nv (data.size(3)), nz (data.size(2))
    {
      PE.array().colwise() *= petable.col(3).array();
      if (motion.rows() != nv && motion.rows() != nv*nz)
        throw Exception("Motion parameters incompatible with data dimensions.");
    }

    void operator() (Image<value_type>& out)
    {
      size_t v = out.index(3), z = out.index(2);
      transform_type Ts2r = get_Ts2r(v, z);
      dinterp.index(3) = v;
      Eigen::Vector3 vox, pos, RdB0;
      Eigen::RowVector3f dB0;
      value_type B0, jac = 1.0;
      for (auto l = Loop({0, 1})(out); l; l++) {
        assign_pos_of(out).to(vox);
        finterp.voxel(Ts2r * vox);
        finterp.value_and_gradient(B0, dB0);
        RdB0 = Ts2r.rotation().transpose() * dB0.transpose().cast<double>();
        pos = vox + B0 * PE.row(v).transpose();
        dinterp.voxel(pos);
        jac = 1.0 + 2. * PE.row(v) * RdB0;
        out.value() = jac * dinterp.value();
      }
    }

  private:
    Interp::Cubic<Image<value_type>> dinterp;
    Interp::LinearInterp<Image<value_type>, Interp::LinearInterpProcessingType::ValueAndDerivative> finterp;
    Eigen::Matrix<double, Eigen::Dynamic, 3> PE;
    Eigen::Matrix<double, Eigen::Dynamic, 6> motion;
    Transform T0, Tf;
    size_t nv, nz;

    inline Eigen::Matrix3d get_rotation(const double a1, const double a2, const double a3) const
    {
      Eigen::Matrix3d m;
      m = Eigen::AngleAxisd(a1, Eigen::Vector3d::UnitZ())
        * Eigen::AngleAxisd(a2, Eigen::Vector3d::UnitY())
        * Eigen::AngleAxisd(a3, Eigen::Vector3d::UnitX());
      return m;
    }

    inline transform_type get_transform(const Eigen::VectorXd& p) const
    {
      transform_type T;
      T.translation() = Eigen::Vector3d(p[0], p[1], p[2]);
      T.linear() = get_rotation(p[3], p[4], p[5]);
      return T;
    }

    inline transform_type get_Ts2r(const size_t v, const size_t z) const
    {
      transform_type Ts2r;
      if (motion.rows() == nv) {
        Ts2r = Tf.scanner2voxel * get_transform(motion.row(v)) * T0.voxel2scanner;
      } else {
        assert (motion.rows() == nv*nz);
        Ts2r = Tf.scanner2voxel * get_transform(motion.row(v*nz+z)) * T0.voxel2scanner;
      }
      return Ts2r;
    }

};


void run ()
{
  auto data = Image<value_type>::open(argument[0]);
  auto field = Image<value_type>::open(argument[1]);

  auto petable = PhaseEncoding::get_scheme(data);
  // -----------------------  // TODO: Eddy uses a reverse LR axis for storing
  petable.col(0) *= -1;       // the PE table, akin to the gradient table.
  // -----------------------  // Fix in the eddy import/export functions in core.

  // Apply rigid rotation to field.
  auto opt = get_options("motion");
  Eigen::MatrixXd motion;
  if (opt.size())
    motion = load_matrix<double>(opt[0][0]);
  else
    motion = Eigen::MatrixXd::Zero(data.size(3), 6);

  // Save output
  Header header (data);
  header.datatype() = DataType::from_command_line (DataType::Float32);

  auto out = Image<value_type>::create(argument[2], header);

  // Loop through shells
  FieldUnwarp func (data, field, petable, motion);
  ThreadedLoop("unwarping field", out, {2, 3}).run(func, out);

}


