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
#include "progressbar.h"

#include "image.h"

#include "sparse/fixel_metric.h"
#include "sparse/keys.h"
#include "sparse/image.h"

#include "math/SH.h"

#include "thread_queue.h"

#include "dwi/fmls.h"
#include "dwi/directions/set.h"




using namespace MR;
using namespace MR::DWI;
using namespace MR::DWI::FMLS;
using namespace App;



using Sparse::FixelMetric;



const OptionGroup OutputOptions = OptionGroup ("Metric values for fixel-based sparse output images")

  + Option ("afd",
            "store the total Apparent Fibre Density per fixel (integral of FOD lobe)")
    + Argument ("image").type_image_out()

  + Option ("peak",
            "store the peak FOD amplitude per fixel")
    + Argument ("image").type_image_out()

  + Option ("disp",
            "store a measure of dispersion per fixel as the ratio between FOD lobe integral and peak amplitude")
    + Argument ("image").type_image_out();




void usage ()
{

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  DESCRIPTION
  + "use a fast-marching level-set method to segment fibre orientation distributions, and save parameters of interest as fixel images";

  REFERENCES 
    + "* Reference for the FOD segmentation method:\n"
    "Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. " // Internal
    "SIFT: Spherical-deconvolution informed filtering of tractograms. "
    "NeuroImage, 2013, 67, 298-312 (Appendix 2)"

    + "* Reference for Apparent Fibre Density:\n"
    "Raffelt, D.; Tournier, J.-D.; Rose, S.; Ridgway, G.R.; Henderson, R.; Crozier, S.; Salvado, O.; Connelly, A. " // Internal
    "Apparent Fibre Density: a novel measure for the analysis of diffusion-weighted magnetic resonance images."
    "Neuroimage, 2012, 15;59(4), 3976-94.";

  ARGUMENTS
  + Argument ("fod", "the input fod image.").type_image_in ();


  OPTIONS

  + Option ("mask",
            "only perform computation within the specified binary brain mask image.")
    + Argument ("image").type_image_in()

  + OutputOptions

  + FMLSSegmentOption;

}







class Segmented_FOD_receiver
{

  public:
    Segmented_FOD_receiver (const Header& header) :
        H (header)
    {
      H.ndim() = 3;
      H.datatype() = DataType::UInt64;
      H.datatype().set_byte_order_native();
      H.keyval()[Sparse::name_key] = str(typeid(FixelMetric).name());
      H.keyval()[Sparse::size_key] = str(sizeof(FixelMetric));
    }


    void set_afd_output  (const std::string&);
    void set_peak_output (const std::string&);
    void set_disp_output (const std::string&);
    size_t num_outputs() const;

    bool operator() (const FOD_lobes&);



  private:
    Header H;

    // These must be stored using pointers as Sparse::Image
    //   uses class constructors rather than static functions;
    //   the Sparse::Image class should be modified
    std::unique_ptr<Sparse::Image<FixelMetric>> afd, peak, disp;
};




void Segmented_FOD_receiver::set_afd_output (const std::string& path)
{
  assert (!afd);
  afd.reset (new Sparse::Image<FixelMetric> (path, H));
}

void Segmented_FOD_receiver::set_peak_output (const std::string& path)
{
  assert (!peak);
  peak.reset (new Sparse::Image<FixelMetric> (path, H));
}

void Segmented_FOD_receiver::set_disp_output (const std::string& path)
{
  assert (!disp);
  disp.reset (new Sparse::Image<FixelMetric> (path, H));
}

size_t Segmented_FOD_receiver::num_outputs() const
{
  size_t count = 0;
  if (afd)  ++count;
  if (peak) ++count;
  if (disp) ++count;
  return count;
}






bool Segmented_FOD_receiver::operator() (const FOD_lobes& in)
{
  if (!in.size())
    return true;

  if (afd) {
    assign_pos_of (in.vox).to (*afd);
    afd->value().set_size (in.size());
    for (size_t i = 0; i != in.size(); ++i) {
      FixelMetric this_fixel (in[i].get_mean_dir(), in[i].get_integral(), in[i].get_integral());
      afd->value()[i] = this_fixel;
    }
  }

  if (peak) {
    assign_pos_of (in.vox).to (*peak);
    peak->value().set_size (in.size());
    for (size_t i = 0; i != in.size(); ++i) {
      FixelMetric this_fixel (in[i].get_peak_dir(0), in[i].get_integral(), in[i].get_max_peak_value());
      peak->value()[i] = this_fixel;
    }
  }

  if (disp) {
    assign_pos_of (in.vox).to (*disp);
    disp->value().set_size (in.size());
    for (size_t i = 0; i != in.size(); ++i) {
      FixelMetric this_fixel (in[i].get_mean_dir(), in[i].get_integral(), in[i].get_integral() / in[i].get_max_peak_value());
      disp->value()[i] = this_fixel;
    }
  }

  return true;
}





void run ()
{
  Header H = Header::open (argument[0]);
  Math::SH::check (H);
  auto fod_data = H.get_image<float>();

  Segmented_FOD_receiver receiver (H);

  auto
  opt = get_options ("afd");  if (opt.size()) receiver.set_afd_output  (opt[0][0]);
  opt = get_options ("peak"); if (opt.size()) receiver.set_peak_output (opt[0][0]);
  opt = get_options ("disp"); if (opt.size()) receiver.set_disp_output (opt[0][0]);
  if (!receiver.num_outputs())
    throw Exception ("Nothing to do; please specify at least one output image type");

  opt = get_options ("mask");
  Image<float> mask;
  if (opt.size()) {
    mask = Image<float>::open (std::string (opt[0][0]));
    if (!dimensions_match (fod_data, mask, 0, 3))
      throw Exception ("Cannot use image \"" + str(opt[0][0]) + "\" as mask image; dimensions do not match FOD image");
  }

  FMLS::FODQueueWriter writer (fod_data, mask);

  const DWI::Directions::Set dirs (1281);
  Segmenter fmls (dirs, Math::SH::LforN (H.size(3)));
  load_fmls_thresholds (fmls);

  Thread::run_queue (writer, Thread::batch (SH_coefs()), Thread::multi (fmls), Thread::batch (FOD_lobes()), receiver);
}

