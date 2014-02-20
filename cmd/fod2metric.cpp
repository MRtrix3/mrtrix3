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

#include "thread/queue.h"

#include "dwi/fmls.h"
#include "dwi/directions/set.h"




using namespace MR;
using namespace MR::DWI;
using namespace MR::DWI::FMLS;
using namespace App;



using Image::Sparse::FixelMetric;



const OptionGroup ScalarOutputOptions = OptionGroup ("Scalar output image options")

  + Option ("afd",
            "compute the sum of per-fixel Apparent Fibre Density in each voxel")
    + Argument ("image").type_image_out()

  + Option ("count",
            "compute the number of discrete fibre populations in each voxel")
    + Argument ("image").type_image_out()

  + Option ("dec",
            "compute a directionally-encoded colour map of fibre population densities")
    + Argument ("image").type_image_out()

  + Option ("gfa",
            "compute a Generalised Fractional Anisotropy image (does not require FOD segmentation)")
    + Argument ("image").type_image_out()

  + Option ("pseudo_fod",
            "compute a pseudo-FOD image in the SH basis, showing the orientations & relative amplitudes of segmented fibre populations (useful for assessing segmentation performance until sparse image format is implemented)")
    + Argument ("image").type_image_out()

  + Option ("sf",
            "compute the fraction of AFD in the voxel that is attributed to the largest FOD lobe, i.e. \"single fibre\" nature of voxels")
    + Argument ("image").type_image_out();




const OptionGroup FixelOutputOptions = OptionGroup ("Fixel-based sparse output image options")

  + Option ("fixel_afd",
            "compute the Apparent Fibre Density per fixel")
    + Argument ("image").type_image_out()

  + Option ("fixel_peak",
            "compute the peak amplitude per fixel")
    + Argument ("image").type_image_out()

  + Option ("fixel_disp",
            "compute a measure of dispersion per fixel as the ratio between FOD lobe integral and peak")
    + Argument ("image").type_image_out();




void usage ()
{
  DESCRIPTION
  + "generate parameter maps from fibre orientation distributions using the fast-marching level-set segmenter.";

  ARGUMENTS
  + Argument ("fod", "the input fod image.").type_image_in ();


  OPTIONS

  + Option ("mask",
            "only perform computation within the specified binary brain mask image.")
    + Argument ("image").type_image_in()

  + ScalarOutputOptions

  + FixelOutputOptions

  + FMLSSegmentOption;

}







class Segmented_FOD_receiver
{

  public:
    Segmented_FOD_receiver (const Image::Header& header, const DWI::Directions::Set& directions) :
      H (header),
      H_fixel (header),
      dirs (directions),
      lmax (0)
    {
      // aPSF does not have data for lmax > 10
      lmax = std::min (size_t(10), Math::SH::LforN (header.dim(3)));
      H.set_ndim (3);
      H.DW_scheme().clear();
      H_fixel.set_ndim (3);
      H_fixel.DW_scheme().clear();
      H_fixel.datatype() = DataType::UInt64;
      H_fixel.datatype().set_byte_order_native();
      H_fixel[Image::Sparse::name_key] = str(typeid(FixelMetric).name());
      H_fixel[Image::Sparse::size_key] = str(sizeof(FixelMetric));
    }


    void set_afd_output        (const std::string&);
    void set_count_output      (const std::string&);
    void set_dec_output        (const std::string&);
    void set_gfa_output        (const std::string&);
    void set_pseudo_fod_output (const std::string&);
    void set_sf_output         (const std::string&);

    void set_fixel_afd_output  (const std::string&);
    void set_fixel_peak_output (const std::string&);
    void set_fixel_disp_output (const std::string&);


    bool operator() (const FOD_lobes&);



  private:
    Image::Header H, H_fixel;
    const DWI::Directions::Set& dirs;
    size_t lmax;

    Ptr< Image::Buffer<float> > afd_data;
    Ptr< Image::Buffer<float>::voxel_type > afd;
    Ptr< Image::Buffer<uint8_t> > count_data;
    Ptr< Image::Buffer<uint8_t>::voxel_type > count;
    Ptr< Image::Buffer<float> > dec_data;
    Ptr< Image::Buffer<float>::voxel_type > dec;
    Ptr< Image::Buffer<float> > gfa_data;
    Ptr< Image::Buffer<float>::voxel_type > gfa;
    Ptr< Image::Buffer<float> > pseudo_fod_data;
    Ptr< Image::Buffer<float>::voxel_type > pseudo_fod;
    Ptr< Image::Buffer<float> > sf_data;
    Ptr< Image::Buffer<float>::voxel_type > sf;


    Ptr< Image::BufferSparse<FixelMetric> > fixel_afd_data;
    Ptr< Image::BufferSparse<FixelMetric>::voxel_type > fixel_afd;
    Ptr< Image::BufferSparse<FixelMetric> > fixel_peak_data;
    Ptr< Image::BufferSparse<FixelMetric>::voxel_type > fixel_peak;
    Ptr< Image::BufferSparse<FixelMetric> > fixel_disp_data;
    Ptr< Image::BufferSparse<FixelMetric>::voxel_type > fixel_disp;


};




void Segmented_FOD_receiver::set_afd_output (const std::string& path)
{
  assert (!afd_data);
  afd_data = new Image::Buffer<float> (path, H);
  afd = new Image::Buffer<float>::voxel_type (*afd_data);
}

void Segmented_FOD_receiver::set_count_output (const std::string& path)
{
  assert (!count_data);
  Image::Header H_count (H);
  H_count.datatype() = DataType::UInt8;
  count_data = new Image::Buffer<uint8_t> (path, H_count);
  count = new Image::Buffer<uint8_t>::voxel_type (*count_data);
}

void Segmented_FOD_receiver::set_dec_output (const std::string& path)
{
  assert (!dec_data);
  Image::Header H_dec (H);
  H_dec.set_ndim (4);
  H_dec.dim(3) = 3;
  dec_data = new Image::Buffer<float> (path, H_dec);
  dec = new Image::Buffer<float>::voxel_type (*dec_data);
}

void Segmented_FOD_receiver::set_gfa_output (const std::string& path)
{
  assert (!gfa_data);
  gfa_data = new Image::Buffer<float> (path, H);
  gfa = new Image::Buffer<float>::voxel_type (*gfa_data);
}

void Segmented_FOD_receiver::set_pseudo_fod_output (const std::string& path)
{
  assert (!pseudo_fod_data);
  Image::Header H_pseudo_fod (H);
  H_pseudo_fod.set_ndim (4);
  H_pseudo_fod.dim(3) = Math::SH::NforL (lmax);
  pseudo_fod_data = new Image::Buffer<float> (path, H_pseudo_fod);
  pseudo_fod = new Image::Buffer<float>::voxel_type (*pseudo_fod_data);
}

void Segmented_FOD_receiver::set_sf_output (const std::string& path)
{
  assert (!sf_data);
  sf_data = new Image::Buffer<float> (path, H);
  sf = new Image::Buffer<float>::voxel_type (*sf_data);
}




void Segmented_FOD_receiver::set_fixel_afd_output (const std::string& path)
{
  assert (!fixel_afd_data);
  fixel_afd_data = new Image::BufferSparse<FixelMetric> (path, H_fixel);
  fixel_afd = new Image::BufferSparse<FixelMetric>::voxel_type (*fixel_afd_data);
  Image::LoopInOrder loop (*fixel_afd);
  for (loop.start (*fixel_afd); loop.ok(); loop.next (*fixel_afd))
    fixel_afd->value().zero();
}

void Segmented_FOD_receiver::set_fixel_peak_output (const std::string& path)
{
  assert (!fixel_peak_data);
  fixel_peak_data = new Image::BufferSparse<FixelMetric> (path, H_fixel);
  fixel_peak = new Image::BufferSparse<FixelMetric>::voxel_type (*fixel_peak_data);
  Image::LoopInOrder loop (*fixel_peak);
  for (loop.start (*fixel_peak); loop.ok(); loop.next (*fixel_peak))
    fixel_peak->value().zero();
}

void Segmented_FOD_receiver::set_fixel_disp_output (const std::string& path)
{
  assert (!fixel_disp_data);
  fixel_disp_data = new Image::BufferSparse<FixelMetric> (path, H_fixel);
  fixel_disp = new Image::BufferSparse<FixelMetric>::voxel_type (*fixel_disp_data);
  Image::LoopInOrder loop (*fixel_disp);
  for (loop.start (*fixel_disp); loop.ok(); loop.next (*fixel_disp))
    fixel_disp->value().zero();
}







bool Segmented_FOD_receiver::operator() (const FOD_lobes& in)
{

  if (afd) {
    float sum_integrals = 0.0;
    for (FOD_lobes::const_iterator i = in.begin(); i != in.end(); ++i)
      sum_integrals += i->get_integral();
    Image::Nav::set_value_at_pos (*afd, in.vox, sum_integrals);
  }

  if (count)
    Image::Nav::set_value_at_pos (*count, in.vox, in.size());

  if (dec) {
    Point<float> sum_decs (0.0, 0.0, 0.0);
    for (FOD_lobes::const_iterator i = in.begin(); i != in.end(); ++i)
      sum_decs += Point<float> (Math::abs(i->get_mean_dir()[0]), Math::abs(i->get_mean_dir()[1]), Math::abs(i->get_mean_dir()[2])) * i->get_integral();
    Image::Nav::set_pos (*dec, in.vox);
    (*dec)[3] = 0; dec->value() = sum_decs[0];
    (*dec)[3] = 1; dec->value() = sum_decs[1];
    (*dec)[3] = 2; dec->value() = sum_decs[2];
  }

  if (gfa) {
    double sum = 0.0;
    std::vector<float> combined_values (dirs.size(), 0.0);
    for (FOD_lobes::const_iterator i = in.begin(); i != in.end(); ++i) {
      const std::vector<float>& values = i->get_values();
      for (size_t j = 0; j != dirs.size(); ++j) {
        sum += values[j];
        combined_values[j] += values[j];
      }
    }
    if (sum) {
      const float fod_normaliser = 1.0 / sum;
      const float normalised_mean = 1.0 / float(dirs.size());
      double sum_variance = 0.0, sum_of_squares = 0.0;
      for (size_t i = 0; i != dirs.size(); ++i) {
        sum_variance   += Math::pow2((combined_values[i] * fod_normaliser) - normalised_mean);
        sum_of_squares += Math::pow2 (combined_values[i] * fod_normaliser);
      }
      const float mean_variance = sum_variance   / double(dirs.size() - 1);
      const float mean_square   = sum_of_squares / double(dirs.size());
      const float value = Math::sqrt (mean_variance / mean_square);
      Image::Nav::set_value_at_pos (*gfa, in.vox, value);
    }
  }

  if (pseudo_fod) {
    Image::Nav::set_pos (*pseudo_fod, in.vox);
    Math::Vector<float> sum_pseudo_fod;
    sum_pseudo_fod.resize (Math::SH::NforL (lmax), 0.0);
    Math::SH::aPSF<float> aPSF (lmax);
    for (FOD_lobes::const_iterator i = in.begin(); i != in.end(); ++i) {
      Math::Vector<float> this_lobe;
      aPSF (this_lobe, i->get_mean_dir());
      for (size_t c = 0; c != sum_pseudo_fod.size(); ++c)
        sum_pseudo_fod[c] += i->get_integral() * this_lobe[c];
    }
    for (size_t c = 0; c != sum_pseudo_fod.size(); ++c) {
      (*pseudo_fod)[3] = c;
      pseudo_fod->value() = sum_pseudo_fod[c];
    }
  }

  if (sf) {
    float sum = 0.0, maximum = 0.0;
    for (FOD_lobes::const_iterator i = in.begin(); i != in.end(); ++i) {
      sum += i->get_integral();
      maximum = std::max (maximum, i->get_integral());
    }
    const float value = sum ? (maximum / sum) : 0.0;
    Image::Nav::set_value_at_pos (*sf, in.vox, value);
  }



  if (fixel_afd && in.size()) {
    Image::Nav::set_pos (*fixel_afd, in.vox);
    fixel_afd->value().set_size (in.size());
    for (size_t i = 0; i != in.size(); ++i) {
      FixelMetric this_fixel (in[i].get_mean_dir(), in[i].get_integral(), in[i].get_integral());
      (*fixel_afd).value()[i] = this_fixel;
    }
  }

  if (fixel_peak && in.size()) {
    Image::Nav::set_pos (*fixel_peak, in.vox);
    fixel_peak->value().set_size (in.size());
    for (size_t i = 0; i != in.size(); ++i) {
      FixelMetric this_fixel (in[i].get_peak_dir(), in[i].get_integral(), in[i].get_peak_value());
      (*fixel_peak).value()[i] = this_fixel;
    }
  }

  if (fixel_disp && in.size()) {
    Image::Nav::set_pos (*fixel_disp, in.vox);
    fixel_disp->value().set_size (in.size());
    for (size_t i = 0; i != in.size(); ++i) {
      FixelMetric this_fixel (in[i].get_mean_dir(), in[i].get_integral(), in[i].get_integral() / in[i].get_peak_value());
      (*fixel_disp).value()[i] = this_fixel;
    }
  }


  return true;

}





void run ()
{
  Image::Header H (argument[0]);
  Image::Buffer<float> fod_data (H);

  if (fod_data.ndim() != 4)
    throw Exception ("input FOD image should contain 4 dimensions");

  const size_t lmax = Math::SH::LforN (fod_data.dim(3));

  if (Math::SH::NforL (lmax) != size_t(fod_data.dim(3)))
    throw Exception ("Input image does not appear to contain an SH series per voxel");

  const DWI::Directions::Set dirs (1281);
  Segmented_FOD_receiver receiver (H, dirs);

  size_t output_count = 0;
  Options opt = get_options ("afd");
  if (opt.size()) {
    receiver.set_afd_output (opt[0][0]);
    ++output_count;
  }
  opt = get_options ("count");
  if (opt.size()) {
    receiver.set_count_output (opt[0][0]);
    ++output_count;
  }
  opt = get_options ("dec");
  if (opt.size()) {
    receiver.set_dec_output (opt[0][0]);
    ++output_count;
  }
  opt = get_options ("gfa");
  if (opt.size()) {
    receiver.set_gfa_output (opt[0][0]);
    ++output_count;
  }
  opt = get_options ("pseudo_fod");
  if (opt.size()) {
    receiver.set_pseudo_fod_output (opt[0][0]);
    ++output_count;
  }
  opt = get_options ("sf");
  if (opt.size()) {
    receiver.set_sf_output (opt[0][0]);
    ++output_count;
  }
  opt = get_options ("fixel_afd");
  if (opt.size()) {
    receiver.set_fixel_afd_output (opt[0][0]);
    ++output_count;
  }
  opt = get_options ("fixel_peak");
  if (opt.size()) {
    receiver.set_fixel_peak_output (opt[0][0]);
    ++output_count;
  }
  opt = get_options ("fixel_disp");
  if (opt.size()) {
    receiver.set_fixel_disp_output (opt[0][0]);
    ++output_count;
  }
  if (!output_count)
    throw Exception ("Nothing to do; please specify at least one output image type");

  FMLS::FODQueueWriter<Image::Buffer<float>::voxel_type> writer (fod_data);

  opt = get_options ("mask");
  Ptr<Image::Buffer<bool> > mask_buffer_ptr;
  if (opt.size()) {
    mask_buffer_ptr = new Image::Buffer<bool> (std::string (opt[0][0]));
    if (!Image::dimensions_match (fod_data, *mask_buffer_ptr, 0, 3))
      throw Exception ("Cannot use image \"" + str(opt[0][0]) + "\" as mask image; dimensions do not match FOD image");
    writer.set_mask (*mask_buffer_ptr);
  }

  Segmenter fmls (dirs, lmax);
  load_fmls_thresholds (fmls);

  Thread::run_queue (writer, SH_coefs(), Thread::multi (fmls), FOD_lobes(), receiver);

}

