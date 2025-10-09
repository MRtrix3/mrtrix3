/* Copyright (c) 2008-2025 the MRtrix3 contributors.
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

#include <limits>

#include "algo/threaded_copy.h"
#include "file/matrix.h"
#include "image.h"
#include "math/SH.h"
#include "mrtrix_version.h"
#include "thread.h"

#include "dwi/tractography/GT/externalenergy.h"
#include "dwi/tractography/GT/gt.h"
#include "dwi/tractography/GT/internalenergy.h"
#include "dwi/tractography/GT/mhsampler.h"
#include "dwi/tractography/GT/particlegrid.h"

using namespace MR;
using namespace App;

constexpr ssize_t default_lmax = 8;
constexpr default_type default_length = 1.0;
constexpr default_type default_weight = 0.1;
constexpr default_type default_ppot = 0.05;
constexpr default_type default_cpot = 0.5;
constexpr default_type default_t0 = 0.1;
constexpr default_type default_t1 = 0.001;
constexpr ssize_t default_niter = 10000000;
constexpr default_type default_balance = 0.0; // TODO Not accessed?
constexpr default_type default_density = 1.0;

constexpr default_type default_prob_birth = 0.25;
constexpr default_type default_prob_death = 0.05;
constexpr default_type default_prob_randshift = 0.25;
constexpr default_type default_prob_optshift = 0.10;
constexpr default_type default_prob_connect = 0.35;

constexpr default_type default_beta = 0.0;
constexpr default_type default_lambda = 1.0; // TODO Not accessed?

// clang-format off
void usage() {

  AUTHOR = "Daan Christiaens (daan.christiaens@kcl.ac.uk)";

  COPYRIGHT = "Copyright (C) 2015 KU Leuven, Dept. Electrical Engineering, ESAT/PSI,\n"
              "Herestraat 49 box 7003, 3000 Leuven, Belgium \n\n"

              "This is free software; see the source for copying conditions.\n"
              "There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.";

  SYNOPSIS = "Multi-Shell Multi-Tissue Global Tractography";

  DESCRIPTION
  + "This command will reconstruct the global white matter fibre tractogram"
    " that best explains the input DWI data,"
    " using a multi-tissue spherical convolution model."

  + "A more thorough description of the operation of global tractography in MRtrix3"
    " can be found in the online documentation: \n"
    "https://mrtrix.readthedocs.io/en/" MRTRIX_BASE_VERSION "/quantitative_structural_connectivity/global_tractography.html";

  EXAMPLES
  + Example("Basic example usage",
            "tckglobal dwi.mif wmr.txt -riso csfr.txt -riso gmr.txt -mask mask.mif"
            " -niter 1e9 -fod fod.mif -fiso fiso.mif tracks.tck",
            "dwi.mif is the input image,"
            " wmr.txt is an anisotropic1, multi-shell response function for WM, "
            "and csfr.txt and gmr.txt are isotropic response functions for CSF and GM."
            " The output tractogram is saved to tracks.tck."
            " Optional output images fod.mif and fiso.mif contain the predicted WM fODF"
            " and isotropic tissue fractions of CSF and GM respectively,"
            " estimated as part of the global optimization"
            " and thus affected by spatial regularization.");

  REFERENCES
  + "Christiaens, D.; Reisert, M.; Dhollander, T.; Sunaert, S.; Suetens, P. & Maes, F. " // Internal
    "Global tractography of multi-shell diffusion-weighted imaging data using a multi-tissue model. "
    "NeuroImage, 2015, 123, 89-101";

  ARGUMENTS
  + Argument ("source", "the image containing the raw DWI data.").type_image_in()
  + Argument ("response", "the response of a track segment on the DWI signal.").type_file_in()
  + Argument ("tracks", "the output file containing the tracks generated.").type_tracks_out();

  OPTIONS
  + OptionGroup("Input options")

  + Option ("grad", "specify the diffusion encoding scheme"
                    " (required if not supplied in the header).")
    + Argument ("scheme").type_file_in()

  + Option ("mask", "only reconstruct the tractogram within the specified brain mask image.")
    + Argument ("image").type_image_in()

  + Option ("riso", "set one or more isotropic response functions."
                    " (multiple allowed)").allow_multiple()
    + Argument ("response").type_file_in()

  + OptionGroup("Parameters")

  + Option ("lmax", "set the maximum harmonic order for the output series."
                    " (default = " + str(default_lmax) + ")")
    + Argument ("order").type_integer(2, 30)

  + Option ("length", "set the length of the particles (fibre segments)."
                      " (default = " + str(default_length, 2) + "mm)")
    + Argument ("size").type_float(1e-6)

  + Option ("weight", "set the weight by which particles contribute to the model."
                      " (default = " + str(default_weight, 2) + ")")
    + Argument ("w").type_float(1e-6, 1.0)

  + Option ("ppot", "set the particle potential,"
                    " i.e., the cost of adding one segment,"
                    " relative to the particle weight."
                    " (default = " + str(default_ppot, 2) + ")")
    + Argument ("u").type_float(0.0, 1.0)

  + Option ("cpot", "set the connection potential,"
                    " i.e., the energy term that drives two segments together."
                    " (default = " + str(default_cpot, 2) + ")")
    + Argument ("v").type_float(0.0)

  + Option ("t0", "set the initial temperature of the metropolis hastings optimizer."
                  " (default = " + str(default_t0, 2) + ")")
    + Argument ("start").type_float(1e-6, 1e6)

  + Option ("t1", "set the final temperature of the metropolis hastings optimizer."
                  " (default = " + str(default_t1, 2) + ")")
    + Argument ("end").type_float(1e-6, 1e6)

  + Option ("niter", "set the number of iterations of the metropolis hastings optimizer."
                     " (default = " + str(default_niter/1000000) + "M)")
    + Argument ("n").type_integer(0)


  + OptionGroup("Output options")

  + Option ("fod", "Predicted fibre orientation distribution function (fODF)."
            " This fODF is estimated as part of the global track optimization,"
            " and therefore incorporates the spatial regularization that it imposes."
            " Internally, the fODF is represented as a discrete sum"
            " of apodized point spread functions (aPSF)"
            " oriented along the directions of all particles in the voxel,"
            " used to predict the DWI signal from the particle configuration.")
    + Argument ("odf").type_image_out()

  + Option ("noapo", "disable spherical convolution of fODF with apodized PSF,"
                     " to output a sum of delta functions rather than a sum of aPSFs.")

  + Option ("fiso", "Predicted isotropic fractions of the tissues"
                    " for which response functions were provided with -riso."
                    " Typically, these are CSF and GM.")
    + Argument ("iso").type_image_out()

  + Option ("eext", "Residual external energy in every voxel.")
    + Argument ("eext").type_image_out()

  + Option ("etrend", "internal and external energy trend and cooling statistics.")
    + Argument ("stats").type_file_out()


  + OptionGroup("Advanced parameters, if you really know what you're doing")

  + Option ("balance", "balance internal and external energy."
                       " (default = " + str(default_balance, 2) + ")."
                       " Negative values give more weight to the internal energy;"
                       " positive to the external energy.")
    + Argument ("b").type_float(-100.0, 100.0)

  + Option ("density", "set the desired density of the free Poisson process."
                       " (default = " + str(default_density, 2) + ")")
    + Argument ("lambda").type_float(0.0)

  + Option ("prob", "set the probabilities of generating"
                    " birth, death, randshift, optshift and connect proposals respectively."
                    " (default = "
                    + str(default_prob_birth, 2) + ","
                    + str(default_prob_death, 2) + ","
                    + str(default_prob_randshift, 2) + ","
                    + str(default_prob_optshift, 2) + ","
                    + str(default_prob_connect, 2) + ")")
    + Argument ("prob").type_sequence_float()

  + Option ("beta", "set the width of the Hanning interpolation window."
                    " (in [0, 1], default = " + str(default_beta, 2) + "). "
                    " If used, a mask is required,"
                    " and this mask must keep at least one voxel distance to the image bounding box.")
    + Argument ("b").type_float(0.0, 1.0)

  + Option ("lambda", "set the weight of the internal energy directly."
                      " (default = " + str(default_lambda, 2) + ")."
                      " If provided, any value of -balance will be ignored.")
    + Argument ("lam").type_float(0.0);

}
// clang-format on

template <typename T> class __copy_fod {
public:
  __copy_fod(const int lmax, const double weight, const bool apodise)
      : w(weight), a(apodise), apo(lmax), SH_in(Math::SH::NforL(lmax)), SH_out(SH_in.size()) {}

  void operator()(Image<T> &in, Image<T> &out) {
    SH_in = in.row(3);
    out.row(3) = w * (a ? Math::SH::sconv(SH_out, apo.RH_coefs(), SH_in) : SH_in);
  }

private:
  T w;
  bool a;
  Math::SH::aPSF<T> apo;
  Eigen::Matrix<T, Eigen::Dynamic, 1> SH_in, SH_out;
};

void run() {

  using namespace DWI::Tractography::GT;

  // Inputs -----------------------------------------------------------------------------
  Header header_in = Header::open(argument[0]);

  Properties properties;
  properties.resp_WM = File::Matrix::load_matrix<float>(argument[1]);
  double wmscale2 = (properties.resp_WM(0, 0) * properties.resp_WM(0, 0)) / M_4PI;

  Eigen::VectorXf riso;
  auto opt = get_options("riso");
  for (auto popt : opt) {
    riso = File::Matrix::load_vector<float>(popt[0]);
    properties.resp_ISO.push_back(riso);
  }

  auto mask = Image<bool>();
  opt = get_options("mask");
  if (!opt.empty()) {
    mask = Image<bool>::open(opt[0][0]);
    check_dimensions(header_in, mask, 0, 3);
  }

  // Parameters -------------------------------------------------------------------------

  Particle::L = get_option_value("length", default_length);
  const double cpot = get_option_value("cpot", default_cpot);

  properties.Lmax = get_option_value("lmax", default_lmax);
  properties.p_birth = default_prob_birth;
  properties.p_death = default_prob_death;
  properties.p_shift = default_prob_randshift;
  properties.p_optshift = default_prob_optshift;
  properties.p_connect = default_prob_connect;
  properties.density = get_option_value("density", default_density);
  properties.weight = get_option_value("weight", default_weight);
  properties.lam_ext = 1.0;
  properties.lam_int = 1.0;
  properties.beta = get_option_value("beta", default_beta);

  opt = get_options("balance");
  if (!opt.empty()) {
    double lam = opt[0][0];
    double b = 1.0 / (1.0 + exp(-lam));
    properties.lam_ext = 2 * b;
    properties.lam_int = 2 * (1 - b);
  }

  opt = get_options("prob");
  if (!opt.empty()) {
    auto prob = opt[0][0].as_sequence_float();
    if (prob.size() == 5) {
      properties.p_birth = prob[0];
      properties.p_death = prob[1];
      properties.p_shift = prob[2];
      properties.p_optshift = prob[3];
      properties.p_connect = prob[4];
    } else {
      throw Exception("Specified list of proposal probabilities is invalid.");
    }
  }

  const uint64_t niter = get_option_value<uint64_t>("niter", default_niter);
  const double t0 = get_option_value("t0", default_t0);
  const double t1 = get_option_value("t1", default_t1);

  const double mu = get_option_value("ppot", default_ppot);
  properties.ppot = mu * wmscale2 * properties.weight;

  opt = get_options("lambda");
  if (!opt.empty()) {
    properties.lam_ext = 1.0;
    properties.lam_int = opt[0][0];
  }

  // Prepare data structures ------------------------------------------------------------

  INFO("Initialise data structures for global tractography.");

  Stats stats(t0, t1, niter);
  opt = get_options("etrend");
  if (!opt.empty())
    stats.open_stream(opt[0][0]);

  auto dwi = header_in.get_image<float>().with_direct_io(3);
  ParticleGrid pgrid(dwi);
  ExternalEnergyComputer *Eext = new ExternalEnergyComputer(stats, header_in, properties);
  InternalEnergyComputer *Eint = new InternalEnergyComputer(stats, pgrid);
  Eint->setConnPot(cpot);
  EnergySumComputer *Esum = new EnergySumComputer(
      stats, Eint, properties.lam_int, Eext, properties.lam_ext / (wmscale2 * properties.weight * properties.weight));

  MHSampler mhs(
      dwi,
      properties,
      stats,
      pgrid,
      Esum,
      mask); // All EnergyComputers are recursively destroyed upon destruction of mhs, except for the shared data.

  INFO("Start MH sampler");

  Thread::run(Thread::multi(mhs), "MH sampler");

  INFO("Final no. particles: " + std::to_string(pgrid.getTotalCount()));
  INFO("Final external energy: " + std::to_string(stats.getEextTotal()));
  INFO("Final internal energy: " + std::to_string(stats.getEintTotal()));

  // Copy results to output buffers -----------------------------------------------------

  // Save the tracks (output)
  INFO("Saving tracks to file");
  MR::DWI::Tractography::Properties ftfileprops;
  ftfileprops.comments.push_back("global tractography");
  ftfileprops.comments.push_back("");
  ftfileprops.comments.push_back("segment length = " + std::to_string((long double)Particle::L));
  ftfileprops.comments.push_back("segment weight = " + std::to_string((long double)properties.weight));
  ftfileprops.comments.push_back("");
  ftfileprops.comments.push_back("connection potential = " + std::to_string((long double)cpot));
  ftfileprops.comments.push_back("particle potential = " + std::to_string((long double)mu));
  ftfileprops.comments.push_back("");
  ftfileprops.comments.push_back("no. iterations = " + std::to_string((long long int)niter));
  ftfileprops.comments.push_back("T0 = " + std::to_string((long double)t0));
  ftfileprops.comments.push_back("T1 = " + std::to_string((long double)t1));

  MR::DWI::Tractography::Writer<float> writer(argument[2], ftfileprops);
  pgrid.exportTracks(writer);

  // Save fiso, tod and eext
  Header header_out(dwi);
  header_out.datatype() = DataType::Float32;

  opt = get_options("fod");
  if (!opt.empty()) {
    INFO("Saving fODF image to file");
    header_out.size(3) = Math::SH::NforL(properties.Lmax);
    auto fODF = Image<float>::create(opt[0][0], header_out);
    auto f = __copy_fod<float>(properties.Lmax, properties.weight, get_options("noapo").empty());
    ThreadedLoop(Eext->getTOD(), 0, 3).run(f, Eext->getTOD(), fODF);
  }

  opt = get_options("fiso");
  if (!opt.empty()) {
    if (!properties.resp_ISO.empty()) {
      INFO("Saving isotropic fractions to file");
      header_out.size(3) = properties.resp_ISO.size();
      auto Fiso = Image<float>::create(opt[0][0], header_out);
      threaded_copy(Eext->getFiso(), Fiso);
    } else {
      WARN("Ignore saving file " + std::string(opt[0][0]) + ", because no isotropic response functions were provided.");
    }
  }

  opt = get_options("eext");
  if (!opt.empty()) {
    INFO("Saving external energy to file");
    header_out.ndim() = 3;
    auto EextI = Image<float>::create(opt[0][0], header_out);
    threaded_copy(Eext->getEext(), EextI);
  }
}
