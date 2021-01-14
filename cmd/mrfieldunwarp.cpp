/* Copyright (c) 2017-2019 Daan Christiaens
 *
 * MRtrix and this add-on module are distributed in the hope
 * that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 */

#include <sstream>

#include "command.h"
#include "image.h"
#include "transform.h"
#include "phase_encoding.h"
#include "interp/linear.h"
#include "interp/cubic.h"

#include "dwi/svr/param.h"


using namespace MR;
using namespace App;


void usage ()
{
  AUTHOR = "Daan Christiaens (daan.christiaens@kcl.ac.uk)";

  SYNOPSIS = "Unwarp an EPI image according to its susceptibility field.";

  DESCRIPTION
    + "This command takes EPI data and a field map in Hz, and inverts the distortion introduced "
      "by the B0 field inhomogeneity. The command can also take motion parameters for each volume "
      "or slice, but does not invert the motion. The motion parameters are only used to align the "
      "field with the moving subject."

    + "If the field map is estimated using FSL Topup, make sure to use the --fmap output "
      "(the field map in Hz) instead of the spline coefficient representation saved by default.";

  ARGUMENTS
    + Argument ("input",
                "the input image.").type_image_in ()
    + Argument ("field",
                "the B0 field map in Hz.").type_file_in ()
    + Argument ("output",
                "the field-unwrapped image.").type_image_out ();

  OPTIONS
    + Option ("motion",
              "rigid motion parameters per volume or slice, applied to the field.")
      + Argument("T").type_file_in()

    + Option ("fidx",
              "index of the input volume to which the field is aligned. (default = none)")
      + Argument("vol").type_integer(0)

    + Option ("nomodulation", "disable Jacobian intensity modulation")

    + PhaseEncoding::ImportOptions

    + DataType::options();
}


using value_type = float;


class FieldUnwarp {
  MEMALIGN(FieldUnwarp)
  public:

    FieldUnwarp (const Image<value_type>& data, const Image<value_type>& field,
                 const Eigen::MatrixXd& petable, const Eigen::MatrixXd& motion,
                 const int fidx = -1, const bool nomod = false) :
      dinterp (data, 0.0f), finterp (field, 0.0f),
      PE (petable.leftCols<3>()), motion (motion.leftCols<6>()), T0 (data),
      nv (data.size(3)), nz (data.size(2)), ne (motion.rows() / nv), nomod (nomod)
    {
      PE.array().colwise() *= petable.col(3).array();
      if ((nv*nz) % motion.rows())
        throw Exception("Motion parameters incompatible with data dimensions.");
      Tf = Transform(field).scanner2voxel * T0.voxel2scanner;
      if (fidx >= 0 && fidx < nv)
        Tf = Tf * get_Ts2r_avg(fidx).inverse();
    }

    void operator() (Image<value_type>& out)
    {
      size_t v = out.index(3), z = out.index(2);
      transform_type Ts2r = Tf * get_Ts2r(v, z);
      dinterp.index(3) = v;
      Eigen::Vector3 vox, pos, RdB0;
      Eigen::RowVector3f dB0;
      value_type B0, jac = 1.0;
      for (auto l = Loop(0,2)(out); l; l++) {
        assign_pos_of(out).to(vox);
        finterp.voxel(Ts2r * vox);
        finterp.value_and_gradient(B0, dB0);
        RdB0 = Ts2r.rotation().transpose() * dB0.transpose().cast<double>();
        pos = vox + B0 * PE.row(v).transpose();
        dinterp.voxel(pos);
        if (!nomod)
          jac = 1.0 + 2. * PE.row(v) * RdB0;
        out.value() = jac * dinterp.value();
      }
    }

  private:
    Interp::Linear<Image<value_type>> dinterp;
    Interp::LinearInterp<Image<value_type>, Interp::LinearInterpProcessingType::ValueAndDerivative> finterp;
    Eigen::Matrix<double, Eigen::Dynamic, 3> PE;
    Eigen::Matrix<double, Eigen::Dynamic, 6> motion;
    Transform T0;
    transform_type Tf;
    size_t nv, nz, ne;
    bool nomod;

    inline transform_type get_transform(const Eigen::VectorXd& p) const
    {
      transform_type T (DWI::SVR::se3exp(p).cast<double>());
      return T;
    }

    inline transform_type get_Ts2r(const size_t v, const size_t z) const
    {
      transform_type Ts2r = T0.scanner2voxel * get_transform(motion.row(v*ne+z%ne)) * T0.voxel2scanner;
      return Ts2r;
    }

    inline transform_type get_Ts2r_avg(const size_t v) const
    {
      transform_type Ts2r = T0.scanner2voxel *
              get_transform(motion.block(v*ne,0,ne,6).colwise().mean()) * T0.voxel2scanner;
      return Ts2r;
    }

};


void run ()
{
  auto data = Image<value_type>::open(argument[0]);
  auto field = Image<value_type>::open(argument[1]);
  if (not voxel_grids_match_in_scanner_space(data, field)) {
    WARN("Field map voxel grid does not match the input data. "
         "If the field map was estimated using FSL TOPUP, make sure to use the --fmap output "
         "(the field map in Hz) instead of the spline coefficient representation.");
  }

  auto petable = PhaseEncoding::get_scheme(data);
  if (petable.rows() != data.size(3))
    throw Exception ("Invalid PE table.");
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

  // field alignment
  int fidx = get_option_value("fidx", -1);
  if (fidx >= data.size(3)) throw Exception("field index invalid.");

  // other options
  opt = get_options("nomodulation");
  bool nomod = opt.size();

  // Save output
  Header header (data);
  header.datatype() = DataType::from_command_line (DataType::Float32);

  auto out = Image<value_type>::create(argument[2], header);

  // Loop through shells
  FieldUnwarp func (data, field, petable, motion, fidx, nomod);
  ThreadedLoop("unwarping field", out, {2, 3}).run(func, out);

}


