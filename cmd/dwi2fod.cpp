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
#include "memory.h"
#include "progressbar.h"
#include "algo/threaded_loop.h"
#include "image.h"
#include "dwi/gradient.h"
#include "dwi/shells.h"
#include "dwi/sdeconv/constrained.h"

using namespace MR;
using namespace App;


void usage ()
{
  DESCRIPTION
    + "perform non-negativity constrained spherical deconvolution."

    + "Note that this program makes use of implied symmetries in the diffusion "
    "profile. First, the fact the signal attenuation profile is real implies "
    "that it has conjugate symmetry, i.e. Y(l,-m) = Y(l,m)* (where * denotes "
    "the complex conjugate). Second, the diffusion profile should be "
    "antipodally symmetric (i.e. S(x) = S(-x)), implying that all odd l "
    "components should be zero. Therefore, this program only computes the even "
    "elements."

    + "Note that the spherical harmonics equations used here differ slightly "
    "from those conventionally used, in that the (-1)^m factor has been "
    "omitted. This should be taken into account in all subsequent calculations."

    + Math::SH::encoding_description;

  REFERENCES 
   + "Tournier, J.-D.; Calamante, F. & Connelly, A. " // Internal
   "Robust determination of the fibre orientation distribution in diffusion MRI: "
   "Non-negativity constrained super-resolved spherical deconvolution. "
   "NeuroImage, 2007, 35, 1459-1472"

   + "Tournier, J.-D.; Calamante, F., Gadian, D.G. & Connelly, A. " // Internal
   "Direct estimation of the fiber orientation density function from "
   "diffusion-weighted MRI data using spherical deconvolution."
   "NeuroImage, 2004, 23, 1176-1185";

  ARGUMENTS
    + Argument ("dwi",
        "the input diffusion-weighted image.").type_image_in()
    + Argument ("response",
        "a text file containing the diffusion-weighted signal response function "
        "coefficients for a single fibre population, ").type_file_in()
    + Argument ("SH",
        "the output spherical harmonics coefficients image.").type_image_out();

  OPTIONS
    + DWI::GradImportOptions()
    + DWI::ShellOption
    + DWI::CSD_options
    + Stride::Options;
}



typedef float value_type;
typedef double cost_value_type;





class Processor
{
  public:
    Processor (const DWI::CSDeconv::Shared& shared, Image<bool>& mask) :
      sdeconv (shared),
      data (shared.dwis.size()),
      mask (mask) { }

    template <class DWIType, class FODType>
    void operator () (DWIType& dwi, FODType& fod) {
      if (!load_data (dwi)) {
        for (auto l = Loop (3) (fod); l; ++l)
          fod.value() = 0.0;
        return;
      }

      sdeconv.set (data);

      size_t n;
      for (n = 0; n < sdeconv.shared.niter; n++)
        if (sdeconv.iterate())
          break;

      if (sdeconv.shared.niter && n >= sdeconv.shared.niter)
        INFO ("voxel [ " + str (dwi.index(0)) + " " + str (dwi.index(1)) + " " + str (dwi.index(2)) +
            " ] did not reach full convergence");

      write_back (fod);
    }



  private:
    DWI::CSDeconv sdeconv;
    Eigen::VectorXd data;
    Image<bool> mask;


    template <class DWIType>
      bool load_data (DWIType& dwi) {
        if (mask.valid()) {
          assign_pos_of (dwi, 0, 3).to (mask);
          if (!mask.value())
            return false;
        }

        for (size_t n = 0; n < sdeconv.shared.dwis.size(); n++) {
          dwi.index(3) = sdeconv.shared.dwis[n];
          data[n] = dwi.value();
          if (!std::isfinite (data[n]))
            return false;
          if (data[n] < 0.0)
            data[n] = 0.0;
        }

        return true;
      }



    template <class FODType> 
      void write_back (FODType& fod) {
        for (auto l = Loop (3) (fod); l; ++l)
          fod.value() = sdeconv.FOD() [fod.index(3)];
      }

};







void run ()
{
  auto dwi = Image<value_type>::open (argument[0]).with_direct_io (3);

  auto mask = Image<bool>();

  auto opt = get_options ("mask");
  if (opt.size()) {
    mask = Header::open (opt[0][0]).get_image<bool>();
    check_dimensions (dwi, mask, 0, 3);
  }


  DWI::CSDeconv::Shared shared (dwi);
  shared.parse_cmdline_options();
  shared.set_response (argument[1]);
  shared.init();


  auto header = Header(dwi);
  header.set_ndim (4);
  header.size(3) = shared.nSH();
  header.datatype() = DataType::Float32;
  Stride::set_from_command_line (header);
  auto fod = Image<value_type>::create (argument[2], header);

  Processor processor (shared, mask);
  ThreadedLoop ("performing constrained spherical deconvolution", dwi, 0, 3)
    .run (processor, dwi, fod);
}

