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

  + Option ("dixels",
            "compute a SH image showing the orientations & relative ampitudes of segmented fibre populations (useful for assessing segmentation performance)")
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




class Lobe
{
  public:
    Lobe (const FOD_lobe& in) :
      values (in.get_values()),
      peak_value (in.get_peak_value()),
      peak_dir (in.get_peak_dir()),
      integral (in.get_integral()) { }

    Lobe () :
      values (),
      peak_value (NAN),
      peak_dir (),
      integral (NAN) { }


    const std::vector<float>& get_values() const { return values; }
    float get_peak_value() const { return peak_value; }
    const Point<float>& get_peak_dir() const { return peak_dir; }
    float get_integral() const { return integral; }


  private:
    std::vector<float> values;
    float peak_value;
    Point<float> peak_dir;
    float integral;
};





class FOD_metric_map : public FOD_map<Lobe>
{

  public:
    template <class Set>
    FOD_metric_map (const Set& i, const DWI::Directions::Set& dirs) :
        FOD_map<Lobe> (i),
        lmax (Math::SH::LforN (i.dim(3))),
        num_dirs (dirs.size()) { }


    float get_afd (const Point<int>&) const;
    int get_count (const Point<int>&) const;
    Point<float> get_dec (const Point<int>&, const DWI::Directions::Set&) const;
    Math::Vector<float> get_dixels (const Point<int>&, const DWI::Directions::Set&) const;
    float get_gfa (const Point<int>&) const;
    float get_sf (const Point<int>&) const;


  private:
    const size_t lmax;
    const size_t num_dirs;

    Point<float> calc_mean_dir (const Lobe&, const DWI::Directions::Set&) const;

};

float FOD_metric_map::get_afd (const Point<int>& voxel) const
{
  VoxelAccessor v (accessor);
  Image::Nav::set_pos (v, voxel);
  float value = 0.0;
  for (FOD_map<Lobe>::ConstIterator i = begin (v); i; ++i)
    value += i().get_integral();
  return value;
}

int FOD_metric_map::get_count (const Point<int>& voxel) const
{
  VoxelAccessor v (accessor);
  Image::Nav::set_pos (v, voxel);
  int count = 0;
  for (FOD_map<Lobe>::ConstIterator i = begin (v); i; ++i)
    ++count;
  return count;
}

Point<float> FOD_metric_map::get_dec (const Point<int>& voxel, const DWI::Directions::Set& dirs) const
{
  VoxelAccessor v (accessor);
  Image::Nav::set_pos (v, voxel);
  Point<float> dec (0.0, 0.0, 0.0);
  for (FOD_map<Lobe>::ConstIterator i = begin (v); i; ++i) {
    const Point<float> mean_dir = calc_mean_dir (i(), dirs);
    dec += i().get_integral() * Point<float> (Math::abs (mean_dir[0]), Math::abs (mean_dir[1]), Math::abs (mean_dir[2]));
  }
  return dec;
}

Math::Vector<float> FOD_metric_map::get_dixels (const Point<int>& voxel, const DWI::Directions::Set& dirs) const
{
  VoxelAccessor v (accessor);
  Image::Nav::set_pos (v, voxel);
  Math::Vector<float> dixels;
  dixels.resize (Math::SH::NforL (lmax), 0.0);
  Math::SH::aPSF<float> aPSF (lmax);
  for (FOD_map<Lobe>::ConstIterator i = begin (v); i; ++i) {
    Math::Vector<float> this_lobe;
    aPSF (this_lobe, calc_mean_dir (i(), dirs));
    for (size_t c = 0; c != dixels.size(); ++c)
      dixels[c] += i().get_integral() * this_lobe[c];
  }
  return dixels;
}

float FOD_metric_map::get_gfa (const Point<int>& voxel) const
{
  VoxelAccessor v (accessor);
  Image::Nav::set_pos (v, voxel);
  double sum = 0.0;
  std::vector<float> combined_values (num_dirs, 0.0);
  for (FOD_map<Lobe>::ConstIterator i = begin (v); i; ++i) {
    const std::vector<float>& values = i().get_values();
    for (size_t j = 0; j != num_dirs; ++j) {
      if (values[j]) {
        sum += values[j];
        combined_values[j] = values[j];
      }
    }
  }
  if (!sum)
    return 0.0;
  const float fod_normaliser = 1.0 / sum;
  const float normalised_mean = 1.0 / float(num_dirs);
  double sum_variance = 0.0, sum_of_squares = 0.0;
  for (size_t i = 0; i != num_dirs; ++i) {
    sum_variance   += Math::pow2((combined_values[i] * fod_normaliser) - normalised_mean);
    sum_of_squares += Math::pow2 (combined_values[i] * fod_normaliser);
  }
  const float mean_variance = sum_variance   / double(num_dirs - 1);
  const float mean_square   = sum_of_squares / double(num_dirs);
  return Math::sqrt (mean_variance / mean_square);
}

float FOD_metric_map::get_sf (const Point<int>& voxel) const
{
  VoxelAccessor v (accessor);
  Image::Nav::set_pos (v, voxel);
  float sum = 0.0, maximum = 0.0;
  for (FOD_map<Lobe>::ConstIterator i = begin (v); i; ++i) {
    sum += i().get_integral();
    maximum = std::max (maximum, i().get_integral());
  }
  return (sum ? (maximum / sum) : 0.0);
}

Point<float> FOD_metric_map::calc_mean_dir (const Lobe& lobe, const DWI::Directions::Set& dirs) const
{
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
  Image::Buffer<float> fod_data (argument[0]);

  if (fod_data.ndim() != 4)
    throw Exception ("input FOD image should contain 4 dimensions");

  const size_t lmax = Math::SH::LforN (fod_data.dim(3));

  if (Math::SH::NforL (lmax) != size_t(fod_data.dim(3)))
    throw Exception ("Input image does not appear to contain an SH series per voxel");

  std::string afd_path, count_path, dec_path, dixel_path, sf_path, gfa_path, mask_path;
  size_t output_count = 0;
  Options opt = get_options ("afd");
  if (opt.size()) {
    afd_path = std::string (opt[0][0]);
    ++output_count;
  }
  opt = get_options ("count");
  if (opt.size()) {
    count_path = std::string (opt[0][0]);
    ++output_count;
  }
  opt = get_options ("dec");
  if (opt.size()) {
    dec_path = std::string (opt[0][0]);
    ++output_count;
  }
  opt = get_options ("dixels");
  if (opt.size()) {
    dixel_path = std::string (opt[0][0]);
    ++output_count;
  }
  opt = get_options ("gfa");
  if (opt.size()) {
    gfa_path = std::string (opt[0][0]);
    ++output_count;
  }
  opt = get_options ("sf");
  if (opt.size()) {
    sf_path = std::string (opt[0][0]);
    ++output_count;
  }
  opt = get_options ("mask");
  if (opt.size()) {
    mask_path = std::string (opt[0][0]);
    Image::Header H_mask (mask_path);
    if (!Image::dimensions_match (fod_data, H_mask, 0, 3))
      throw Exception ("Cannot use image \"" + str(mask_path) + "\" as mask image; dimensions do not match FOD image");
  }
  if (!output_count)
    throw Exception ("Nothing to do; please specify at least one output image type");

  const DWI::Directions::Set dirs (1281);
  FOD_metric_map map (fod_data, dirs);

  {
    FOD_queue_writer writer (fod_data);
    if (!mask_path.empty())
      writer.set_mask (mask_path);
    Segmenter fmls (dirs, lmax);
    load_fmls_thresholds (fmls);

    Thread::run_queue_threaded_pipe (writer, SH_coefs(), fmls, FOD_lobes(), map);
  }

  ProgressBar progress ("Generating output images... ", output_count);

  Image::Header H_out (argument[0]);
  H_out.set_ndim (3);
  Image::Loop loop (0, 3);

  if (!afd_path.empty()) {
    Image::Buffer<float> afd_out (afd_path, H_out);
    Image::Buffer<float>::voxel_type v (afd_out);
    for (loop.start (v); loop.ok(); loop.next (v))
      v.value() = map.get_afd (Point<int> (v[0], v[1], v[2]));
    ++progress;
  }

  if (!count_path.empty()) {
    Image::Header H_count (H_out);
    H_count.datatype() = DataType::UInt8;
    Image::Buffer<uint8_t> count_out (count_path, H_count);
    Image::Buffer<uint8_t>::voxel_type v (count_out);
    for (loop.start (v); loop.ok(); loop.next (v))
      v.value() = map.get_count (Point<int> (v[0], v[1], v[2]));
    ++progress;
  }

  if (!dec_path.empty()) {
    Image::Header H_dec (H_out);
    H_dec.set_ndim (4);
    H_dec.dim (3) = 3;
    H_dec.stride (3) = 0;
    Image::Buffer<float> dec_out (dec_path, H_dec);
    Image::Buffer<float>::voxel_type v (dec_out);
    for (loop.start (v); loop.ok(); loop.next (v)) {
      const Point<float> dec (map.get_dec (Point<int> (v[0], v[1], v[2]), dirs));
      for (v[3] = 0; v[3] != 3; ++v[3])
        v.value() = dec[size_t(v[3])];
    }
    ++progress;
  }

  if (!dixel_path.empty()) {
    Image::Header H_dixel (argument[0]);
    Image::Buffer<float> dixel_out (dixel_path, H_dixel);
    Image::Buffer<float>::voxel_type v (dixel_out);
    for (loop.start (v); loop.ok(); loop.next (v)) {
      const Math::Vector<float> dixels (map.get_dixels (Point<int> (v[0], v[1], v[2]), dirs));
      for (v[3] = 0; v[3] != v.dim(3); ++v[3])
        v.value() = dixels[v[3]];
    }
    ++progress;
  }

  if (!gfa_path.empty()) {
    Image::Buffer<float> gfa_out (gfa_path, H_out);
    Image::Buffer<float>::voxel_type v (gfa_out);
    for (loop.start (v); loop.ok(); loop.next (v))
      v.value() = map.get_gfa (Point<int> (v[0], v[1], v[2]));
    ++progress;
  }

  if (!sf_path.empty()) {
    Image::Buffer<float> sf_out (sf_path, H_out);
    Image::Buffer<float>::voxel_type v (sf_out);
    for (loop.start (v); loop.ok(); loop.next (v))
      v.value() = map.get_sf (Point<int> (v[0], v[1], v[2]));
    ++progress;
  }

}

