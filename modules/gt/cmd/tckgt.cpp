/*
    Copyright 2013 KU Leuven, Dept. Electrical Engineering, Medical Image Computing
    Herestraat 49 box 7003, 3000 Leuven, Belgium
    
    Written by Daan Christiaens, 19/03/14.
    
    This file is part of the Global Tractography module for MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

*/


#include "command.h"

#include <limits>

#include "math/SH.h"
#include "image/buffer_preload.h"
#include "image/buffer_scratch.h"
#include "thread/exec.h"

#include "particlegrid.h"
#include "gt.h"
#include "externalenergy.h"
#include "internalenergy.h"
#include "mhsampler.h"


using namespace MR;
using namespace App;



void usage ()
{

  AUTHOR = "Daan Christiaens (daan.christiaens@esat.kuleuven.be)";
  
  COPYRIGHT = "KU Leuven, Dept. Electrical Engineering, Medical Image Computing,\n"
              "Herestraat 49 box 7003, 3000 Leuven, Belgium";

  DESCRIPTION
  + "perform global tractography.";

  ARGUMENTS
  + Argument ("source",
              "the image containing the raw DWI data."
             ).type_image_in()

  + Argument ("tracks", "the output file containing the tracks generated.").type_file();



  OPTIONS

  + Option ("grad",
            "specify the diffusion encoding scheme (if not supplied in the header)")
    + Argument ("scheme").type_file()

  + Option ("lmax",
            "set the maximum harmonic order for the output series. By default, the "
            "program will use the highest possible lmax given the number of "
            "diffusion-weighted images.")
    + Argument ("order").type_integer (2, 8, 30)

  + Option ("mask",
            "only reconstruct the tractogram within the specified brain mask image.")
    + Argument ("image").type_image_in()
  
  + Option ("length",
            "set the length of the particles (fibre segments).")
    + Argument ("size").type_float(1e-6, 1.0, 10.0)
      
  + Option ("density",
            "set the desired density of the free Poisson process.")
    + Argument ("lambda").type_float(0, 1., std::numeric_limits<float>::max())
      
  + Option ("weight",
            "set the weight by which particles contribute to the model.")
    + Argument ("w").type_float(1e-6, 0.1, 1.0)

  + Option ("cpot",
            "set the energy term that drives two segments together.")
    + Argument ("c").type_float(0, 1.0, 1e6)

  + Option ("wmr",
            "set the response of a single particle on the DWI signal.").required()
    + Argument ("response").type_file()

  + Option ("csfr",
            "set the response of CSF on the DWI signal.")
    + Argument ("response").type_file()
  
  + Option ("gmr",
            "set the response of GM on the DWI signal.")
    + Argument ("response").type_file()

  + Option ("riso",
            "set one or more isotropic response kernels.").allow_multiple()
    + Argument ("response").type_file()

  + Option ("balance",
            "set the balance between internal and external energy."
            "Negative values give more weight to the internal energy, positive to the external energy.")
    + Argument ("bal").type_float(-100, 0, 100)

  + Option ("prob",
            "set the probabilities of generating birth, death, randshift, optshift and connect probabilities respectively.")
    + Argument ("prob").type_sequence_float()

  + Option ("t0",
            "set the initial temperature of the metropolis hastings optimizer.")
    + Argument ("start").type_float(1e-6, 0.1, 1e6)

  + Option ("t1",
            "set the final temperature of the metropolis hastings optimizer.")
    + Argument ("end").type_float(1e-6, 0.001, 1e6)
      
  + Option ("niter",
            "set the number of iterations of the metropolis hastings optimizer.")
    + Argument ("n").type_integer(1, 1000000, std::numeric_limits<int>::max())
  
  + Option ("beta",
            "set the width of the Hanning interpolation window.")
    + Argument ("b").type_float(0.0, 0.1, 1.0)
  
  + Option ("lambda",
            "set the weight of a Tikhonov constraint on the no. segments.")
    + Argument ("t").type_float(0.0, 0.0, 1e3)

  + Option ("todi",
            "filename of the resulting TOD image.")
    + Argument ("todimage").type_image_out()

  + Option ("fiso",
            "filename of the resulting ISO fractions image.")
    + Argument ("iso").type_image_out()

  + Option ("eext",
            "filename of the resulting image of the residual external energy.")
    + Argument ("eext").type_image_out()
  
  + Option ("etrend",
            "internal and external energy trend and cooling statistics.")
    + Argument ("stats").type_file();


};


void launch_MHS(DWI::Tractography::GT::MHSampler& mhs) {
  Thread::Array<DWI::Tractography::GT::MHSampler> mhs_threaded (mhs);
  Thread::Exec exec (mhs_threaded, "MH sampler");
}


void run ()
{

  using namespace DWI::Tractography::GT;
  
  // Parse options ----------------------------------------------------------------------
  
  Options opt = get_options("length");
  if (opt.size())
    Particle::L = opt[0][0];    
  
  double ChemPot = 1.0;
  opt = get_options("cpot");
  if (opt.size())
    ChemPot = opt[0][0];
  
  Properties properties;
  properties.Lmax = 8;
  properties.p_birth = 0.25;
  properties.p_death = 0.05;
  properties.p_shift = 0.25;
  properties.p_optshift = 0.10;
  properties.p_connect = 0.35;
  properties.density = 1.;
  properties.weight = 0.1;
  properties.lam_ext = 1.;
  properties.lam_int = 1.;
  properties.beta = 0.1;
  properties.tikhonov = 0.0;
  
  opt = get_options("lmax");
  if (opt.size()) 
    properties.Lmax = opt[0][0];
  
  opt = get_options("density");
  if (opt.size()) 
    properties.density = opt[0][0];
  
  opt = get_options("weight");
  if (opt.size())
    properties.weight = opt[0][0];
  
  opt = get_options("wmr");
  Math::Matrix<float> wmr (opt[0][0]);
  properties.resp_WM = wmr;
  
  Math::Vector<float> riso;
  opt = get_options("csfr");
  if (opt.size())
  {
    riso.load(opt[0][0]);
    properties.resp_ISO.push_back(riso);
  }
  
  opt = get_options("gmr");
  if (opt.size())
  {
    riso.load(opt[0][0]);
    properties.resp_ISO.push_back(riso);
  }
  
  opt = get_options("riso");
  for (size_t i = 0; i < opt.size(); i++)
  {
    riso.load(opt[i][0]);
    properties.resp_ISO.push_back(riso);
  }

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
  std::vector<float> prob;
  if (opt.size())
  {
    prob = opt[0][0];
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
  
  
  Image::BufferPreload<bool>* mask = NULL;
  opt = get_options("mask");
  if (opt.size())
    mask = new Image::BufferPreload<bool>(opt[0][0]);
  
  
  int niter = 1000000;
  opt = get_options("niter");
  if (opt.size())
    niter = opt[0][0];
  
  double t0 = 0.1;
  opt = get_options("t0");
  if (opt.size())
    t0 = opt[0][0];
  
  double t1 = 0.001;
  opt = get_options("t1");
  if (opt.size())
    t1 = opt[0][0];
  
  opt = get_options("beta");
  if (opt.size())
    properties.beta = opt[0][0];
  
  opt = get_options("lambda");
  if (opt.size())
    properties.tikhonov = opt[0][0];


  // Prepare buffers --------------------------------------------------------------------
  
  //PAUSE;

  Image::BufferPreload<float> dwi_buffer (argument[0], Image::Stride::contiguous_along_axis(3));
  //Image::ConstInfo dwi_info (dwi_buffer);

  Stats stats (t0, t1, niter);
  
  opt = get_options("etrend");
  if (opt.size())
    stats.open_stream(opt[0][0]);
  
  ExternalEnergyComputer::Shared EextShared = ExternalEnergyComputer::Shared(dwi_buffer, properties);
  ExternalEnergyComputer* Eext = new ExternalEnergyComputer(stats, EextShared);
  
  ParticleGrid pgrid = ParticleGrid(dwi_buffer);
  
  InternalEnergyComputer* Eint = new InternalEnergyComputer(stats, pgrid);
  Eint->setChemPot(ChemPot);
  
  
  EnergySumComputer* Esum = new EnergySumComputer(stats, Eint, properties.lam_int, 
                                             Eext, properties.lam_ext * 4*M_PI/(properties.resp_WM(0,0)*properties.resp_WM(0,0)*properties.weight*properties.weight));
  
  
  MHSampler mhs (dwi_buffer, properties, stats, pgrid, Esum, mask);   // All EnergyComputers are recursively destroyed upon destruction of mhs, except for the shared data.
    
  launch_MHS(mhs);
  
  VAR(pgrid.getTotalCount());
  
  std::cout << stats << std::endl;
  
  
  // Copy results to output buffers -----------------------------------------------------
  
  // Save the tracks (output)
  INFO("Saving tracks to file");
  MR::DWI::Tractography::Properties ftfileprops;
  ftfileprops.comments.push_back("global tractography");
  ftfileprops.comments.push_back(MRTRIX_PROJECT_VERSION);
  ftfileprops.comments.push_back("");
  ftfileprops.comments.push_back("segment length = " + std::to_string((long double) Particle::L));
  ftfileprops.comments.push_back("segment density = " + std::to_string((long double) properties.density));
  ftfileprops.comments.push_back("segment weight = " + std::to_string((long double) properties.weight));
  ftfileprops.comments.push_back("");
  ftfileprops.comments.push_back("connection potential = " + std::to_string((long double) ChemPot));
  ftfileprops.comments.push_back("balance = " + std::to_string((long double) lam));
  ftfileprops.comments.push_back("");
  ftfileprops.comments.push_back("no. iterations = " + std::to_string((long long int) niter));
  ftfileprops.comments.push_back("T0 = " + std::to_string((long double) t0));
  ftfileprops.comments.push_back("T1 = " + std::to_string((long double) t1));
  
  MR::DWI::Tractography::Writer<float> writer (argument[1], ftfileprops);
  pgrid.exportTracks(writer);
  
  
  // Save fiso, tod and eext
  Image::Header header (dwi_buffer);
  header.datatype() = DataType::Float32;
  
  opt = get_options("todi");
  if (opt.size()) {
    header.dim(3) = Math::SH::NforL(properties.Lmax);
    Image::Buffer<float> TOD (opt[0][0], header);
    Image::Buffer<float>::voxel_type vox_out (TOD);
    Image::BufferScratch<float>::voxel_type vox_in (EextShared.getTOD());
    Image::copy_with_progress_message("copying TOD image", vox_in, vox_out);
  }
  
  opt = get_options("fiso");
  if (opt.size()) {
    header.dim(3) = properties.resp_ISO.size();
    Image::Buffer<float> Fiso (opt[0][0], header);
    Image::Buffer<float>::voxel_type vox_out (Fiso);
    Image::BufferScratch<float>::voxel_type vox_in (EextShared.getFiso());
    Image::copy_with_progress_message("copying isotropic fractions", vox_in, vox_out);
  }
  
  opt = get_options("eext");
  if (opt.size()) {
    header.set_ndim(3);
    Image::Buffer<float> EextI (opt[0][0], header);
    Image::Buffer<float>::voxel_type vox_out (EextI);
    Image::BufferScratch<float>::voxel_type vox_in (EextShared.getEext());
    Image::copy_with_progress_message("copying external energy", vox_in, vox_out);
  }
  
  


}

