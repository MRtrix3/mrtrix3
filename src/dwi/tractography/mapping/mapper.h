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

#ifndef __dwi_tractography_mapping_mapper_h__
#define __dwi_tractography_mapping_mapper_h__


#include "image.h"
#include "thread_queue.h"
#include "transform.h"
#include "types.h"
#include "fixel/dataset.h"
#include "math/sphere/set/adjacency.h"

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
                info          (template_image),
                scanner2voxel (Transform(template_image).scanner2voxel.cast<float>()),
                map_zero      (false),
                algorithm     (algorithm_t::NEAREST),
                upsampler     (1) { }

            template <class HeaderType>
            TrackMapperBase (const HeaderType& template_image, const Math::Sphere::Set::Assigner& dirs) :
                info          (template_image),
                scanner2voxel (Transform(template_image).scanner2voxel.cast<float>()),
                map_zero      (false),
                algorithm     (algorithm_t::NEAREST),
                dixel_plugin  (new DixelMappingPlugin (dirs)),
                upsampler     (1) { }

            template <class HeaderType>
            TrackMapperBase (const HeaderType& template_image, const MR::Fixel::Dataset& fixels) :
                info          (template_image),
                scanner2voxel (Transform(template_image).scanner2voxel.cast<float>()),
                map_zero      (false),
                algorithm     (algorithm_t::NEAREST),
                fixel_plugin  (new FixelMappingPlugin (fixels)),
                upsampler     (1) { }

            TrackMapperBase (const TrackMapperBase& that) :
                info (that.info),
                scanner2voxel (that.scanner2voxel),
                map_zero (that.map_zero),
                algorithm (that.algorithm),
                dixel_plugin (that.dixel_plugin),
                tod_plugin (that.tod_plugin ? new TODMappingPlugin (*that.tod_plugin) : nullptr),
                fixel_plugin (that.fixel_plugin ? new FixelMappingPlugin (*that.fixel_plugin) : nullptr),
                upsampler (that.upsampler) { }

            TrackMapperBase (TrackMapperBase&&) = default;


            virtual ~TrackMapperBase() { }


            void set_upsample_ratio (const size_t i) { upsampler.set_ratio (i); }
            void set_map_zero (const bool i) { map_zero = i; }
            void set_algorithm (const algorithm_t i) { algorithm = i; }

            void create_dixel_plugin (const Math::Sphere::Set::Assigner& dirs)
            {
              assert (!has_plugin());
              dixel_plugin.reset (new DixelMappingPlugin (dirs));
            }

            void create_tod_plugin (const size_t N)
            {
              assert (!has_plugin());
              tod_plugin.reset (new TODMappingPlugin (N));
            }

            void create_fixel_plugin (const MR::Fixel::Dataset& fixels)
            {
              assert (!has_plugin());
              fixel_plugin.reset (new FixelMappingPlugin (fixels));
            }


            template <class Cont>
              bool operator() (const Streamline<>& in, Cont& out) const
              {
                out.clear();
                out.index = in.get_index();
                out.weight = in.weight;
                if (in.empty())
                  return true;
                if (preprocess (in, out) || map_zero) {
                  Streamline<> temp;
                  upsampler (in, temp);
                  switch (algorithm) {
                    case algorithm_t::BLURRED:   voxelise_blurred (temp, out); break;
                    case algorithm_t::ENDPOINTS: voxelise_ends    (temp, out); break;
                    case algorithm_t::NEAREST:   voxelise_nearest (temp, out); break;
                    case algorithm_t::PRECISE:   voxelise_precise (temp, out); break;
                  }
                  postprocess (temp, out);
                }
                return true;
              }



          protected:
            const Header info;
            const Eigen::Transform<float,3,Eigen::AffineCompact> scanner2voxel;
            bool map_zero;
            algorithm_t algorithm;

            // TODO Some of these may prefer to have one instance per thread...?
            // Particularly the fixel plugin needs a separate index image accessor per thread...
            std::shared_ptr<DixelMappingPlugin> dixel_plugin;
            std::unique_ptr<TODMappingPlugin>   tod_plugin;
            std::unique_ptr<FixelMappingPlugin> fixel_plugin;
            bool has_plugin() const { return dixel_plugin || tod_plugin || fixel_plugin; }


            // Specialist version of voxelise() is provided for the SetVoxel container:
            //   this is the simplest form of track mapping, and don't want to slow it down
            //   by unnecessarily computing tangents
            // Second version does nothing fancy, but includes computation of the
            //   streamline tangent, and forces normalisation of the contribution from
            //   each streamline to each voxel it traverses
            // Third version is the 'precise' mapping as described in the SIFT paper
            // Fourth method only maps the streamline endpoints
            void voxelise_nearest (const Streamline<>&, Set<Voxel>&) const;
            template <class Cont> void voxelise_blurred (const Streamline<>&, Cont&) const;
            template <class Cont> void voxelise_ends    (const Streamline<>&, Cont&) const;
            template <class Cont> void voxelise_nearest (const Streamline<>&, Cont&) const;
            template <class Cont> void voxelise_precise (const Streamline<>&, Cont&) const;

            virtual bool preprocess  (const Streamline<>& tck, SetExtras& out) const { out.factor = 1.0; return true; }
            virtual void postprocess (const Streamline<>& tck, SetExtras& out) const { }

            // Used by voxelise() and voxelise_precise() to increment the relevant set
            inline void add_to_set (Set<Voxel>&   , const Eigen::Vector3i&, const Eigen::Vector3d&, const default_type) const;
            inline void add_to_set (Set<VoxelDEC>&, const Eigen::Vector3i&, const Eigen::Vector3d&, const default_type) const;
            inline void add_to_set (Set<VoxelDir>&, const Eigen::Vector3i&, const Eigen::Vector3d&, const default_type) const;
            inline void add_to_set (Set<Dixel>&   , const Eigen::Vector3i&, const Eigen::Vector3d&, const default_type) const;
            inline void add_to_set (Set<VoxelTOD>&, const Eigen::Vector3i&, const Eigen::Vector3d&, const default_type) const;
            inline void add_to_set (Set<Fixel>&,    const Eigen::Vector3i&, const Eigen::Vector3d&, const default_type) const;

            DWI::Tractography::Resampling::Upsampler upsampler;

        };



        template <class Cont>
          void TrackMapperBase::voxelise_blurred (const Streamline<>& tck, Cont& out) const
          {

            using point_type = Streamline<>::point_type;
            using value_type = Streamline<>::value_type;

            const default_type accuracy = Math::pow2 (0.005 * std::min (info.spacing (0), std::min (info.spacing (1), info.spacing (2))));

            if (tck.size() < 2)
              return;

            Math::Hermite<value_type> hermite (0.1);

            const point_type tck_proj_front = (tck[      0     ] * 2.0) - tck[     1      ];
            const point_type tck_proj_back  = (tck[tck.size()-1] * 2.0) - tck[tck.size()-2];

            unsigned int next_index = 1;
            point_type p_prev = tck.front();
            default_type mu = default_type(0);
            Eigen::Vector3i lowercorner_voxel ((scanner2voxel * tck.front()).array().floor().cast<int>());

            do {

              point_type p_next = tck[next_index];
              Eigen::Vector3i next_lowercorner_voxel ((scanner2voxel * p_next).array().floor().cast<int>());
              bool increment_next_index = false;
              if (next_lowercorner_voxel == lowercorner_voxel) {
                increment_next_index = true;
                mu = 0.0;
              } else {

                // Binary search to within tolerance
                // Need to retain the voxel lower index of the next segment
                //   based on the last sample point that is not the same as the current segment
                default_type mu_min = mu;
                default_type mu_max = default_type(1);

                const point_type* p_one  = (next_index == 1)              ? &tck_proj_front : &tck[next_index - 2];
                const point_type* p_four = (next_index == tck.size() - 1) ? &tck_proj_back  : &tck[next_index + 1];

                point_type p_min = p_prev;
                point_type p_max = tck[next_index];

                while ((p_min - p_max).squaredNorm() > accuracy) {

                  mu = 0.5 * (mu_min + mu_max);
                  hermite.set (mu);
                  p_next = hermite.value (*p_one, tck[next_index - 1], tck[next_index], *p_four);
                  const Eigen::Vector3i mu_lowercorner_voxel ((scanner2voxel * p_next).array().floor().cast<int>());

                  if (mu_lowercorner_voxel == lowercorner_voxel) {
                    mu_min = mu;
                    p_min = p_next;
                  } else {
                    mu_max = mu;
                    p_max = p_next;
                    next_lowercorner_voxel = mu_lowercorner_voxel;
                  }

                }

              }

              // Contribute segment to all eight voxels in neighbourhood
              const point_type p_mean = 0.5 * (p_prev + p_next);
              const point_type v_mean(scanner2voxel * p_mean);
              const Eigen::Array<default_type, 3, 1> mus ({
                  v_mean[0] - float(lowercorner_voxel[0]),
                  v_mean[1] - float(lowercorner_voxel[1]),
                  v_mean[2] - float(lowercorner_voxel[2]) });
              const default_type length = (p_next - p_prev).norm();
              const Eigen::Vector3d traversal_vector = (p_next - p_prev).cast<default_type>().normalized();
              if (traversal_vector.squaredNorm() && length > value_type(0)) {
                Eigen::Vector3i offset (Eigen::Vector3i::Zero());
                for (offset[2] = 0; offset[2] != 2; ++offset[2]) {
                  for (offset[1] = 0; offset[1] != 2; ++offset[1]) {
                    for (offset[0] = 0; offset[0] != 2; ++offset[0]) {
                      const auto voxel = lowercorner_voxel + offset;
                      if (check(voxel, info)) {
                        const default_type weight = (offset[0] ? mus[0] : (value_type(1) - mus[0])) \
                                                    * (offset[1] ? mus[1] : (value_type(1) - mus[1])) \
                                                    * (offset[2] ? mus[2] : (value_type(1) - mus[2]));
                        add_to_set (out, voxel, traversal_vector, weight * length);
                      }
                    }
                  }
                }

              } else {
                assert (false);
              }

              // Get ready for the next segment
              // Also: If we have just generated a segment,
              //   and that segment lies entirely within the same region,
              //   and we're already pointing at the streamline endpoint,
              //
              p_prev = p_next;
              lowercorner_voxel = next_lowercorner_voxel;
              if (increment_next_index)
                ++next_index;

            } while (next_index < tck.size());

          }



        template <class Cont>
          void TrackMapperBase::voxelise_ends (const Streamline<>& tck, Cont& out) const
          {
            if (!tck.size())
              return;
            if (tck.size() == 1) {
              const auto vox = round (scanner2voxel * tck.front());
              if (check (vox, info))
                add_to_set (out, vox, Eigen::Vector3d(NaN, NaN, NaN), 1.0);
              return;
            }
            for (size_t end = 0; end != 2; ++end) {
              const auto vox = round (scanner2voxel * (end ? tck.back() : tck.front()));
              if (check (vox, info)) {
                Eigen::Vector3d dir { NaN, NaN, NaN };
                if (tck.size() > 1)
                  dir = (end ? (tck[tck.size()-1] - tck[tck.size()-2]) : (tck[0] - tck[1])).cast<default_type>().normalized();
                add_to_set (out, vox, dir, 1.0);
              }
            }
          }



        template <class Cont>
          void TrackMapperBase::voxelise_nearest (const Streamline<>& tck, Cont& output) const
          {

            auto prev = tck.cbegin();
            const auto last = tck.cend() - 1;

            Eigen::Vector3i vox;
            for (auto i = tck.cbegin(); i != last; ++i) {
              vox = round (scanner2voxel * (*i));
              if (check (vox, info)) {
                const Eigen::Vector3d dir = (*(i+1) - *prev).cast<default_type>().normalized();
                if (dir.allFinite() && !dir.isZero())
                  add_to_set (output, vox, dir, 1.0);
              }
              prev = i;
            }

            vox = round (scanner2voxel * (*last));
            if (check (vox, info)) {
              const Eigen::Vector3d dir = (*last - *prev).cast<default_type>().normalized();
              if (dir.allFinite() && !dir.isZero())
                add_to_set (output, vox, dir, 1.0);
            }

            for (auto& i : output)
              i.IntersectionLength::normalize();

          }





        template <class Cont>
          void TrackMapperBase::voxelise_precise (const Streamline<>& tck, Cont& out) const
          {

            using point_type = Streamline<>::point_type;
            using value_type = Streamline<>::value_type;

            const default_type accuracy = Math::pow2 (0.005 * std::min (info.spacing (0), std::min (info.spacing (1), info.spacing (2))));

            if (tck.size() < 2)
              return;

            Math::Hermite<value_type> hermite (0.1);

            const point_type tck_proj_front = (tck[      0     ] * 2.0) - tck[     1      ];
            const point_type tck_proj_back  = (tck[tck.size()-1] * 2.0) - tck[tck.size()-2];

            unsigned int p = 0;
            point_type p_voxel_exit = tck.front();
            default_type mu = 0.0;
            bool end_track = false;
            auto next_voxel = round (scanner2voxel * tck.front());

            do {

              const point_type p_voxel_entry (p_voxel_exit);
              point_type p_prev (p_voxel_entry);
              default_type length = 0.0;
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

                default_type mu_min = mu;
                default_type mu_max = 1.0;

                const point_type* p_one  = (p == 1)              ? &tck_proj_front : &tck[p - 2];
                const point_type* p_four = (p == tck.size() - 1) ? &tck_proj_back  : &tck[p + 1];

                point_type p_min = p_prev;
                point_type p_max = tck[p];

                while ((p_min - p_max).squaredNorm() > accuracy) {

                  mu = 0.5 * (mu_min + mu_max);
                  hermite.set (mu);
                  const point_type p_mu = hermite.value (*p_one, tck[p - 1], tck[p], *p_four);
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
              const Eigen::Vector3d traversal_vector = (p_voxel_exit - p_voxel_entry).cast<default_type>().normalized();
              if (traversal_vector.squaredNorm() && length && check (this_voxel, info))
                add_to_set (out, this_voxel, traversal_vector, length);

            } while (!end_track);

          }








        // These are inlined to make as fast as possible
        inline void TrackMapperBase::add_to_set (Set<Voxel>&    out, const Eigen::Vector3i& v, const Eigen::Vector3d& d, const default_type l) const
        {
          const Voxel voxel (v, l);
          out.insert (voxel);
        }
        inline void TrackMapperBase::add_to_set (Set<VoxelDEC>& out, const Eigen::Vector3i& v, const Eigen::Vector3d& d, const default_type l) const
        {
          const VoxelDEC voxel (v, d, l);
          out.insert (voxel);
        }
        inline void TrackMapperBase::add_to_set (Set<VoxelDir>& out, const Eigen::Vector3i& v, const Eigen::Vector3d& d, const default_type l) const
        {
          const VoxelDir voxel (v, d, l);
          out.insert (voxel);
        }
        inline void TrackMapperBase::add_to_set (Set<Dixel>&    out, const Eigen::Vector3i& v, const Eigen::Vector3d& d, const default_type l) const
        {
          assert (dixel_plugin);
          const Math::Sphere::Set::index_type bin = (*dixel_plugin) (d);
          const Dixel dixel (v, bin, l);
          out.insert (dixel);
        }
        inline void TrackMapperBase::add_to_set (Set<VoxelTOD>& out, const Eigen::Vector3i& v, const Eigen::Vector3d& d, const default_type l) const
        {
          assert (tod_plugin);
          (*tod_plugin) (d);
          const VoxelTOD voxel (v, (*tod_plugin)(), l);
          out.insert (voxel);
        }
        inline void TrackMapperBase::add_to_set (Set<Fixel>& out, const Eigen::Vector3i& v, const Eigen::Vector3d& d, const default_type l) const
        {
          assert (fixel_plugin);
          const MR::Fixel::index_type fixel_index = (*fixel_plugin) (v, d);
          if (fixel_index != fixel_plugin->nfixels()) {
            const Fixel fixel (fixel_index, l);
            out.insert (fixel);
          }
        }












        class TrackMapperTWI : public TrackMapperBase
        {
          public:
            template <class HeaderType>
            TrackMapperTWI (const HeaderType& template_image, const contrast_t c, const tck_stat_t s) :
                TrackMapperBase (template_image),
                contrast        (c),
                track_statistic (s) { }

            TrackMapperTWI (const TrackMapperTWI& that) :
                TrackMapperBase (static_cast<const TrackMapperBase&> (that)),
                contrast        (that.contrast),
                track_statistic (that.track_statistic),
                vector_data     (that.vector_data)
            {
              if (that.image_plugin)
                image_plugin.reset (that.image_plugin->clone());
            }


            void add_scalar_image (const std::string&);
            void set_backtrack();
            void add_fod_image    (const std::string&);
            void add_twdfc_static_image  (Image<float>&);
            void add_twdfc_dynamic_image (Image<float>&, const vector<float>&, const ssize_t);
            void add_vector_data  (const std::string&);


          protected:
            const contrast_t contrast;
            const tck_stat_t track_statistic;

            // Members for when the contribution of a track is not constant along its length
            mutable vector<default_type> factors;
            void load_factors (const Streamline<>&) const;

            // Member for incorporating additional information from an external image into the TWI process
            std::unique_ptr<TWIImagePluginBase> image_plugin;

            // Member for incorporating contrast from an external vector data file into the TWI process
            std::shared_ptr<Eigen::VectorXf> vector_data;


          private:

            virtual void set_factor (const Streamline<>&, SetExtras&) const;

            // Overload virtual function
            virtual bool preprocess (const Streamline<>& tck, SetExtras& out) const { set_factor (tck, out); return out.factor; }



        };






      }
    }
  }
}

#endif



