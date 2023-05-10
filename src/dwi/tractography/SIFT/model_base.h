/* Copyright (c) 2008-2023 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#ifndef __dwi_tractography_sift_model_base_h__
#define __dwi_tractography_sift_model_base_h__

#include "app.h"
#include "image.h"
#include "thread_queue.h"
#include "algo/copy.h"
#include "fixel/helpers.h"
#include "file/path.h"
#include "file/utils.h"

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
              dir ({NaN, NaN, NaN}) { }

            FixelBase (const default_type amp) :
              FOD (amp),
              TD (0.0),
              weight (1.0),
              dir ({NaN, NaN, NaN}) { }

            FixelBase (const default_type amp, const Eigen::Vector3d& d) :
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
            const Eigen::Vector3d& get_dir() const { return dir; }

            void       scale_FOD  (const default_type factor) { FOD *= factor; }
            void       set_weight (const default_type w)      { weight = w; }
            FixelBase& operator+= (const default_type length) { TD += length; return *this; }

            void clear_TD() { TD = 0.0; }

            default_type get_diff (const default_type mu) const { return ((TD * mu) - FOD); }
            default_type get_cost (const default_type mu) const { return get_cost_unweighted (mu) * weight; }


          protected:
            default_type FOD, TD, weight;
            Eigen::Vector3d dir;

            default_type get_cost_unweighted (const default_type mu) const { return Math::pow2 (get_diff (mu)); }

        };






        // Templated Fixel class should derive from FixelBase to ensure that it has adequate functionality
        // This class stores the necessary fixel information (including streamline densities), but does not retain the
        //   list of fixels traversed by each streamline. If this information is necessary, use the Model class (model.h)

        template <class Fixel>
        class ModelBase : public Mapping::Fixel_TD_map<Fixel>
        {

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
            void initialise_debug_image_output (const std::string&) const;
            void output_all_debug_images (const std::string&, const std::string&) const;

            using Mapping::Fixel_TD_map<Fixel>::begin;


          protected:
            using Fixel_map<Fixel>::accessor;
            using Fixel_map<Fixel>::fixels;
            using Mapping::Fixel_TD_map<Fixel>::dirs;

            Image<float> act_5tt, proc_mask;
            default_type FOD_sum, TD_sum;
            bool have_null_lobes;

            // The definitions of these functions are located in dwi/tractography/SIFT/output.h
            void output_target_voxel (const std::string&) const;
            void output_target_sh (const std::string&) const;
            void output_target_fixel (const std::string&) const;
            void output_tdi_voxel (const std::string&) const;
            void output_tdi_null_lobes (const std::string&) const;
            void output_tdi_sh (const std::string&) const;
            void output_tdi_fixel (const std::string&) const;
            void output_errors_voxel (const std::string&, const std::string&, const std::string&, const std::string&) const;
            void output_errors_fixel (const std::string&, const std::string&, const std::string&) const;
            void output_scatterplot (const std::string&) const;
            void output_fixel_count_image (const std::string&) const;

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
        void ModelBase<Fixel>::initialise_debug_image_output (const std::string& dirpath) const
        {
          File::mkdir (dirpath);
#ifdef SIFT_MODEL_OUTPUT_FIXEL_IMAGES
          Header H_index (this->header());
          H_index.ndim() = 4;
          H_index.size(3) = 2;
          H_index.datatype() = DataType::native (DataType::UInt64);
          Header H_directions;
          H_directions.ndim() = 3;
          H_directions.size(0) = fixels.size();
          H_directions.size(1) = 3;
          H_directions.size(2) = 1;
          H_directions.stride(0) = 2;
          H_directions.stride(1) = 1;
          H_directions.stride(2) = 3;
          H_directions.spacing(0) = H_directions.spacing(1) = H_directions.spacing(2) = 1.0;
          H_directions.transform().setIdentity();
          H_directions.datatype() = DataType::native (DataType::from<float>());
          Image<uint64_t> index_image = Image<uint64_t>::create (Path::join (dirpath, "index.mif"), H_index);
          Image<float> directions_image = Image<float>::create (Path::join (dirpath, "directions.mif"), H_directions);
          VoxelAccessor v (accessor());
          for (auto l = Loop (v) (v, index_image); l; ++l) {
            if (v.value()) {
              index_image.index(3) = 0;
              index_image.value() = (*v.value()).num_fixels();
              index_image.index(3) = 1;
              index_image.value() = (*v.value()).first_index();
            }
          }
          for (size_t i = 0; i != fixels.size(); ++i) {
            directions_image.index(0) = i;
            directions_image.row(1) = fixels[i].get_dir();
          }
#endif

          // These images do not change between before and after filtering
          output_target_voxel (Path::join (dirpath, "target_voxel.mif"));
#ifdef SIFT_MODEL_OUTPUT_SH_IMAGES
          output_target_sh (Path::join (dirpath, "target_sh.mif"));
#endif
#ifdef SIFT_MODEL_OUTPUT_FIXEL_IMAGES
          output_target_fixel (Path::join (dirpath, "target_fixel.mif"));
#endif
          output_fixel_count_image (Path::join (dirpath, "trackcount_fixel.mif"));
        }



        template <class Fixel>
        void ModelBase<Fixel>::output_all_debug_images (const std::string& dirpath, const std::string& prefix) const
        {
          output_tdi_voxel (Path::join (dirpath, prefix + "_tdi_voxel.mif"));
          if (have_null_lobes)
            output_tdi_null_lobes (Path::join (dirpath, prefix + "_tdi_nulllobes.mif"));
#ifdef SIFT_MODEL_OUTPUT_SH_IMAGES
          output_tdi_sh (Path::join (dirpath, prefix + "_tdi_sh.mif"));
#endif
#ifdef SIFT_MODEL_OUTPUT_FIXEL_IMAGES
          output_tdi_fixel (Path::join (dirpath, prefix + "_tdi_fixel.mif"));
#endif

          output_errors_voxel (dirpath, prefix + "_maxabsdiff_voxel.mif", prefix + "_diff_voxel.mif", prefix + "_cost_voxel.mif");
#ifdef SIFT_MODEL_OUTPUT_FIXEL_IMAGES
          output_errors_fixel (dirpath, prefix + "_diff_fixel.mif", prefix + "_cost_fixel.mif");
#endif
          output_scatterplot (Path::join (dirpath, prefix + "_scatterplot.csv"));
        }




      }
    }
  }
}


#endif
