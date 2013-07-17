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

#include "app.h"
#include "progressbar.h"

#include "image/buffer.h"
#include "image/nav.h"
#include "image/utils.h"
#include "image/voxel.h"

#include "math/matrix.h"
#include "math/SH.h"
#include "math/vector.h"

#include "thread/queue.h"

#include "dwi/fmls.h"
#include "dwi/fod_map.h"
#include "dwi/directions/set.h"



MRTRIX_APPLICATION

using namespace MR;
using namespace MR::DWI;
using namespace MR::DWI::FMLS;
using namespace App;



void usage ()
{
  DESCRIPTION
  + "generate parameter maps from fibre orientation distributions using the fast-marching level-set segmenter.";

  ARGUMENTS
  + Argument ("fod", "the input fod image.").type_image_in ();


  OPTIONS
  + Option ("afd",
            "compute the sum of per-dixel Apparent Fibre Density in each voxel")
    + Argument ("image").type_image_out()

  + Option ("count",
            "compute the number of discrete fibre populations in each voxel")
    + Argument ("image").type_image_out()

  + Option ("dec",
            "compute a directionally-encoded colour map of fibre population densities")
    + Argument ("image").type_image_out()

  + Option ("dixel_sh",
            "compute a SH image showing the orientations & relative amplitudes of segmented fibre populations (useful for assessing segmentation performance until sparse image format is implemented)")
    + Argument ("image").type_image_out()

  + Option ("gfa",
            "compute a Generalised Fractional Anisotropy image (does not require FOD segmentation)")
    + Argument ("image").type_image_out()

  + Option ("sf",
            "compute the fraction of AFD in the voxel that is attributed to the largest FOD lobe, i.e. \"single fibre\" nature of voxels")
    + Argument ("image").type_image_out()

  + Option ("mask",
            "only perform computation within the specified binary brain mask image.")
    + Argument ("image").type_image_in()


  + FMLSSegmentOption;

}




class FOD_queue_writer
{

    typedef Image::Buffer<float> FODBufferType;
    typedef Image::Buffer<bool > MaskBufferType;

  public:
    FOD_queue_writer (FODBufferType& fod_buffer) :
        fod_vox (fod_buffer),
        loop ("Segmenting FODs...", 0, 3)
    {
      loop.start (fod_vox);
    }


    void set_mask (const std::string& path)
    {
      assert (!mask_buffer);
      mask_buffer = new MaskBufferType (path);
      mask_vox = new MaskBufferType::voxel_type (*mask_buffer);
    }


    bool operator () (SH_coefs& out)
    {
      if (!loop.ok())
        return false;
      if (mask_vox) {
        do {
          Image::Nav::set_pos (*mask_vox, fod_vox);
          if (!mask_vox->value())
            loop.next (fod_vox);
        } while (loop.ok() && !mask_vox->value());
      }
      out.vox[0] = fod_vox[0]; out.vox[1] = fod_vox[1]; out.vox[2] = fod_vox[2];
      out.allocate (fod_vox.dim (3));
      for (fod_vox[3] = 0; fod_vox[3] != fod_vox.dim (3); ++fod_vox[3])
        out[fod_vox[3]] = fod_vox.value();
      loop.next (fod_vox);
      return true;
    }


  private:
    FODBufferType::voxel_type fod_vox;
    Ptr< MaskBufferType > mask_buffer;
    Ptr< MaskBufferType::voxel_type > mask_vox;
    Image::Loop loop;

};





class Segmented_FOD_receiver
{

  public:
    Segmented_FOD_receiver (const Image::Header& header, const DWI::Directions::Set& directions) :
      H (header),
      dirs (directions),
      lmax (0)
    {
      // aPSF does not have data for lmax > 10
      lmax = std::min (size_t(10), Math::SH::LforN (header.dim(3)));
      H.set_ndim (3);
    }


    void set_afd_output      (const std::string&);
    void set_count_output    (const std::string&);
    void set_dec_output      (const std::string&);
    void set_dixel_sh_output (const std::string&);
    void set_gfa_output      (const std::string&);
    void set_sf_output       (const std::string&);


    bool operator() (const FOD_lobes&);



  private:
    Image::Header H;
    const DWI::Directions::Set& dirs;
    size_t lmax;

    Ptr< Image::Buffer<float> > afd_data;
    Ptr< Image::Buffer<float>::voxel_type > afd;
    Ptr< Image::Buffer<uint8_t> > count_data;
    Ptr< Image::Buffer<uint8_t>::voxel_type > count;
    Ptr< Image::Buffer<float> > dec_data;
    Ptr< Image::Buffer<float>::voxel_type > dec;
    Ptr< Image::Buffer<float> > dixel_sh_data;
    Ptr< Image::Buffer<float>::voxel_type > dixel_sh;
    Ptr< Image::Buffer<float> > gfa_data;
    Ptr< Image::Buffer<float>::voxel_type > gfa;
    Ptr< Image::Buffer<float> > sf_data;
    Ptr< Image::Buffer<float>::voxel_type > sf;



    Point<float> calc_mean_dir (const FOD_lobe&) const;


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

void Segmented_FOD_receiver::set_dixel_sh_output (const std::string& path)
{
  assert (!dixel_sh_data);
  Image::Header H_dixel_sh (H);
  H_dixel_sh.set_ndim (4);
  H_dixel_sh.dim(3) = Math::SH::NforL (lmax);
  dixel_sh_data = new Image::Buffer<float> (path, H_dixel_sh);
  dixel_sh = new Image::Buffer<float>::voxel_type (*dixel_sh_data);
}

void Segmented_FOD_receiver::set_gfa_output (const std::string& path)
{
  assert (!gfa_data);
  gfa_data = new Image::Buffer<float> (path, H);
  gfa = new Image::Buffer<float>::voxel_type (*gfa_data);
}

void Segmented_FOD_receiver::set_sf_output (const std::string& path)
{
  assert (!sf_data);
  sf_data = new Image::Buffer<float> (path, H);
  sf = new Image::Buffer<float>::voxel_type (*sf_data);
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
    for (FOD_lobes::const_iterator i = in.begin(); i != in.end(); ++i) {
      const Point<float> mean_dir (calc_mean_dir (*i));
      sum_decs += Point<float> (Math::abs(mean_dir[0]), Math::abs(mean_dir[1]), Math::abs(mean_dir[2])) * i->get_integral();
    }
    Image::Nav::set_pos (*dec, in.vox);
    (*dec)[3] = 0; dec->value() = sum_decs[0];
    (*dec)[3] = 1; dec->value() = sum_decs[1];
    (*dec)[3] = 2; dec->value() = sum_decs[2];
  }

  if (dixel_sh) {
    Image::Nav::set_pos (*dixel_sh, in.vox);
    Math::Vector<float> sum_dixel_sh;
    sum_dixel_sh.resize (Math::SH::NforL (lmax), 0.0);
    Math::SH::aPSF<float> aPSF (lmax);
    for (FOD_lobes::const_iterator i = in.begin(); i != in.end(); ++i) {
      Math::Vector<float> this_lobe;
      aPSF (this_lobe, calc_mean_dir (*i));
      for (size_t c = 0; c != sum_dixel_sh.size(); ++c)
        sum_dixel_sh[c] += i->get_integral() * this_lobe[c];
    }
    for (size_t c = 0; c != sum_dixel_sh.size(); ++c) {
      (*dixel_sh)[3] = c;
      dixel_sh->value() = sum_dixel_sh[c];
    }
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

  if (sf) {
    float sum = 0.0, maximum = 0.0;
    for (FOD_lobes::const_iterator i = in.begin(); i != in.end(); ++i) {
      sum += i->get_integral();
      maximum = std::max (maximum, i->get_integral());
    }
    const float value = sum ? (maximum / sum) : 0.0;
    Image::Nav::set_value_at_pos (*sf, in.vox, value);
  }

  return true;

}



Point<float> Segmented_FOD_receiver::calc_mean_dir (const FOD_lobe& lobe) const
{
  // Note that this method for calculating the mean direction is approximate only,
  //   but is good enough for most applications
  // For a better method see Buss and Fillmore 2001
  const Point<float>& peak_dir (lobe.get_peak_dir());
  const std::vector<float>& values (lobe.get_values());
  Point<float> mean_dir (0.0, 0.0, 0.0);
  for (size_t j = 0; j != dirs.size(); ++j) {
    if (values[j]) {
      const float multiplier = (peak_dir.dot (dirs.get_dir (j)) > 0.0) ? 1.0 : -1.0;
      mean_dir += multiplier * values[j] * dirs.get_dir (j);
    }
  }
  mean_dir.normalise();
  return mean_dir;
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
  opt = get_options ("dixel_sh");
  if (opt.size()) {
    receiver.set_dixel_sh_output (opt[0][0]);
    ++output_count;
  }
  opt = get_options ("gfa");
  if (opt.size()) {
    receiver.set_gfa_output (opt[0][0]);
    ++output_count;
  }
  opt = get_options ("sf");
  if (opt.size()) {
    receiver.set_sf_output (opt[0][0]);
    ++output_count;
  }
  if (!output_count)
    throw Exception ("Nothing to do; please specify at least one output image type");

  opt = get_options ("mask");
  std::string mask_path;
  if (opt.size()) {
    mask_path = std::string (opt[0][0]);
    Image::Header H_mask (mask_path);
    if (!Image::dimensions_match (fod_data, H_mask, 0, 3))
      throw Exception ("Cannot use image \"" + str(mask_path) + "\" as mask image; dimensions do not match FOD image");
  }

  FOD_queue_writer writer (fod_data);
  if (!mask_path.empty())
    writer.set_mask (mask_path);
  Segmenter fmls (dirs, lmax);
  load_fmls_thresholds (fmls);

  Thread::run_queue_threaded_pipe (writer, SH_coefs(), fmls, FOD_lobes(), receiver);

}

