/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */



#include "command.h"

#include <limits>

#include "math/SH.h"
#include "image.h"
#include "thread.h"
#include "algo/threaded_copy.h"

#include "dwi/tractography/GT/particlegrid.h"
#include "dwi/tractography/GT/gt.h"
#include "dwi/tractography/GT/externalenergy.h"
#include "dwi/tractography/GT/internalenergy.h"
#include "dwi/tractography/GT/mhsampler.h"


using namespace MR;
using namespace App;



void usage ()
{

  AUTHOR = "Daan Christiaens (daan.christiaens@kuleuven.be)";
  
  COPYRIGHT = "Copyright (C) 2015 KU Leuven, Dept. Electrical Engineering, ESAT/PSI,\n"
              "Herestraat 49 box 7003, 3000 Leuven, Belgium \n\n"

              "This is free software; see the source for copying conditions.\n"
              "There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.";

  DESCRIPTION
  + "Multi-Shell Multi-Tissue Global Tractography."
  
  + "This command will reconstruct the global white matter fibre tractogram that best "
    "explains the input DWI data, using a multi-tissue spherical convolution model."
  
  + "Example use: "
  
  + " $ tckglobal dwi.mif wmr.txt -riso csfr.txt -riso gmr.txt -mask mask.mif \n"
    "   -niter 1e8 -fod fod.mif -fiso fiso.mif tracks.tck "
  
  + "in which dwi.mif is the input image, wmr.txt is an anisotropic, multi-shell response function for WM, "
    "and csfr.txt and gmr.txt are isotropic response functions for CSF and GM. The output tractogram is "
    "saved to tracks.tck; ancillary output images fod.mif and fiso.mif contain the WM fODF and isotropic "
    "tissue fractions of CSF and GM respectively.";
  
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
  
  + Option ("grad", "specify the diffusion encoding scheme (required if not supplied in the header).")
    + Argument ("scheme").type_file_in()

  + Option ("mask", "only reconstruct the tractogram within the specified brain mask image.")
    + Argument ("image").type_image_in()

  + Option ("riso", "set one or more isotropic response functions. (multiple allowed)").allow_multiple()
    + Argument ("response").type_file_in()


  + OptionGroup("Parameters")
  
  + Option ("lmax", "set the maximum harmonic order for the output series. (default = 8)")
    + Argument ("order").type_integer(2, 8, 30)

  + Option ("length", "set the length of the particles (fibre segments). (default = 1.0 mm)")
    + Argument ("size").type_float(1e-6, 1.0, 10.0)
      
  + Option ("weight", "set the weight by which particles contribute to the model. (default = 0.1)")
    + Argument ("w").type_float(1e-6, 0.1, 1.0)

  + Option ("ppot", "set the particle potential, i.e., the cost of adding one segment, relative to the particle weight. (default = 5% w)")
    + Argument ("u").type_float(0.0, 0.05, 1.)

  + Option ("cpot", "set the connection potential, i.e., the energy term that drives two segments together. (default = 0.5)")
    + Argument ("v").type_float(0, 0.5, 1e6)

  + Option ("t0", "set the initial temperature of the metropolis hastings optimizer. (default = 0.1)")
    + Argument ("start").type_float(1e-6, 0.1, 1e6)

  + Option ("t1", "set the final temperature of the metropolis hastings optimizer. (default = 0.001)")
    + Argument ("end").type_float(1e-6, 0.001, 1e6)

  + Option ("niter", "set the number of iterations of the metropolis hastings optimizer. (default = 10M)")
    + Argument ("n").type_integer(0, 10000000, std::numeric_limits<int64_t>::max())


  + OptionGroup("Output options")

  + Option ("fod", "filename of the resulting fODF image.")
    + Argument ("odf").type_image_out()
  + Option ("noapo", "disable spherical convolution of fODF with apodized PSF.")

  + Option ("fiso", "filename of the resulting isotropic fractions image.")
    + Argument ("iso").type_image_out()

  + Option ("eext", "filename of the resulting image of the residual external energy.")
    + Argument ("eext").type_image_out()

  + Option ("etrend", "internal and external energy trend and cooling statistics.")
    + Argument ("stats").type_file_out()


  + OptionGroup("Advanced parameters, if you really know what you're doing")
  
  + Option ("balance", "balance internal and external energy. (default = 0)\n"
            "Negative values give more weight to the internal energy, positive to the external energy.")
    + Argument ("b").type_float(-100, 0, 100)

  + Option ("density", "set the desired density of the free Poisson process. (default = 1.0)")
    + Argument ("lambda").type_float(0, 1., std::numeric_limits<float>::max())

  + Option ("prob", "set the probabilities of generating birth, death, randshift, optshift "
            "and connect probabilities respectively. (default = .25,.05,.25,.10,.35)")
    + Argument ("prob").type_sequence_float()

  + Option ("beta", "set the width of the Hanning interpolation window. (in [0, 1], default = 0)\n"
            "If used, a mask is required, and this mask must keep at least one voxel distance to the image bounding box.")
    + Argument ("b").type_float(0.0, 0.0, 1.0)

  + Option ("lambda", "set the weight of the internal energy directly. (default = 1.0)\n"
            "If provided, any value of -balance will be ignored.")
    + Argument ("lam").type_float(0.0, 1.0, 1e5);


};


template<typename T>
class __copy_fod {
  public:
    __copy_fod (const int lmax, const double weight, const bool apodise) 
      : w(weight), a(apodise), apo (lmax), SH_out (Math::SH::NforL(lmax)) { }

    void operator() (Image<T>& in, Image<T>& out) {
      out.row(3) = w * (a ? Math::SH::sconv (SH_out, apo.RH_coefs(), in.row(3)) : in.row(3));
    }

  private:
    T w;
    bool a;
    Math::SH::aPSF<T> apo;
    Eigen::Matrix<T, Eigen::Dynamic, 1> SH_out;

};




void run ()
{

  using namespace DWI::Tractography::GT;
  
  // Inputs -----------------------------------------------------------------------------
  
  auto dwi = Image<float>::open(argument[0]).with_direct_io(3);
  
  Properties properties;
  properties.resp_WM = load_matrix<float> (argument[1]);
  double wmscale2 = (properties.resp_WM(0,0)*properties.resp_WM(0,0))/M_4PI;
  
  Eigen::VectorXf riso;
  auto opt = get_options("riso");
  for (auto popt : opt)
  {
    riso = load_vector<float>(popt[0]);
    properties.resp_ISO.push_back(riso);
  }
  
  auto mask = Image<bool>();
  opt = get_options("mask");
  if (opt.size()) {
    mask = Image<bool>::open(opt[0][0]);
    check_dimensions(dwi, mask, 0, 3);
  }
  
  // Parameters -------------------------------------------------------------------------
  
  Particle::L = get_option_value("length", 1.0);    
  double cpot = get_option_value("cpot", 1.0);
  
  properties.Lmax = get_option_value("lmax", 8);
  properties.p_birth = 0.25;
  properties.p_death = 0.05;
  properties.p_shift = 0.25;
  properties.p_optshift = 0.10;
  properties.p_connect = 0.35;
  properties.density = get_option_value("density", 1.);
  properties.weight = get_option_value("weight", 0.1);
  properties.lam_ext = 1.;
  properties.lam_int = 1.;
  properties.beta = get_option_value("beta", 0.0);
  properties.ppot = 0.0;
  
  double lam = 0.0;
  opt = get_options("balance");
  if (opt.size())
  {
    lam = opt[0][0];
    double b = 1.0 / (1.0 + exp(-lam));
    properties.lam_ext = 2*b;
    properties.lam_int = 2*(1-b);
  }
  
  opt = get_options("prob");
  if (opt.size())
  {
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
  
  uint64_t niter = get_option_value("niter", 10000000);
  double t0 = get_option_value("t0", 0.1);
  double t1 = get_option_value("t1", 0.001);
  
  double mu = get_option_value("ppot", 0.0);
  properties.ppot = mu * wmscale2 * properties.weight;
  
  opt = get_options("lambda");
  if (opt.size()) {
    properties.lam_ext = 1.0;
    properties.lam_int = opt[0][0];
  }


  // Prepare data structures ------------------------------------------------------------
  
  INFO("Initialise data structures for global tractography.");
  
  Stats stats (t0, t1, niter);
  opt = get_options("etrend");
  if (opt.size())
    stats.open_stream(opt[0][0]);
  
  ParticleGrid pgrid (dwi);
  
  ExternalEnergyComputer* Eext = new ExternalEnergyComputer(stats, dwi, properties);
  InternalEnergyComputer* Eint = new InternalEnergyComputer(stats, pgrid);
  Eint->setConnPot(cpot);
  EnergySumComputer* Esum = new EnergySumComputer(stats, Eint, properties.lam_int, Eext, properties.lam_ext / ( wmscale2 * properties.weight*properties.weight));
  
  MHSampler mhs (dwi, properties, stats, pgrid, Esum, mask);   // All EnergyComputers are recursively destroyed upon destruction of mhs, except for the shared data.
  
  
  INFO("Start MH sampler");
  
  auto t = Thread::run (Thread::multi(mhs), "MH sampler");
  t.wait();
  
  INFO("Final no. particles: " + std::to_string(pgrid.getTotalCount()));
  INFO("Final external energy: " + std::to_string(stats.getEextTotal()));
  INFO("Final internal energy: " + std::to_string(stats.getEintTotal()));
  
  
  // Copy results to output buffers -----------------------------------------------------
  
  // Save the tracks (output)
  INFO("Saving tracks to file");
  MR::DWI::Tractography::Properties ftfileprops;
  ftfileprops.comments.push_back("global tractography");
  ftfileprops.comments.push_back("");
  ftfileprops.comments.push_back("segment length = " + std::to_string((long double) Particle::L));
  ftfileprops.comments.push_back("segment weight = " + std::to_string((long double) properties.weight));
  ftfileprops.comments.push_back("");
  ftfileprops.comments.push_back("connection potential = " + std::to_string((long double) cpot));
  ftfileprops.comments.push_back("particle potential = " + std::to_string((long double) mu));
  ftfileprops.comments.push_back("");
  ftfileprops.comments.push_back("no. iterations = " + std::to_string((long long int) niter));
  ftfileprops.comments.push_back("T0 = " + std::to_string((long double) t0));
  ftfileprops.comments.push_back("T1 = " + std::to_string((long double) t1));
  
  MR::DWI::Tractography::Writer<float> writer (argument[2], ftfileprops);
  pgrid.exportTracks(writer);
  
  
  // Save fiso, tod and eext
  Header header (dwi);
  header.datatype() = DataType::Float32;
  
  opt = get_options("fod");
  if (opt.size()) {
    INFO("Saving fODF image to file");
    header.size(3) = Math::SH::NforL(properties.Lmax);
    auto fODF = Image<float>::create (opt[0][0], header);
    auto f = __copy_fod<float>(properties.Lmax, properties.weight, !get_options("noapo").size());
    ThreadedLoop(Eext->getTOD(), 0, 3).run(f, Eext->getTOD(), fODF);
  }
  
  opt = get_options("fiso");
  if (opt.size()) {
    if (properties.resp_ISO.size() > 0) {
      INFO("Saving isotropic fractions to file");
      header.size(3) = properties.resp_ISO.size();
      auto Fiso = Image<float>::create (opt[0][0], header);
      threaded_copy(Eext->getFiso(), Fiso);
    }
    else {
      WARN("Ignore saving file " + opt[0][0] + ", because no isotropic response functions were provided.");
    }
  }
  
  opt = get_options("eext");
  if (opt.size()) {
    INFO("Saving external energy to file");
    header.set_ndim(3);
    auto EextI = Image<float>::create (opt[0][0], header);
    threaded_copy(Eext->getEext(), EextI);
  }
  
  


}

