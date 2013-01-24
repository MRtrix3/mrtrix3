/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert Smith, 2011.

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



#ifndef __dwi_tractography_sift_misc_h__
#define __dwi_tractography_sift_misc_h__



#include "point.h"

#include <stdint.h>

#include "thread/queue.h"

#include "min_mem_array.h"

#include "dwi/fod_map.h"




namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace SIFT
      {



      typedef unsigned int track_t;
      typedef unsigned int voxel_t;




      class FOD_TD_weighted
      {

        public:
          FOD_TD_weighted () :
            FOD (0.0),
            TD (0.0),
            weight (0.0),
            dir (0) { }

          FOD_TD_weighted (const double amp, size_t d) :
            FOD (amp),
            TD (0.0),
            weight (0.0),
            dir (d) { }

          FOD_TD_weighted (const FOD_lobe& lobe) :
            FOD (lobe.get_integral()),
            TD (0.0),
            weight (0.0),
            dir (lobe.get_peak_dir_bin()) { }

          FOD_TD_weighted (const FOD_TD_weighted& that) :
            FOD (that.FOD),
            TD (that.TD),
            weight (that.weight),
            dir (that.dir)
          { }


          double get_FOD()    const { return FOD; }
          double get_TD()     const { return TD; }
          float  get_weight() const { return weight; }
          size_t get_dir()    const { return dir; }


          void             set_weight (const float w)       { weight = w; }
          FOD_TD_weighted& operator+= (const double length) { TD += length; return *this; }


          // This function has two purposes; removes the relevant track length, and also provides the change in cost function given that removal
          double remove_TD (const double length, const double new_mu, const double old_mu)
          {
            const double old_cost = get_cost (old_mu);
            TD = MAX(TD - length, 0.0);
            const double new_cost = get_cost (new_mu);
            return (new_cost - old_cost);
          }

          void clear_TD() { TD = 0.0; }


          double get_diff          (const double mu) const { return ((TD * mu) - FOD); }
          double get_cost          (const double mu) const { return get_cost_unweighted          (mu) * weight; }
          double get_d_cost_d_mu   (const double mu) const { return get_d_cost_d_mu_unweighted   (mu) * weight; }
          double get_d_cost_d_TD   (const double mu) const { return get_d_cost_d_TD_unweighted   (mu) * weight; }
          double get_d2_cost_d_TD2 (const double mu) const { return get_d2_cost_d_TD2_unweighted (mu) * weight; }

          double get_cost_wo_track  (const double mu, const double length)    const { return get_cost_wo_track_unweighted  (mu, length)    * weight; }
          double get_cost_manual_TD (const double mu, const double manual_TD) const { return get_cost_manual_TD_unweighted (mu, manual_TD) * weight; }

          double calc_quantisation  (const double mu, const double length)    const { return get_cost_manual_TD (mu, (FOD/mu) + length); }


        private:
          double FOD;
          double TD;
          float weight;
          size_t dir; // This is purely for visualisation purposes

          double get_cost_unweighted          (const double mu) const { return (Math::pow2 (get_diff (mu))); }
          double get_d_cost_d_mu_unweighted   (const double mu) const { return (2.0 * TD * get_diff (mu)); }
          double get_d_cost_d_TD_unweighted   (const double mu) const { return (2.0 * mu * get_diff (mu)); }
          double get_d2_cost_d_TD2_unweighted (const double mu) const { return (2.0 * Math::pow2 (mu)); }

          double get_cost_wo_track_unweighted  (const double mu, const double length)    const { return (Math::pow2 (((TD - length) * mu) - FOD)); }
          double get_cost_manual_TD_unweighted (const double mu, const double manual_TD) const { return  Math::pow2 ((  manual_TD   * mu) - FOD); }

      };





      class Track_lobe_contribution
      {
        public:
          Track_lobe_contribution (const uint32_t lobe_index, const float length) {
            const uint32_t length_as_int = MIN(255, Math::round (scale_to_storage * length));
            data = (lobe_index & 0x00FFFFFF) | (length_as_int << 24);
          }

          Track_lobe_contribution() :
            data (0) { }

          uint32_t get_lobe_index() const { return (data & 0x00FFFFFF); }
          float    get_value()      const { return (uint32_t((data & 0xFF000000) >> 24) * scale_from_storage); }


          bool add (const float length)
          {
            // Allow summing of multiple contributions to a lobe, UNLESS it would cause truncation, in which
            //   case keep them separate
            const uint32_t increment = Math::round (scale_to_storage * length);
            const uint32_t existing = (data & 0xFF000000) >> 24;
            if (existing + increment > 255)
              return false;
            data = (data & 0x00FFFFFF) | ((existing + increment) << 24);
            return true;
          }


          static void set_scaling (const Image::Info& in)
          {
            const float max_length = Math::sqrt (Math::pow2 (in.vox(0)) + Math::pow2 (in.vox(1)) + Math::pow2 (in.vox(2)));
            scale_to_storage = 255.0 / max_length;
            scale_from_storage = max_length / 255.0;
          }


        private:
          uint32_t data;

          static float scale_to_storage, scale_from_storage;

      };




      /*
// This is a 'safe' version of Track_lobe_contribution that does not use byte-sharing, but requires double the RAM
class Track_lobe_contribution
{
  public:
    Track_lobe_contribution (const uint32_t lobe_index, const float length) :
      lobe (lobe_index),
      value (length) { }

    Track_lobe_contribution() :
      lobe (0),
      value (0.0) { }


    bool add (const float length) { value += length; return true; }

    uint32_t get_lobe_index() const { return lobe; }
    float    get_value()      const { return value; }


  private:
    uint32_t lobe;
    float value;

};
       */




      template <class Cont>
      class TrackContribution : public Min_mem_array<Cont>
      {

        public:
        TrackContribution (const std::vector<Cont>& in, const float c, const float l) :
          Min_mem_array<Cont> (in),
          total_contribution  (c),
          total_length        (l) { }

        TrackContribution () :
          Min_mem_array<Cont> (),
          total_contribution  (0.0),
          total_length        (0.0) { }

        float get_total_contribution() const { return total_contribution; }
        float get_total_length      () const { return total_length; }


        private:
        const float total_contribution, total_length;


      };



      class Cost_fn_gradient_sort
      {
        public:
          Cost_fn_gradient_sort (const track_t i, const double g, const double gpul) :
            tck_index (i),
            cost_gradient (g),
            grad_per_unit_length (gpul) { }

          Cost_fn_gradient_sort (const Cost_fn_gradient_sort& that) :
            tck_index            (that.tck_index),
            cost_gradient        (that.cost_gradient),
            grad_per_unit_length (that.grad_per_unit_length) { }

          void set (const track_t i, const double g, const double gpul) { tck_index = i; cost_gradient = g; grad_per_unit_length = gpul; }

          bool operator< (const Cost_fn_gradient_sort& that) const { return grad_per_unit_length < that.grad_per_unit_length; }

          track_t get_tck_index()                const { return tck_index; }
          double  get_cost_gradient()            const { return cost_gradient; }
          double  get_gradient_per_unit_length() const { return grad_per_unit_length; }

        private:
          track_t tck_index;
          double  cost_gradient;
          double  grad_per_unit_length;
      };



      }
    }
  }
}


#endif


