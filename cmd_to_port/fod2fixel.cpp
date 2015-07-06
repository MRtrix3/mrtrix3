/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 2013.

    This file is part of MRtrix.

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
#include "progressbar.h"

#include "image/buffer.h"
#include "image/buffer_sparse.h"
#include "image/nav.h"
#include "image/utils.h"
#include "image/voxel.h"

#include "image/sparse/fixel_metric.h"
#include "image/sparse/keys.h"
#include "image/sparse/voxel.h"

#include "math/matrix.h"
#include "math/SH.h"
#include "math/vector.h"

#include "thread_queue.h"

#include "dwi/fmls.h"
#include "dwi/directions/set.h"




using namespace MR;
using namespace MR::DWI;
using namespace MR::DWI::FMLS;
using namespace App;



using Image::Sparse::FixelMetric;



const OptionGroup OutputOptions = OptionGroup ("Metric values for fixel-based sparse output images")

  + Option ("afd",
            "store the total Apparent Fibre Density per fixel (integral of FOD lobe)")
    + Argument ("image").type_image_out()

  + Option ("peak",
            "store the peak FOD amplitude per fixel")
    + Argument ("image").type_image_out()

  + Option ("disp",
            "store a measure of dispersion per fixel as the ratio between FOD lobe integral and peak")
    + Argument ("image").type_image_out();




void usage ()
{

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  DESCRIPTION
  + "use a fast-marching level-set method to segment fibre orientation distributions, and save parameters of interest as fixel images";

  REFERENCES 
    + "Reference for the FOD segmentation method:\n"
    "Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. "
    "SIFT: Spherical-deconvolution informed filtering of tractograms. "
    "NeuroImage, 2013, 67, 298-312 (Appendix 2)";

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
    Segmented_FOD_receiver (const Image::Info& info)
    {
      H.info() = info;
      H.set_ndim (3);
      H.DW_scheme().clear();
      H.datatype() = DataType::UInt64;
      H.datatype().set_byte_order_native();
      H[Image::Sparse::name_key] = str(typeid(FixelMetric).name());
      H[Image::Sparse::size_key] = str(sizeof(FixelMetric));
    }


    void set_afd_output  (const std::string&);
    void set_peak_output (const std::string&);
    void set_disp_output (const std::string&);
    size_t num_outputs() const;

    bool operator() (const FOD_lobes&);



  private:
    Image::Header H;

    std::unique_ptr<Image::BufferSparse<FixelMetric>> afd_data;
    std::unique_ptr<Image::BufferSparse<FixelMetric>::voxel_type> afd;
    std::unique_ptr<Image::BufferSparse<FixelMetric>> peak_data;
    std::unique_ptr<Image::BufferSparse<FixelMetric>::voxel_type> peak;
    std::unique_ptr<Image::BufferSparse<FixelMetric>> disp_data;
    std::unique_ptr<Image::BufferSparse<FixelMetric>::voxel_type> disp;
};




void Segmented_FOD_receiver::set_afd_output (const std::string& path)
{
  assert (!afd_data);
  afd_data.reset (new Image::BufferSparse<FixelMetric> (path, H));
  afd.reset (new Image::BufferSparse<FixelMetric>::voxel_type (*afd_data));
}

void Segmented_FOD_receiver::set_peak_output (const std::string& path)
{
  assert (!peak_data);
  peak_data.reset (new Image::BufferSparse<FixelMetric> (path, H));
  peak.reset (new Image::BufferSparse<FixelMetric>::voxel_type (*peak_data));
}

void Segmented_FOD_receiver::set_disp_output (const std::string& path)
{
  assert (!disp_data);
  disp_data.reset (new Image::BufferSparse<FixelMetric> (path, H));
  disp.reset (new Image::BufferSparse<FixelMetric>::voxel_type (*disp_data));
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

  if (afd && in.size()) {
    Image::Nav::set_pos (*afd, in.vox);
    afd->value().set_size (in.size());
    for (size_t i = 0; i != in.size(); ++i) {
      FixelMetric this_fixel (in[i].get_mean_dir(), in[i].get_integral(), in[i].get_integral());
      (*afd).value()[i] = this_fixel;
    }
  }

  if (peak && in.size()) {
    Image::Nav::set_pos (*peak, in.vox);
    peak->value().set_size (in.size());
    for (size_t i = 0; i != in.size(); ++i) {
      FixelMetric this_fixel (in[i].get_peak_dir(), in[i].get_integral(), in[i].get_peak_value());
      (*peak).value()[i] = this_fixel;
    }
  }

  if (disp && in.size()) {
    Image::Nav::set_pos (*disp, in.vox);
    disp->value().set_size (in.size());
    for (size_t i = 0; i != in.size(); ++i) {
      FixelMetric this_fixel (in[i].get_mean_dir(), in[i].get_integral(), in[i].get_integral() / in[i].get_peak_value());
      (*disp).value()[i] = this_fixel;
    }
  }


  return true;

}





void run ()
{
  Image::Header H (argument[0]);
  Math::SH::check (H);
  Image::Buffer<float> fod_data (H);

  Segmented_FOD_receiver receiver (H);

  Options
  opt = get_options ("afd");  if (opt.size()) receiver.set_afd_output  (opt[0][0]);
  opt = get_options ("peak"); if (opt.size()) receiver.set_peak_output (opt[0][0]);
  opt = get_options ("disp"); if (opt.size()) receiver.set_disp_output (opt[0][0]);
  if (!receiver.num_outputs())
    throw Exception ("Nothing to do; please specify at least one output image type");

  FMLS::FODQueueWriter<Image::Buffer<float>::voxel_type> writer (fod_data);

  opt = get_options ("mask");
  std::unique_ptr<Image::Buffer<bool> > mask_buffer_ptr;
  if (opt.size()) {
    mask_buffer_ptr.reset (new Image::Buffer<bool> (std::string (opt[0][0])));
    if (!Image::dimensions_match (fod_data, *mask_buffer_ptr, 0, 3))
      throw Exception ("Cannot use image \"" + str(opt[0][0]) + "\" as mask image; dimensions do not match FOD image");
    writer.set_mask (*mask_buffer_ptr);
  }

  const DWI::Directions::Set dirs (1281);
  Segmenter fmls (dirs, Math::SH::LforN (H.dim(3)));
  load_fmls_thresholds (fmls);

  Thread::run_queue (writer, SH_coefs(), Thread::multi (fmls), FOD_lobes(), receiver);

}

