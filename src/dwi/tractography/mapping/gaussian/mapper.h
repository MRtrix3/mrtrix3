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

#ifndef __dwi_tractography_mapping_gaussian_mapper_h__
#define __dwi_tractography_mapping_gaussian_mapper_h__


#include "image.h"

#include "dwi/tractography/mapping/mapper.h"
#include "dwi/tractography/mapping/twi_stats.h"
#include "dwi/tractography/mapping/gaussian/voxel.h"



namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Mapping {
        namespace Gaussian {




          class TrackMapper : public Mapping::TrackMapperTWI
          {

            using BaseMapper = Mapping::TrackMapperTWI;

            public:
            template <class HeaderType>
              TrackMapper (const HeaderType& template_image, const contrast_t c) :
              BaseMapper (template_image, c, GAUSSIAN),
              gaussian_denominator  (0.0) {
                assert (c == SCALAR_MAP || c == SCALAR_MAP_COUNT || c == FOD_AMP || c == CURVATURE);
              }

            TrackMapper (const TrackMapper&) = default;
            ~TrackMapper() { }


            void set_gaussian_FWHM (const default_type FWHM)
            {
              if (track_statistic != GAUSSIAN)
                throw Exception ("Cannot set Gaussian FWHM unless the track statistic is Gaussian");
              const default_type theta = FWHM / (2.0 * std::sqrt (2.0 * std::log (2.0)));
              gaussian_denominator = 2.0 * Math::pow2 (theta);
            }


            // Have to re-define the functor here, so that the alternative voxelise() methods can be called
            //   (Can't define these virtual because they're templated)
            // Also means that the call to run_queue() must cast to Gaussian::TrackMapper rather than using base class
            template <class Cont>
              bool operator() (Streamline<>& in, Cont& out) const
              {
                out.clear();
                out.index = in.get_index();
                out.weight = in.weight;
                if (in.empty())
                  return true;
                if (preprocess (in, out) || map_zero) {
                  Streamline<> temp;
                  upsampler (in, temp);
                  if (precise)
                    voxelise_precise (temp, out);
                  else if (ends_only)
                    voxelise_ends (temp, out);
                  else
                    voxelise (temp, out);
                  postprocess (temp, out);
                }
                return true;
              }




            protected:
            default_type gaussian_denominator;
            void gaussian_smooth_factors (const Streamline<>&) const;

            // Overload corresponding functions in TrackMapperTWI
            void set_factor (const Streamline<>& tck, SetExtras& out) const;
            bool preprocess (const Streamline<>& tck, SetExtras& out) const { set_factor (tck, out); return true; }

            // Three versions of voxelise() function, just as in base class: difference is that here the
            //   corresponding TWI factor for each voxel mapping must be determined and passed to add_to_set()
            template <class Cont> void voxelise         (const Streamline<>&, Cont&) const;
            template <class Cont> void voxelise_precise (const Streamline<>&, Cont&) const;
            template <class Cont> void voxelise_ends    (const Streamline<>&, Cont&) const;

            inline void add_to_set (Set<Voxel>&   , const Eigen::Vector3i&, const Eigen::Vector3d&, const default_type, const default_type) const;
            inline void add_to_set (Set<VoxelDEC>&, const Eigen::Vector3i&, const Eigen::Vector3d&, const default_type, const default_type) const;
            inline void add_to_set (Set<Dixel>&   , const Eigen::Vector3i&, const Eigen::Vector3d&, const default_type, const default_type) const;
            inline void add_to_set (Set<VoxelTOD>&, const Eigen::Vector3i&, const Eigen::Vector3d&, const default_type, const default_type) const;
            inline void add_to_set (Set<Fixel>&,    const Eigen::Vector3i&, const Eigen::Vector3d&, const default_type, const default_type) const;

            // Convenience function to convert from streamline position index to a linear-interpolated
            //   factor value (TrackMapperTWI member field factors[] only contains one entry per pre-upsampled point)
            inline default_type tck_index_to_factor (const size_t) const;


          };



          template <class Cont>
            void TrackMapper::voxelise (const Streamline<>& tck, Cont& output) const
            {

              size_t prev = 0;
              const size_t last = tck.size() - 1;

              Eigen::Vector3i vox;
              for (size_t i = 0; i != last; ++i) {
                vox = round (scanner2voxel * tck[i]);
                if (check (vox, info)) {
                  const Eigen::Vector3d dir ((tck[i+1] - tck[prev]).cast<default_type>().normalized());
                  const default_type factor = tck_index_to_factor (i);
                  add_to_set (output, vox, dir, 1.0, factor);
                }
                prev = i;
              }

              vox = round (scanner2voxel * tck[last]);
              if (check (vox, info)) {
                const Eigen::Vector3d dir ((tck[last] - tck[prev]).cast<default_type>().normalized());
                const default_type factor = tck_index_to_factor (last);
                add_to_set (output, vox, dir, 1.0f, factor);
              }

              for (auto& i : output)
                i.IntersectionLength::normalize();

            }






          template <class Cont>
            void TrackMapper::voxelise_precise (const Streamline<>& tck, Cont& out) const
            {

              using point_type = Streamline<>::point_type;
              using value_type = Streamline<>::value_type;


              static const default_type accuracy = Math::pow2 (0.005 * std::min (info.spacing (0), std::min (info.spacing (1), info.spacing (2))));

              if (tck.size() < 2)
                return;

              Math::Hermite<value_type> hermite (0.1);

              const point_type tck_proj_front = (tck[      0     ] * 2.0) - tck[     1      ];
              const point_type tck_proj_back  = (tck[tck.size()-1] * 2.0) - tck[tck.size()-2];

              unsigned int p = 0;
              point_type p_voxel_exit = tck.front();
              default_type mu = 0.0;
              bool end_track = false;
              Eigen::Vector3i next_voxel (round (scanner2voxel * tck.front()));

              do {

                const point_type p_voxel_entry (p_voxel_exit);
                point_type p_prev (p_voxel_entry);
                default_type length = 0.0;
                const default_type index_voxel_entry = default_type(p) + mu;
                const Eigen::Vector3i this_voxel = next_voxel;

                while ((p != tck.size()) && ((next_voxel = round (scanner2voxel * tck[p])) == this_voxel)) {
                  length += (p_prev - tck[p]).norm();
                  p_prev = tck[p];
                  ++p;
                  mu = 0.0;
                }

                if (p == tck.size()) {
                  p_voxel_exit = tck.back();
                  end_track = true;
                } else {

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
                    } else {
                      mu_max = mu;
                      p_max = p_mu;
                      next_voxel = mu_voxel;
                    }

                  }
                  p_voxel_exit = p_max;

                }

                length += (p_prev - p_voxel_exit).norm();
                Eigen::Vector3d traversal_vector = (p_voxel_exit - p_voxel_entry).cast<default_type>().normalized();
                if (traversal_vector.allFinite() && check (this_voxel, info)) {
                  const default_type index_voxel_exit = default_type(p) + mu;
                  const size_t mean_tck_index = std::round (0.5 * (index_voxel_entry + index_voxel_exit));
                  const default_type factor = tck_index_to_factor (mean_tck_index);
                  add_to_set (out, this_voxel, traversal_vector, length, factor);
                }

              } while (!end_track);

            }



          template <class Cont>
            void TrackMapper::voxelise_ends (const Streamline<>& tck, Cont& out) const
            {
              for (size_t end = 0; end != 2; ++end) {
                const Eigen::Vector3i vox = round (scanner2voxel * (end ? tck.back() : tck.front()));
                if (check (vox, info)) {
                  const Eigen::Vector3d dir = (end ? (tck[tck.size()-1] - tck[tck.size()-2]) : (tck[0] - tck[1])).cast<default_type>().normalized();
                  const default_type factor = (end ? factors.back() : factors.front());
                  add_to_set (out, vox, dir, 1.0, factor);
                }
              }
            }



          inline void TrackMapper::add_to_set (Set<Voxel>&    out, const Eigen::Vector3i& v, const Eigen::Vector3d& d, const default_type l, const default_type f) const
          {
            const Voxel voxel (v, l, f);
            out.insert (voxel);
          }
          inline void TrackMapper::add_to_set (Set<VoxelDEC>& out, const Eigen::Vector3i& v, const Eigen::Vector3d& d, const default_type l, const default_type f) const
          {
            const VoxelDEC voxel (v, d, l, f);
            out.insert (voxel);
          }
          inline void TrackMapper::add_to_set (Set<Dixel>&    out, const Eigen::Vector3i& v, const Eigen::Vector3d& d, const default_type l, const default_type f) const
          {
            assert (dixel_plugin);
            const size_t bin = (*dixel_plugin) (d);
            const Dixel dixel (v, bin, l, f);
            out.insert (dixel);
          }
          inline void TrackMapper::add_to_set (Set<VoxelTOD>& out, const Eigen::Vector3i& v, const Eigen::Vector3d& d, const default_type l, const default_type f) const
          {
            assert (tod_plugin);
            (*tod_plugin) (d);
            const VoxelTOD voxel (v, (*tod_plugin)(), l, f);
            out.insert (voxel);
          }
          inline void TrackMapper::add_to_set (Set<Fixel>&    out, const Eigen::Vector3i& v, const Eigen::Vector3d& d, const default_type l, const default_type f) const
          {
            assert (fixel_plugin);
            const MR::Fixel::index_type fixel_index = (*fixel_plugin) (v, d);
            if (fixel_index != fixel_plugin->nfixels()) {
              const Fixel fixel (fixel_index, l, f);
              out.insert (fixel);
            }
          }





          inline default_type TrackMapper::tck_index_to_factor (const size_t i) const
          {
            const default_type ideal_index = default_type(i) / default_type(upsampler.get_ratio());
            const size_t lower_index = std::max (size_t(std::floor (ideal_index)), size_t(0));
            const size_t upper_index = std::min (size_t(std::ceil  (ideal_index)), size_t(factors.size() - 1));
            const default_type mu = ideal_index - default_type(lower_index);
            return ((mu * factors[upper_index]) + ((1.0-mu) * factors[lower_index]));
          }





        }
      }
    }
  }
}

#endif



