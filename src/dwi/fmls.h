/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by Robert Smith, 2012.

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



#ifndef __dwi_fmls_h__
#define __dwi_fmls_h__



#include "math/SH.h"
#include "math/vector.h"
#include "point.h"
#include "dwi/directions/set.h"
#include "dwi/directions/mask.h"
#include "image/buffer.h"
#include "image/voxel.h"
#include "image/nav.h"


#include <map> // Used for sorting FOD samples


#define FMLS_RATIO_TO_NEGATIVE_LOBE_INTEGRAL_DEFAULT 0.0
#define FMLS_RATIO_TO_NEGATIVE_LOBE_MEAN_PEAK_DEFAULT 1.0 // Peak amplitude needs to be greater than the mean negative peak
#define FMLS_PEAK_VALUE_THRESHOLD 0.1 // Throw out anything that's below the CSD regularisation threshold
#define FMLS_RATIO_TO_PEAK_VALUE_DEFAULT 1.0 // By default, turn all peaks into lobes (discrete peaks are never merged)



namespace MR {
namespace DWI {
namespace FMLS {


using DWI::Directions::Mask;
using DWI::Directions::dir_t;


class Segmenter;

// These are for configuring the FMLS segmenter at the command line, particularly for fod_metric command
extern const App::OptionGroup FMLSSegmentOption;
void load_fmls_thresholds (Segmenter&);


class FOD_lobe {

  public:
    FOD_lobe (const DWI::Directions::Set& dirs, const dir_t seed, const float value) :
        mask (dirs),
        values (dirs.size(), 0.0),
        peak_dir_bin (seed),
        peak_value (Math::abs (value)),
        peak_dir (dirs.get_dir (seed)),
        integral (Math::abs (value)),
        neg (value <= 0.0)
    {
      mask[seed] = true;
      values[seed] = value;
    }

    // This is used for creating a `null lobe' i.e. an FOD lobe with zero size, containing all directions not
    //   assigned to any other lobe in the voxel
    FOD_lobe (const Mask& i) :
        mask (i),
        values (i.size(), 0.0),
        peak_dir_bin (i.size()),
        peak_value (0.0),
        integral (0.0),
        neg (false) { }


    void add (const dir_t bin, const float value)
    {
      assert ((value <= 0.0 && neg) || (value > 0.0 && !neg));
      mask[bin] = true;
      values[bin] = value;
      integral += Math::abs (value);
    }

    void revise_peak (const Point<float>& real_peak, const float value)
    {
      assert (!neg);
      peak_dir = real_peak;
      peak_value = value;
    }

    void normalise_integral()
    {
      // 4pi == solid angle of sphere in steradians
      integral *= 4.0 * M_PI / float(mask.size());
    }

    void merge (const FOD_lobe& that)
    {
      assert (neg == that.neg);
      mask |= that.mask;
      for (size_t i = 0; i != mask.size(); ++i)
        values[i] += that.values[i];
      if (that.peak_value > peak_value) {
        peak_dir_bin = that.peak_dir_bin;
        peak_value = that.peak_value;
        peak_dir = that.peak_dir;
      }
      integral += that.integral;
    }

    const Mask& get_mask() const { return mask; }
    const std::vector<float>& get_values() const { return values; }
    dir_t get_peak_dir_bin() const { return peak_dir_bin; }
    float get_peak_value() const { return peak_value; }
    const Point<float>& get_peak_dir() const { return peak_dir; }
    float get_integral() const { return integral; }
    bool is_negative() const { return neg; }


  private:
    Mask mask;
    std::vector<float> values;
    dir_t peak_dir_bin;
    float peak_value;
    Point<float> peak_dir;
    float integral;
    bool neg;

};



class FOD_lobes : public std::vector<FOD_lobe> {
  public:
    Point<int> vox;
    std::vector<uint8_t> lut;
};


class SH_coefs : public Math::Vector<float> {
  public:
    Point<int> vox;
};

template <class FODBufferType, class MaskBufferType = Image::Buffer<bool> >
class FODQueueWriter
{

  public:
    typedef typename MaskBufferType::voxel_type mask_voxel_type;

    FODQueueWriter (FODBufferType& fod_buffer) :
        fod_vox (fod_buffer),
        loop ("Segmenting FODs...", 0, 3)
    {
      loop.start (fod_vox);
    }

    FODQueueWriter (FODBufferType& fod_buffer, MaskBufferType& mask_buffer) :
        fod_vox (fod_buffer),
        loop ("Segmenting FODs...", 0, 3),
        mask_vox_ptr (new mask_voxel_type (mask_buffer))
    {
      loop.start (fod_vox);
    }


    void set_mask (MaskBufferType& mask_buffer)
    {
      mask_vox_ptr = new mask_voxel_type (mask_buffer);
    }


    bool operator () (SH_coefs& out)
    {
      if (!loop.ok())
        return false;
      if (mask_vox_ptr) {
        do {
          Image::Nav::set_pos (*mask_vox_ptr, fod_vox);
          if (!mask_vox_ptr->value())
            loop.next (fod_vox);
        } while (loop.ok() && !mask_vox_ptr->value());
      }
      out.vox[0] = fod_vox[0]; out.vox[1] = fod_vox[1]; out.vox[2] = fod_vox[2];
      out.allocate (fod_vox.dim (3));
      for (fod_vox[3] = 0; fod_vox[3] != fod_vox.dim (3); ++fod_vox[3])
        out[fod_vox[3]] = fod_vox.value();
      loop.next (fod_vox);
      return true;
    }


  private:
    typename FODBufferType::voxel_type fod_vox;
    Image::Loop loop;
    Ptr<mask_voxel_type> mask_vox_ptr;

};




class Segmenter {

  public:
    Segmenter (const DWI::Directions::Set&, const size_t);

    bool operator() (const SH_coefs&, FOD_lobes&) const;


    float get_ratio_to_negative_lobe_integral  ()              const { return ratio_to_negative_lobe_integral; }
    void  set_ratio_to_negative_lobe_integral  (const float i)       { ratio_to_negative_lobe_integral = i; }
    float get_ratio_to_negative_lobe_mean_peak ()              const { return ratio_to_negative_lobe_mean_peak; }
    void  set_ratio_to_negative_lobe_mean_peak (const float i)       { ratio_to_negative_lobe_mean_peak = i; }
    float get_peak_value_threshold             ()              const { return peak_value_threshold; }
    void  set_peak_value_threshold             (const float i)       { peak_value_threshold = i; }
    float get_ratio_of_peak_value_to_merge     ()              const { return ratio_of_peak_value_to_merge; }
    void  set_ratio_of_peak_value_to_merge     (const float i)       { ratio_of_peak_value_to_merge = i; }
    bool  get_create_null_lobe                 ()              const { return create_null_lobe; }
    void  set_create_null_lobe                 (const bool  i)       { create_null_lobe = i; verify_settings(); }
    bool  get_create_lookup_table              ()              const { return create_lookup_table; }
    void  set_create_lookup_table              (const bool  i)       { create_lookup_table = i; verify_settings(); }
    bool  get_dilate_lookup_table              ()              const { return dilate_lookup_table; }
    void  set_dilate_lookup_table              (const bool  i)       { dilate_lookup_table = i; verify_settings(); }


  private:

    const DWI::Directions::Set& dirs;

    const size_t lmax;

    RefPtr< Math::SH::Transform<float> > transform;
    RefPtr< Math::SH::PrecomputedAL<float> > precomputer;

    float ratio_to_negative_lobe_integral; // Integral of positive lobe must be at least this ratio larger than the largest negative lobe integral
    float ratio_to_negative_lobe_mean_peak; // Peak value of positive lobe must be at least this ratio larger than the mean negative lobe peak
    float peak_value_threshold; // Absolute threshold for the peak amplitude of the lobe
    float ratio_of_peak_value_to_merge; // Determines whether two lobes get agglomerated into one, depending on the FOD amplitude at the current point and how it compares to the peak amplitudes of the lobes to which it could be assigned
    bool  create_null_lobe; // If this is set, an additional lobe will be created after segmentation with zero size, containing all directions not assigned to any other lobe
    bool  create_lookup_table; // If this is set, an additional lobe will be created after segmentation with zero size, containing all directions not assigned to any other lobe
    bool  dilate_lookup_table; // If this is set, the lookup table created for each voxel will be dilated so that all directions correspond to the nearest positive non-zero FOD lobe


    void verify_settings() const
    {
      if (create_null_lobe && dilate_lookup_table)
        throw Exception ("For FOD segmentation, options '-create_null_lobe' and '-dilate_lookup_table' are mutually exclusive");
      if (!create_lookup_table && dilate_lookup_table)
        throw Exception ("For FOD segmentation, '-create_lookup_table' must be set in order for lookup tables to be dilated ('-dilate_lookup_table')");
    }

};




}
}
}


#endif

