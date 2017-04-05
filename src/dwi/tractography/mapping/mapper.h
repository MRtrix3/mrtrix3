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

#ifndef __dwi_tractography_mapping_mapper_h__
#define __dwi_tractography_mapping_mapper_h__



#include <vector>

#include "image.h"
#include "transform.h"
#include "thread_queue.h"

#include "dwi/directions/set.h"

#include "dwi/tractography/resampling/upsampler.h"
#include "dwi/tractography/streamline.h"

#include "dwi/tractography/mapping/mapper_plugins.h"
#include "dwi/tractography/mapping/mapping.h"
#include "dwi/tractography/mapping/twi_stats.h"
#include "dwi/tractography/mapping/voxel.h"

#include "math/hermite.h"


// Didn't bother making this a command-line option, since curvature contrast results were very poor regardless of smoothing
#define CURVATURE_TRACK_SMOOTHING_FWHM 10.0 // In mm






namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Mapping {




        class TrackMapperBase
        {

          public:
            template <class HeaderType>
              TrackMapperBase (const HeaderType& template_image) :
                info      (template_image),
                scanner2voxel (Transform(template_image).scanner2voxel.cast<float>()),
                map_zero  (false),
                precise   (false),
                ends_only (false),
                upsampler (1) { }

            template <class HeaderType>
              TrackMapperBase (const HeaderType& template_image, const DWI::Directions::FastLookupSet& dirs) :
                info         (template_image),
                scanner2voxel (Transform(template_image).scanner2voxel.cast<float>()),
                map_zero     (false),
                precise      (false),
                ends_only    (false),
                dixel_plugin (new DixelMappingPlugin (dirs)),
                upsampler    (1) { }

            TrackMapperBase (const TrackMapperBase&) = default;
            TrackMapperBase (TrackMapperBase&&) = default;

            EIGEN_MAKE_ALIGNED_OPERATOR_NEW  // avoid memory alignment errors in Eigen3;

            virtual ~TrackMapperBase() { }


            void set_upsample_ratio (const size_t i) { upsampler.set_ratio (i); }
            void set_map_zero (const bool i) { map_zero = i; }
            void set_use_precise_mapping (const bool i) {
              if (i && ends_only) 
                throw Exception ("Cannot do precise mapping and endpoint mapping together");
              precise = i;
            }
            void set_map_ends_only (const bool i) {
              if (i && precise) 
                throw Exception ("Cannot do precise mapping and endpoint mapping together");
              ends_only = i;
            }

            void create_dixel_plugin (const DWI::Directions::FastLookupSet& dirs)
            {
              assert (!dixel_plugin && !tod_plugin);
              dixel_plugin.reset (new DixelMappingPlugin (dirs));
            }

            void create_tod_plugin (const size_t N)
            {
              assert (!dixel_plugin && !tod_plugin);
              tod_plugin.reset (new TODMappingPlugin (N));
            }


            template <class Cont>
              bool operator() (Streamline<>& in, Cont& out) const
              {
                out.clear();
                out.index = in.index;
                out.weight = in.weight;
                if (in.empty())
                  return true;
                if (preprocess (in, out) || map_zero) {
                  upsampler (in);
                  if (precise)
                    voxelise_precise (in, out);
                  else if (ends_only)
                    voxelise_ends (in, out);
                  else
                    voxelise (in, out);
                  postprocess (in, out);
                }
                return true;
              }



          protected:
            const Header info;
            const Eigen::Transform<float,3,Eigen::AffineCompact> scanner2voxel;
            bool map_zero;
            bool precise;
            bool ends_only;

            std::shared_ptr<DixelMappingPlugin> dixel_plugin;
            std::shared_ptr<TODMappingPlugin>   tod_plugin;


            // Specialist version of voxelise() is provided for the SetVoxel container:
            //   this is the simplest form of track mapping, and don't want to slow it down
            //   by unnecessarily computing tangents
            // Second version does nothing fancy, but includes computation of the
            //   streamline tangent, and forces normalisation of the contribution from
            //   each streamline to each voxel it traverses
            // Third version is the 'precise' mapping as described in the SIFT paper
            // Fourth method only maps the streamline endpoints
            void voxelise (const Streamline<>&, SetVoxel&) const;
            template <class Cont> void voxelise         (const Streamline<>&, Cont&) const;
            template <class Cont> void voxelise_precise (const Streamline<>&, Cont&) const;
            template <class Cont> void voxelise_ends    (const Streamline<>&, Cont&) const;

            virtual bool preprocess  (const Streamline<>& tck, SetVoxelExtras& out) const { out.factor = 1.0; return true; }
            virtual void postprocess (const Streamline<>& tck, SetVoxelExtras& out) const { }

            // Used by voxelise() and voxelise_precise() to increment the relevant set
            inline void add_to_set (SetVoxel&   , const Eigen::Vector3i&, const Eigen::Vector3f&, const float) const;
            inline void add_to_set (SetVoxelDEC&, const Eigen::Vector3i&, const Eigen::Vector3f&, const float) const;
            inline void add_to_set (SetVoxelDir&, const Eigen::Vector3i&, const Eigen::Vector3f&, const float) const;
            inline void add_to_set (SetDixel&   , const Eigen::Vector3i&, const Eigen::Vector3f&, const float) const;
            inline void add_to_set (SetVoxelTOD&, const Eigen::Vector3i&, const Eigen::Vector3f&, const float) const;

            DWI::Tractography::Resampling::Upsampler upsampler;

        };



        template <class Cont>
          void TrackMapperBase::voxelise (const Streamline<>& tck, Cont& output) const
          {

            auto prev = tck.cbegin();
            const auto last = tck.cend() - 1;

            Eigen::Vector3i vox;
            for (auto i = tck.cbegin(); i != last; ++i) {
              vox = round (scanner2voxel * (*i));
              if (check (vox, info)) {
                const auto dir = (*(i+1) - *prev).normalized();
                if (dir.allFinite())
                  add_to_set (output, vox, dir, 1.0f);
              }
              prev = i;
            }

            vox = round (scanner2voxel * (*last));
            if (check (vox, info)) {
              const auto dir = (*last - *prev).normalized();
              if (dir.allFinite())
                add_to_set (output, vox, dir, 1.0f);
            }

            for (auto& i : output) 
              i.normalize();

          }






        template <class Cont>
          void TrackMapperBase::voxelise_precise (const Streamline<>& tck, Cont& out) const
          {
            using PointF = Eigen::Vector3f;

            const float accuracy = Math::pow2 (0.005 * std::min (info.spacing (0), std::min (info.spacing (1), info.spacing (2))));

            if (tck.size() < 2)
              return;

            Math::Hermite<float> hermite (0.1);

            const PointF tck_proj_front = (tck[      0     ] * 2.0) - tck[     1      ];
            const PointF tck_proj_back  = (tck[tck.size()-1] * 2.0) - tck[tck.size()-2];

            unsigned int p = 0;
            PointF p_voxel_exit = tck.front();
            float mu = 0.0;
            bool end_track = false;
            auto next_voxel = round (scanner2voxel * tck.front());

            do {

              const PointF p_voxel_entry (p_voxel_exit);
              PointF p_prev (p_voxel_entry);
              float length = 0.0;
              const auto this_voxel = next_voxel;

              while ((p != tck.size()) && ((next_voxel = round (scanner2voxel * tck[p])) == this_voxel)) {
                length += (p_prev - tck[p]).norm();
                p_prev = tck[p];
                ++p;
                mu = 0.0;
              }

              if (p == tck.size()) {
                p_voxel_exit = tck.back();
                end_track = true;
              } 
              else {

                float mu_min = mu;
                float mu_max = 1.0;

                const PointF* p_one  = (p == 1)              ? &tck_proj_front : &tck[p - 2];
                const PointF* p_four = (p == tck.size() - 1) ? &tck_proj_back  : &tck[p + 1];

                PointF p_min = p_prev;
                PointF p_max = tck[p];

                while ((p_min - p_max).squaredNorm() > accuracy) {

                  mu = 0.5 * (mu_min + mu_max);
                  hermite.set (mu);
                  const PointF p_mu = hermite.value (*p_one, tck[p - 1], tck[p], *p_four);
                  const Eigen::Vector3i mu_voxel = round (scanner2voxel * p_mu);

                  if (mu_voxel == this_voxel) {
                    mu_min = mu;
                    p_min = p_mu;
                  }
                  else {
                    mu_max = mu;
                    p_max = p_mu;
                    next_voxel = mu_voxel;
                  }

                }
                p_voxel_exit = p_max;

              }

              length += (p_prev - p_voxel_exit).norm();
              PointF traversal_vector = (p_voxel_exit - p_voxel_entry).normalized();
              if (std::isfinite (traversal_vector[0]) && check (this_voxel, info))
                add_to_set (out, this_voxel, traversal_vector, length);

            } while (!end_track);

          }



        template <class Cont>
          void TrackMapperBase::voxelise_ends (const Streamline<>& tck, Cont& out) const
          {
            for (size_t end = 0; end != 2; ++end) {
              const auto vox = round (scanner2voxel * (end ? tck.back() : tck.front()));
              if (check (vox, info)) {
                Streamline<>::point_type dir { NAN, NAN, NAN };
                if (tck.size() > 1)
                  dir = (end ? (tck[tck.size()-1] - tck[tck.size()-2]) : (tck[0] - tck[1])).normalized();
                add_to_set (out, vox, dir, 1.0f);
              }
            }
          }




        // These are inlined to make as fast as possible
        inline void TrackMapperBase::add_to_set (SetVoxel&    out, const Eigen::Vector3i& v, const Eigen::Vector3f& d, const float l) const
        {
          out.insert (v, l);
        }
        inline void TrackMapperBase::add_to_set (SetVoxelDEC& out, const Eigen::Vector3i& v, const Eigen::Vector3f& d, const float l) const
        {
          out.insert (v, d, l);
        }
        inline void TrackMapperBase::add_to_set (SetVoxelDir& out, const Eigen::Vector3i& v, const Eigen::Vector3f& d, const float l) const
        {
          out.insert (v, d, l);
        }
        inline void TrackMapperBase::add_to_set (SetDixel&    out, const Eigen::Vector3i& v, const Eigen::Vector3f& d, const float l) const
        {
          assert (dixel_plugin);
          const size_t bin = (*dixel_plugin) (d);
          out.insert (v, bin, l);
        }
        inline void TrackMapperBase::add_to_set (SetVoxelTOD& out, const Eigen::Vector3i& v, const Eigen::Vector3f& d, const float l) const
        {
          assert (tod_plugin);
          Eigen::VectorXf sh;
          (*tod_plugin) (sh, d);
          out.insert (v, sh, l);
        }












        class TrackMapperTWI : public TrackMapperBase
        {
          public:
            template <class HeaderType>
              TrackMapperTWI (const HeaderType& template_image, const contrast_t c, const tck_stat_t s) :
                TrackMapperBase       (template_image),
                contrast              (c),
                track_statistic       (s) { }

            TrackMapperTWI (const TrackMapperTWI& that) :
              TrackMapperBase       (static_cast<const TrackMapperBase&> (that)),
              contrast              (that.contrast),
              track_statistic       (that.track_statistic),
              vector_data           (that.vector_data) {
                if (that.image_plugin) {
                  if (contrast == SCALAR_MAP || contrast == SCALAR_MAP_COUNT)
                    image_plugin.reset (new TWIScalarImagePlugin (*dynamic_cast<TWIScalarImagePlugin*> (that.image_plugin.get())));
                  else if (contrast == FOD_AMP)
                    image_plugin.reset (new TWIFODImagePlugin    (*dynamic_cast<TWIFODImagePlugin*>    (that.image_plugin.get())));
                  else
                    throw Exception ("Copy-constructing TrackMapperTWI with unknown image plugin");
                }
              }

            EIGEN_MAKE_ALIGNED_OPERATOR_NEW  // avoid memory alignment errors in Eigen3;

            void add_scalar_image (const std::string&);
            void add_fod_image    (const std::string&);
            void add_vector_data  (const std::string&);


          protected:
            const contrast_t contrast;
            const tck_stat_t track_statistic;

            // Members for when the contribution of a track is not constant along its length
            mutable std::vector<float> factors;
            void load_factors (const Streamline<>&) const;

            // Member for incorporating additional information from an external image into the TWI process
            std::unique_ptr<TWIImagePluginBase> image_plugin;

            // Member for incorporating contrast from an external vector data file into the TWI process
            std::shared_ptr<Eigen::VectorXf> vector_data;


          private:

            virtual void set_factor (const Streamline<>&, SetVoxelExtras&) const;

            // Overload virtual function
            virtual bool preprocess (const Streamline<>& tck, SetVoxelExtras& out) const { set_factor (tck, out); return out.factor; }



        };






      }
    }
  }
}

#endif



