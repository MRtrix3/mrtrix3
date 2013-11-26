/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

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



#ifndef __dwi_tractography_sift_model_base_h__
#define __dwi_tractography_sift_model_base_h__

#include "app.h"
#include "point.h"
#include "ptr.h"

#include "dwi/fixel_map.h"
#include "dwi/fmls.h"

#include "dwi/directions/set.h"

#include "dwi/tractography/ACT/tissues.h"

#include "dwi/tractography/mapping/fixel_td_map.h"
#include "dwi/tractography/mapping/voxel.h"

#include "dwi/tractography/SIFT/proc_mask.h"

#include "image/buffer.h"
#include "image/buffer_scratch.h"
#include "image/copy.h"
#include "image/header.h"
#include "image/loop.h"

#include "thread/queue.h"


//#define SIFT_MODEL_OUTPUT_SH_IMAGES
#define SIFT_MODEL_OUTPUT_FIXEL_IMAGES


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace SIFT
      {



      class FixelBase
      {

        public:
          FixelBase () :
            FOD (0.0),
            TD (0.0),
            weight (0.0),
            dir () { }

          FixelBase (const double amp) :
            FOD (amp),
            TD (0.0),
            weight (1.0),
            dir () { }

          FixelBase (const double amp, const Point<float>& d) :
            FOD (amp),
            TD (0.0),
            weight (1.0),
            dir (d) { }

          FixelBase (const FMLS::FOD_lobe& lobe) :
            FOD (lobe.get_integral()),
            TD (0.0),
            weight (1.0),
            dir (lobe.get_mean_dir()) { }

          FixelBase (const FixelBase& that) :
            FOD (that.FOD),
            TD (that.TD),
            weight (that.weight),
            dir (that.dir) { }


          double get_FOD()    const { return FOD; }
          double get_TD()     const { return TD; }
          float  get_weight() const { return weight; }
          const Point<float>& get_dir() const { return dir; }

          void       scale_FOD  (const float factor)  { FOD *= factor; }
          void       set_weight (const float w)       { weight = w; }
          FixelBase& operator+= (const double length) { TD += length; return *this; }
          void       clear_TD   ()                    { TD = 0.0; }

          double get_diff (const double mu) const { return ((TD * mu) - FOD); }
          double get_cost (const double mu) const { return get_cost_unweighted        (mu) * weight; }


        protected:
          double FOD;
          double TD;
          float  weight;
          Point<float> dir;

          double get_cost_unweighted (const double mu) const { return (Math::pow2 (get_diff (mu))); }

      };






      // Templated Fixel class should derive from FixelBase to ensure that it has adequate functionality
      // This class stores the necessary fixel information (including streamline densities), but does not retain the
      //   list of fixels traversed by each streamline. If this information is necessary, use the Model class (model.h)

      template <class Fixel>
      class ModelBase : public Mapping::Fixel_TD_map<Fixel>
      {

        typedef typename Fixel_map<Fixel>::MapVoxel MapVoxel;
        typedef typename Fixel_map<Fixel>::VoxelAccessor VoxelAccessor;

        public:
          template <class Set>
          ModelBase (Set& dwi, const DWI::Directions::FastLookupSet& dirs) :
              Mapping::Fixel_TD_map<Fixel> (dwi, dirs),
              proc_mask (Fixel_map<Fixel>::info(), "SIFT model processing mask"),
              FOD_sum (0.0),
              TD_sum (0.0),
              have_null_lobes (false)
          {
            SIFT::initialise_processing_mask (dwi, proc_mask, act_4tt);
            H.info() = dwi.info();
            H.set_ndim (3);
          }

          virtual ~ModelBase () { }


          template <class Set>
          void perform_FOD_segmentation (Set&);
          void scale_FODs_by_GM ();

          virtual bool operator() (const FMLS::FOD_lobes& in);
          virtual bool operator() (const Mapping::SetDixel& in);

          double calc_cost_function() const;

          double mu() const { return FOD_sum / TD_sum; }
          bool have_act_data() const { return act_4tt; }

          void output_proc_mask (const std::string&);
          void output_all_debug_images (const std::string&) const;

          using Mapping::Fixel_TD_map<Fixel>::begin;


        protected:
          using Fixel_map<Fixel>::accessor;
          using Fixel_map<Fixel>::fixels;
          using Mapping::Fixel_TD_map<Fixel>::dirs;

          Ptr< Image::BufferScratch<float> > act_4tt;
          Image::BufferScratch<float> proc_mask;
          Image::Header H;
          double FOD_sum, TD_sum;
          bool have_null_lobes;

          // The definitions of these functions are located in dwi/tractography/SIFT/output.h
          void output_target_image (const std::string&) const;
          void output_target_image_sh (const std::string&) const;
          void output_target_image_fixel (const std::string&) const;
          void output_tdi (const std::string&) const;
          void output_tdi_null_lobes (const std::string&) const;
          void output_tdi_sh (const std::string&) const;
          void output_tdi_fixel (const std::string&) const;
          void output_error_images (const std::string&, const std::string&, const std::string&) const;
          void output_scatterplot (const std::string&) const;
          void output_fixel_count_image (const std::string&) const;
          void output_untracked_fixels (const std::string&, const std::string&) const;


          ModelBase (const ModelBase& that) : Mapping::Fixel_TD_map<Fixel> (that), act_4tt (NULL), proc_mask (that.proc_mask.info()), FOD_sum (0.0), TD_sum (0.0) { assert (0); }

      };




      template <class Fixel>
      template <class Set>
      void ModelBase<Fixel>::perform_FOD_segmentation (Set& data)
      {
        DWI::FMLS::FODQueueWriter<Set, Image::BufferScratch<float> > writer (data, proc_mask);
        DWI::FMLS::Segmenter fmls (dirs, Math::SH::LforN (data.dim(3)));
        fmls.set_dilate_lookup_table (!App::get_options ("no_dilate_lut").size());
        fmls.set_create_null_lobe (App::get_options ("make_null_lobes").size());
        Thread::run_queue_threaded_pipe (writer, FMLS::SH_coefs(), fmls, FMLS::FOD_lobes(), *this);
        have_null_lobes = fmls.get_create_null_lobe();
      }




      template <class Fixel>
      void ModelBase<Fixel>::scale_FODs_by_GM ()
      {
        if (App::get_options("no_fod_scaling").size() || !act_4tt)
          return;
        // Loop through voxels, getting total GM fraction for each, and scale all fixels in each voxel
        VoxelAccessor v (accessor);
        Image::BufferScratch<float>::voxel_type v_anat (*act_4tt);
        FOD_sum = 0.0;
        Image::LoopInOrder loop (v);
        for (loop.start (v, v_anat); loop.ok(); loop.next (v, v_anat)) {
          Tractography::ACT::Tissues tissues (v_anat);
          const float multiplier = 1.0 - tissues.get_cgm() - (0.5 * tissues.get_sgm()); // Heuristic
          for (typename Fixel_map<Fixel>::Iterator i = begin(v); i; ++i) {
            i().scale_FOD (multiplier);
            FOD_sum += i().get_weight() * i().get_FOD();
          }
        }
      }




      template <class Fixel>
      bool ModelBase<Fixel>::operator() (const FMLS::FOD_lobes& in)
      {
        if (!Fixel_map<Fixel>::operator() (in))
          return false;
        Image::BufferScratch<float>::voxel_type mask (proc_mask);
        const float mask_value = Image::Nav::get_value_at_pos (mask, in.vox);
        VoxelAccessor v (accessor);
        Image::Nav::set_pos (v, in.vox);
        if (v.value()) {
          for (typename Fixel_map<Fixel>::Iterator i = begin (v); i; ++i) {
            i().set_weight (mask_value);
            FOD_sum += i().get_FOD() * mask_value;
          }
        }
        return true;
      }




      template <class Fixel>
      bool ModelBase<Fixel>::operator() (const Mapping::SetDixel& in)
      {
        Image::BufferScratch<float>::voxel_type v (proc_mask);
        float total_contribution = 0.0;
        for (Mapping::SetDixel::const_iterator i = in.begin(); i != in.end(); ++i) {
          const size_t fixel_index = Mapping::Fixel_TD_map<Fixel>::dixel2fixel (*i);
          if (fixel_index) {
            fixels[fixel_index] += i->get_value();
            total_contribution += fixels[fixel_index].get_weight() * i->get_value();
          }
        }
        TD_sum += total_contribution;
        return true;
      }




      template <class Fixel>
      double ModelBase<Fixel>::calc_cost_function() const
      {
        const double current_mu = mu();
        double cost = 0.0;
        typename std::vector<Fixel>::const_iterator i = fixels.begin();
        for (++i; i != fixels.end(); ++i)
          cost += i->get_cost (current_mu);
        return cost;
      }



      template <class Fixel>
      void ModelBase<Fixel>::output_proc_mask (const std::string& path)
      {
        Image::BufferScratch<float>::voxel_type v (proc_mask);
        v.save (path);
      }




      template <class Fixel>
      void ModelBase<Fixel>::output_all_debug_images (const std::string& prefix) const
      {
        output_target_image (prefix + "_target.mif");
#ifdef SIFT_MODEL_OUTPUT_SH_IMAGES
        output_target_image_sh (prefix + "_target_sh.mif");
#endif
#ifdef SIFT_MODEL_OUTPUT_FIXEL_IMAGES
        output_target_image_fixel (prefix + "_target_fixel.msih");
#endif
        output_tdi (prefix + "_tdi.mif");
        if (have_null_lobes)
          output_tdi_null_lobes (prefix + "_tdi_null_lobes.mif");
#ifdef SIFT_MODEL_OUTPUT_SH_IMAGES
        output_tdi_sh (prefix + "_tdi_sh.mif");
#endif
#ifdef SIFT_MODEL_OUTPUT_FIXEL_IMAGES
        output_tdi_fixel (prefix + "_tdi_fixel.msih");
#endif
        output_error_images (prefix + "_max_abs_diff.mif", prefix + "_diff.mif", prefix + "_cost.mif");
        output_scatterplot (prefix + "_scatterplot.csv");
        output_fixel_count_image (prefix + "_fixel_count.mif");
        output_untracked_fixels (prefix + "_untracked_count.mif", prefix + "_untracked_amps.mif");
      }




      }
    }
  }
}


#endif
