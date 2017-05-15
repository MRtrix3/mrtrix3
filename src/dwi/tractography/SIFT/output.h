/* Copyright (c) 2008-2017 the MRtrix3 contributors.
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


#ifndef __dwi_tractography_sift_output_h__
#define __dwi_tractography_sift_output_h__


#include "image.h"
#include "header.h"

#include "algo/loop.h"

#include "dwi/fixel_map.h"
#include "dwi/tractography/SIFT/model_base.h"

#include "file/ofstream.h"

#include "math/SH.h"

#include "fixel/legacy/fixel_metric.h"
#include "fixel/legacy/image.h"
#include "fixel/legacy/keys.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace SIFT
      {

      // Output functions - non-essential, mostly debugging outputs
      template <class Fixel>
      void ModelBase<Fixel>::output_target_image (const std::string& path) const
      {
        auto out = Image<float>::create (path, Fixel_map<Fixel>::header());
        VoxelAccessor v (accessor());
        for (auto l = Loop(out) (out, v); l; ++l) {
          if (v.value()) {
            default_type value = 0.0;
            for (typename Fixel_map<Fixel>::ConstIterator i = begin (v); i; ++i)
              value += i().get_FOD();
            out.value() = value;
          } else {
            out.value() = NAN;
          }
        }
      }

      template <class Fixel>
      void ModelBase<Fixel>::output_target_image_sh (const std::string& path) const
      {
        const size_t L = 8;
        const size_t N = Math::SH::NforL (L);
        Math::SH::aPSF<default_type> aPSF (L);
        Header H_sh (Fixel_map<Fixel>::header());
        H_sh.ndim() = 4;
        H_sh.size(3) = N;
        H_sh.stride (3) = 0;
        auto out = Image<float>::create (path, H_sh);
        VoxelAccessor v (accessor());
        for (auto l = Loop (0, 3) (out, v); l; ++l) {
          if (v.value()) {
            Eigen::Matrix<default_type, Eigen::Dynamic, 1> sum = Eigen::Matrix<default_type, Eigen::Dynamic, 1>::Zero (N);
            for (typename Fixel_map<Fixel>::ConstIterator i = begin (v); i; ++i) {
              if (i().get_FOD()) {
                Eigen::Matrix<default_type, Eigen::Dynamic, 1> this_lobe;
                aPSF (this_lobe, i().get_dir());
                for (size_t c = 0; c != N; ++c)
                  sum[c] += i().get_FOD() * this_lobe[c];
              }
            }
            for (auto l = Loop (3) (out); l; ++l)
              out.value() = sum[out.index(3)];
          } else {
            for (auto l = Loop (3) (out); l; ++l)
              out.value() = NAN;
          }
        }
      }

      template <class Fixel>
      void ModelBase<Fixel>::output_target_image_fixel (const std::string& path) const
      {
        using MR::Fixel::Legacy::FixelMetric;
        Header H_fixel (Fixel_map<Fixel>::header());
        H_fixel.datatype() = DataType::UInt64;
        H_fixel.datatype().set_byte_order_native();
        H_fixel.keyval()[MR::Fixel::Legacy::name_key] = str(typeid(FixelMetric).name());
        H_fixel.keyval()[MR::Fixel::Legacy::size_key] = str(sizeof(FixelMetric));
        MR::Fixel::Legacy::Image<FixelMetric> out (path, H_fixel);
        VoxelAccessor v (accessor());
        for (auto l = Loop (out) (out, v); l; ++l) {
          if (v.value()) {
            out.value().set_size ((*v.value()).num_fixels());
            size_t index = 0;
            for (typename Fixel_map<Fixel>::ConstIterator iter = begin (v); iter; ++iter, ++index) {
              FixelMetric fixel (iter().get_dir().template cast<float>(), iter().get_FOD(), iter().get_FOD());
              out.value()[index] = fixel;
            }
          }
        }
      }

      template <class Fixel>
      void ModelBase<Fixel>::output_tdi (const std::string& path) const
      {
        const default_type current_mu = mu();
        auto out = Image<float>::create (path, Fixel_map<Fixel>::header());
        VoxelAccessor v (accessor());
        for (auto l = Loop (out) (out, v); l; ++l) {
          if (v.value()) {
            default_type value = 0.0;
            for (typename Fixel_map<Fixel>::ConstIterator i = begin (v); i; ++i)
              value += i().get_TD();
            out.value() = value * current_mu;
          } else {
            out.value() = NaN;
          }
        }
      }

      template <class Fixel>
      void ModelBase<Fixel>::output_tdi_null_lobes (const std::string& path) const
      {
        const default_type current_mu = mu();
        auto out = Image<float>::create (path, Fixel_map<Fixel>::header());
        VoxelAccessor v (accessor());
        for (auto l = Loop (out) (out, v); l; ++l) {
          if (v.value()) {
            default_type value = 0.0;
            for (typename Fixel_map<Fixel>::ConstIterator i = begin (v); i; ++i) {
              if (!i().get_FOD())
                value += i().get_TD();
            }
            out.value() = value * current_mu;
          } else {
            out.value() = NaN;
          }
        }
      }

      template <class Fixel>
      void ModelBase<Fixel>::output_tdi_sh (const std::string& path) const
      {
        const default_type current_mu = mu();
        const size_t L = 8;
        const size_t N = Math::SH::NforL (L);
        Math::SH::aPSF<default_type> aPSF (L);
        Header H_sh (Fixel_map<Fixel>::header());
        H_sh.ndim() = 4;
        H_sh.size(3) = N;
        H_sh.stride (3) = 0;
        auto out = Image<float>::create (path, H_sh);
        VoxelAccessor v (accessor());
        for (auto l = Loop (out) (out, v); l; ++l) {
          if (v.value()) {
            Eigen::Matrix<default_type, Eigen::Dynamic, 1> sum = Eigen::Matrix<default_type, Eigen::Dynamic, 1>::Zero (N);
            for (typename Fixel_map<Fixel>::ConstIterator i = begin (v); i; ++i) {
              if (i().get_FOD()) {
                Eigen::Matrix<default_type, Eigen::Dynamic, 1> this_lobe;
                aPSF (this_lobe, i().get_dir());
                for (size_t c = 0; c != N; ++c)
                  sum[c] += i().get_TD() * this_lobe[c];
              }
              for (auto l = Loop (3) (out); l; ++l)
                out.value() = sum[out.index(3)] * current_mu;
            }
          } else {
            for (auto l = Loop (3) (out); l; ++l)
              out.value() = NaN;
          }
        }
      }

      template <class Fixel>
      void ModelBase<Fixel>::output_tdi_fixel (const std::string& path) const
      {
        using MR::Fixel::Legacy::FixelMetric;
        const default_type current_mu = mu();
        Header H_fixel (Fixel_map<Fixel>::header());
        H_fixel.datatype() = DataType::UInt64;
        H_fixel.datatype().set_byte_order_native();
        H_fixel.keyval()[MR::Fixel::Legacy::name_key] = str(typeid(FixelMetric).name());
        H_fixel.keyval()[MR::Fixel::Legacy::size_key] = str(sizeof(FixelMetric));
        MR::Fixel::Legacy::Image<FixelMetric> out (path, H_fixel);
        VoxelAccessor v (accessor());
        for (auto l = Loop (out) (out, v); l; ++l) {
          if (v.value()) {
            out.value().set_size ((*v.value()).num_fixels());
            size_t index = 0;
            for (typename Fixel_map<Fixel>::ConstIterator iter = begin (v); iter; ++iter, ++index) {
              FixelMetric fixel (iter().get_dir().template cast<float>(), iter().get_FOD(), current_mu * iter().get_TD());
              out.value()[index] = fixel;
            }
          }
        }
      }

      template <class Fixel>
      void ModelBase<Fixel>::output_error_images (const std::string& max_abs_diff_path, const std::string& diff_path, const std::string& cost_path) const
      {
        const default_type current_mu = mu();
        auto out_max_abs_diff = Image<float>::create (max_abs_diff_path, Fixel_map<Fixel>::header());
        auto out_diff = Image<float>::create (diff_path, Fixel_map<Fixel>::header());
        auto out_cost = Image<float>::create (cost_path, Fixel_map<Fixel>::header());
        VoxelAccessor v (accessor());
        for (auto l = Loop (v) (v, out_max_abs_diff, out_diff, out_cost); l; ++l) {
          if (v.value()) {
            default_type max_abs_diff = 0.0, diff = 0.0, cost = 0.0;
            for (typename Fixel_map<Fixel>::ConstIterator i = begin (v); i; ++i) {
              const default_type this_diff = i().get_diff (current_mu);
              max_abs_diff = std::max (max_abs_diff, std::abs (this_diff));
              diff += this_diff;
              cost += i().get_cost (current_mu) * i().get_weight();
            }
            out_max_abs_diff.value() = max_abs_diff;
            out_diff.value() = diff;
            out_cost.value() = cost;
          } else {
            out_max_abs_diff.value() = NaN;
            out_diff.value() = NaN;
            out_cost.value() = NaN;
          }
        }
      }

      template <class Fixel>
      void ModelBase<Fixel>::output_error_fixel_images (const std::string& diff_path, const std::string& cost_path) const
      {
        using MR::Fixel::Legacy::FixelMetric;
        const default_type current_mu = mu();
        Header H_fixel (Fixel_map<Fixel>::header());
        H_fixel.datatype() = DataType::UInt64;
        H_fixel.datatype().set_byte_order_native();
        H_fixel.keyval()[MR::Fixel::Legacy::name_key] = str(typeid(FixelMetric).name());
        H_fixel.keyval()[MR::Fixel::Legacy::size_key] = str(sizeof(FixelMetric));
        MR::Fixel::Legacy::Image<FixelMetric> out_diff (diff_path, H_fixel);
        MR::Fixel::Legacy::Image<FixelMetric> out_cost (cost_path, H_fixel);
        VoxelAccessor v (accessor());
        for (auto l = Loop (v) (v, out_diff, out_cost); l; ++l) {
          if (v.value()) {
            out_diff.value().set_size ((*v.value()).num_fixels());
            out_cost.value().set_size ((*v.value()).num_fixels());
            size_t index = 0;
            for (typename Fixel_map<Fixel>::ConstIterator iter = begin (v); iter; ++iter, ++index) {
              FixelMetric fixel_diff (iter().get_dir().template cast<float>(), iter().get_FOD(), iter().get_diff (current_mu));
              out_diff.value()[index] = fixel_diff;
              FixelMetric fixel_cost (iter().get_dir().template cast<float>(), iter().get_FOD(), iter().get_cost (current_mu));
              out_cost.value()[index] = fixel_cost;
            }
          }
        }
      }

      template <class Fixel>
      void ModelBase<Fixel>::output_scatterplot (const std::string& path) const
      {
        File::OFStream out (path, std::ios_base::out | std::ios_base::trunc);
        const default_type current_mu = mu();
        out << "Fibre density,Track density (unscaled),Track density (scaled),Weight,\n";
        for (typename vector<Fixel>::const_iterator i = fixels.begin(); i != fixels.end(); ++i)
          out << str (i->get_FOD()) << "," << str (i->get_TD()) << "," << str (i->get_TD() * current_mu) << "," << str (i->get_weight()) << ",\n";
        out.close();
      }

      template <class Fixel>
      void ModelBase<Fixel>::output_fixel_count_image (const std::string& path) const
      {
        Header H_out (Fixel_map<Fixel>::header());
        H_out.datatype() = DataType::UInt8;
        auto out = Image<uint8_t>::create (path, H_out);
        VoxelAccessor v (accessor());
        for (auto l = Loop (v) (v, out); l; ++l) {
          if (v.value())
            out.value() = (*v.value()).num_fixels();
          else
            out.value() = 0;
        }
      }

      template <class Fixel>
      void ModelBase<Fixel>::output_untracked_fixels (const std::string& path_count, const std::string& path_amps) const
      {
        Header H_uint8_t (Fixel_map<Fixel>::header());
        H_uint8_t.datatype() = DataType::UInt8;
        auto out_count = Image<uint8_t>::create (path_count, H_uint8_t);
        auto out_amps = Image<float>::create (path_amps, Fixel_map<Fixel>::header());
        VoxelAccessor v (accessor());
        for (auto l = Loop (v) (v, out_count, out_amps, v); l; ++l) {
          if (v.value()) {
            uint8_t count = 0;
            default_type sum = 0.0;
            for (typename Fixel_map<Fixel>::ConstIterator i = begin (v); i; ++i) {
              if (!i().get_TD()) {
                ++count;
                sum += i().get_FOD();
              }
            }
            out_count.value() = count;
            out_amps .value() = sum;
          } else {
            out_count.value() = 0;
            out_amps .value() = NaN;
          }
        }
      }



      }
    }
  }
}


#endif


