/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#ifndef __dwi_tractography_sift_model_base_h__
#define __dwi_tractography_sift_model_base_h__

#include "app.h"
#include "dwi/fixel_map.h"
#include "dwi/fmls.h"

#include "dwi/directions/set.h"

#include "dwi/tractography/file.h"

#include "dwi/tractography/ACT/tissues.h"

#include "dwi/tractography/mapping/fixel_td_map.h"
#include "dwi/tractography/mapping/loader.h"
#include "dwi/tractography/mapping/mapper.h"
#include "dwi/tractography/mapping/mapping.h"
#include "dwi/tractography/mapping/voxel.h"

#include "dwi/tractography/SIFT/proc_mask.h"
#include "dwi/tractography/SIFT/types.h"

#include "file/path.h"

#include "image.h"
#include "algo/copy.h"
#include "thread_queue.h"


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
        { MEMALIGN(FixelBase)

          public:
            FixelBase () :
              FOD (0.0),
              TD (0.0),
              weight (0.0),
              dir () { }

            FixelBase (const default_type amp) :
              FOD (amp),
              TD (0.0),
              weight (1.0),
              dir () { }

            FixelBase (const default_type amp, const Eigen::Vector3& d) :
              FOD (amp),
              TD (0.0),
              weight (1.0),
              dir (d) { }

            FixelBase (const FMLS::FOD_lobe& lobe) :
              FOD (lobe.get_integral()),
              TD (0.0),
              weight (1.0),
              dir (lobe.get_mean_dir()) { }

            FixelBase (const FixelBase&) = default;

            default_type get_FOD()    const { return FOD; }
            default_type get_TD()     const { return TD; }
            default_type get_weight() const { return weight; }
            const Eigen::Vector3& get_dir() const { return dir; }

            void       scale_FOD  (const default_type factor) { FOD *= factor; }
            void       set_weight (const default_type w)      { weight = w; }
            FixelBase& operator+= (const default_type length) { TD += length; return *this; }

            void clear_TD() { TD = 0.0; }

            default_type get_diff (const default_type mu) const { return ((TD * mu) - FOD); }
            default_type get_cost (const default_type mu) const { return get_cost_unweighted (mu) * weight; }


          protected:
            default_type FOD, TD, weight;
            Eigen::Vector3 dir;

            default_type get_cost_unweighted (const default_type mu) const { return Math::pow2 (get_diff (mu)); }

        };






        // Templated Fixel class should derive from FixelBase to ensure that it has adequate functionality
        // This class stores the necessary fixel information (including streamline densities), but does not retain the
        //   list of fixels traversed by each streamline. If this information is necessary, use the Model class (model.h)

        template <class Fixel>
        class ModelBase : public Mapping::Fixel_TD_map<Fixel>
        { MEMALIGN(ModelBase<Fixel>)

          protected:
            using MapVoxel = typename Fixel_map<Fixel>::MapVoxel;
            using VoxelAccessor = typename Fixel_map<Fixel>::VoxelAccessor;

          public:
            ModelBase (Image<float>& dwi, const DWI::Directions::FastLookupSet& dirs) :
                Mapping::Fixel_TD_map<Fixel> (dwi, dirs),
                proc_mask (Image<float>::scratch (Fixel_map<Fixel>::header(), "SIFT model processing mask")),
                FOD_sum (0.0),
                TD_sum (0.0),
                have_null_lobes (false)
            {
              SIFT::initialise_processing_mask (dwi, proc_mask, act_5tt);
            }
            ModelBase (const ModelBase&) = delete;

            virtual ~ModelBase () { }

            void perform_FOD_segmentation (Image<float>&);
            void scale_FDs_by_GM();

            void map_streamlines (const std::string&);

            virtual bool operator() (const FMLS::FOD_lobes& in);
            virtual bool operator() (const Mapping::SetDixel& in);

            default_type calc_cost_function() const;

            default_type mu() const { return FOD_sum / TD_sum; }
            bool have_act_data() const { return act_5tt.valid(); }

            void output_proc_mask (const std::string&);
            void output_5tt_image (const std::string&);
            void output_all_debug_images (const std::string&) const;

            using Mapping::Fixel_TD_map<Fixel>::begin;


          protected:
            using Fixel_map<Fixel>::accessor;
            using Fixel_map<Fixel>::fixels;
            using Mapping::Fixel_TD_map<Fixel>::dirs;

            Image<float> act_5tt, proc_mask;
            default_type FOD_sum, TD_sum;
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
            void output_error_fixel_images (const std::string&, const std::string&) const;
            void output_scatterplot (const std::string&) const;
            void output_fixel_count_image (const std::string&) const;
            void output_untracked_fixels (const std::string&, const std::string&) const;

        };




        template <class Fixel>
        void ModelBase<Fixel>::perform_FOD_segmentation (Image<float>& data)
        {
          Math::SH::check (data);
          DWI::FMLS::FODQueueWriter writer (data, proc_mask);
          DWI::FMLS::Segmenter fmls (dirs, Math::SH::LforN (data.size(3)));
          fmls.set_dilate_lookup_table (!App::get_options ("no_dilate_lut").size());
          fmls.set_create_null_lobe (App::get_options ("make_null_lobes").size());
          Thread::run_queue (writer, Thread::batch (FMLS::SH_coefs()), Thread::multi (fmls), Thread::batch (FMLS::FOD_lobes()), *this);
          have_null_lobes = fmls.get_create_null_lobe();
        }




        template <class Fixel>
        void ModelBase<Fixel>::scale_FDs_by_GM ()
        {
          if (!App::get_options("fd_scale_gm").size())
            return;
          if (!act_5tt.valid()) {
            INFO ("Cannot scale fibre densities according to GM fraction; no ACT image data provided");
            return;
          }
          // Loop through voxels, getting total GM fraction for each, and scale all fixels in each voxel
          VoxelAccessor v (accessor());
          FOD_sum = 0.0;
          for (auto l = Loop(v) (v, act_5tt); l; ++l) {
            Tractography::ACT::Tissues tissues (act_5tt);
            const default_type multiplier = 1.0 - tissues.get_cgm() - (0.5 * tissues.get_sgm()); // Heuristic
            for (typename Fixel_map<Fixel>::Iterator i = begin(v); i; ++i) {
              i().scale_FOD (multiplier);
              FOD_sum += i().get_weight() * i().get_FOD();
            }
          }
        }




        template <class Fixel>
        void ModelBase<Fixel>::map_streamlines (const std::string& path)
        {
          Tractography::Properties properties;
          Tractography::Reader<> file (path, properties);

          const track_t count = (properties.find ("count") == properties.end()) ? 0 : to<track_t>(properties["count"]);
          if (!count)
            throw Exception ("Cannot map streamlines: track file " + Path::basename(path) + " is empty");

          Mapping::TrackLoader loader (file, count);
          Mapping::TrackMapperBase mapper (Fixel_map<Fixel>::header(), dirs);
          mapper.set_upsample_ratio (Mapping::determine_upsample_ratio (Fixel_map<Fixel>::header(), properties, 0.1));
          mapper.set_use_precise_mapping (true);
          Thread::run_queue (
              loader,
              Thread::batch (Tractography::Streamline<float>()),
              Thread::multi (mapper),
              Thread::batch (Mapping::SetDixel()),
              *this);

          INFO ("Proportionality coefficient after streamline mapping is " + str (mu()));
        }




        template <class Fixel>
        bool ModelBase<Fixel>::operator() (const FMLS::FOD_lobes& in)
        {
          if (!Fixel_map<Fixel>::operator() (in))
            return false;

          VoxelAccessor v (accessor());
          assign_pos_of (in.vox).to (v);

          if (v.value()) {
            assign_pos_of (in.vox).to (proc_mask);
            const float mask_value = proc_mask.value();

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
          default_type total_contribution = 0.0;
          for (Mapping::SetDixel::const_iterator i = in.begin(); i != in.end(); ++i) {
            const size_t fixel_index = Mapping::Fixel_TD_map<Fixel>::dixel2fixel (*i);
            if (fixel_index) {
              fixels[fixel_index] += i->get_length();
              total_contribution += fixels[fixel_index].get_weight() * i->get_length();
            }
          }
          TD_sum += total_contribution;
          return true;
        }




        template <class Fixel>
        default_type ModelBase<Fixel>::calc_cost_function() const
        {
          const default_type current_mu = mu();
          default_type cost = 0.0;
          for (auto i = fixels.cbegin()+1; i != fixels.end(); ++i)
            cost += i->get_cost (current_mu);
          return cost;
        }



        template <class Fixel>
        void ModelBase<Fixel>::output_proc_mask (const std::string& path)
        {
          save (proc_mask, path);
        }


        template <class Fixel>
        void ModelBase<Fixel>::output_5tt_image (const std::string& path)
        {
          if (!have_act_data())
            throw Exception ("Cannot export 5TT image; no such data present");
          save (act_5tt, path);
        }




        template <class Fixel>
        void ModelBase<Fixel>::output_all_debug_images (const std::string& prefix) const
        {
          output_target_image (prefix + "_target.mif");
#ifdef SIFT_MODEL_OUTPUT_SH_IMAGES
          output_target_image_sh (prefix + "_target_sh.mif");
#endif
#ifdef SIFT_MODEL_OUTPUT_FIXEL_IMAGES
          output_target_image_fixel (prefix + "_target_fixel.msf");
#endif
          output_tdi (prefix + "_tdi.mif");
          if (have_null_lobes)
            output_tdi_null_lobes (prefix + "_tdi_null_lobes.mif");
#ifdef SIFT_MODEL_OUTPUT_SH_IMAGES
          output_tdi_sh (prefix + "_tdi_sh.mif");
#endif
#ifdef SIFT_MODEL_OUTPUT_FIXEL_IMAGES
          output_tdi_fixel (prefix + "_tdi_fixel.msf");
#endif
          output_error_images (prefix + "_max_abs_diff.mif", prefix + "_diff.mif", prefix + "_cost.mif");
#ifdef SIFT_MODEL_OUTPUT_FIXEL_IMAGES
          output_error_fixel_images (prefix + "_diff_fixel.msf", prefix + "_cost_fixel.msf");
#endif
          output_scatterplot (prefix + "_scatterplot.csv");
          output_fixel_count_image (prefix + "_fixel_count.mif");
          output_untracked_fixels (prefix + "_untracked_count.mif", prefix + "_untracked_amps.mif");
        }




      }
    }
  }
}


#endif
