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



#ifndef __dwi_tractography_sift_output_h__
#define __dwi_tractography_sift_output_h__


#include "dwi/fixel_map.h"

#include "dwi/tractography/SIFT/model_base.h"

#include "file/ofstream.h"

#include "image/buffer.h"
#include "image/buffer_sparse.h"
#include "image/header.h"
#include "image/loop.h"

#include "image/sparse/fixel_metric.h"
#include "image/sparse/keys.h"
#include "image/sparse/voxel.h"

#include "math/SH.h"
#include "math/vector.h"


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
        Image::Buffer<float> out (path, H);
        Image::Buffer<float>::voxel_type v_out (out);
        VoxelAccessor v (accessor);
        Image::LoopInOrder loop (v_out);
        for (loop.start (v_out, v); loop.ok(); loop.next (v_out, v)) {
          if (v.value()) {
            float value = 0.0;
            for (typename Fixel_map<Fixel>::ConstIterator i = begin (v); i; ++i)
              value += i().get_FOD();
            v_out.value() = value;
          } else {
            v_out.value() = NAN;
          }
        }
      }

      template <class Fixel>
      void ModelBase<Fixel>::output_target_image_sh (const std::string& path) const
      {
        const size_t L = 8;
        const size_t N = Math::SH::NforL (L);
        Math::SH::aPSF<float> aPSF (L);
        Image::Header H_sh (H);
        H_sh.set_ndim (4);
        H_sh.dim(3) = N;
        H_sh.stride (3) = 0;
        Image::Buffer<float> out (path, H_sh);
        Image::Buffer<float>::voxel_type v_out (out);
        VoxelAccessor v (accessor);
        Image::LoopInOrder loop (v_out, 0, 3);
        for (loop.start (v_out, v); loop.ok(); loop.next (v_out, v)) {
          if (v.value()) {
            Math::Vector<float> sum;
            sum.resize (N, 0.0);
            for (typename Fixel_map<Fixel>::ConstIterator i = begin (v); i; ++i) {
              if (i().get_FOD()) {
                Math::Vector<float> this_lobe;
                aPSF (this_lobe, i().get_dir());
                for (size_t c = 0; c != N; ++c)
                  sum[c] += i().get_FOD() * this_lobe[c];
              }
            }
            for (v_out[3] = 0; v_out[3] != (int)N; ++v_out[3])
              v_out.value() = sum[v_out[3]];
          } else {
            for (v_out[3] = 0; v_out[3] != (int)N; ++v_out[3])
              v_out.value() = NAN;
          }
        }
      }

      template <class Fixel>
      void ModelBase<Fixel>::output_target_image_fixel (const std::string& path) const
      {
        using Image::Sparse::FixelMetric;
        Image::Header H_fixel (H);
        H_fixel.datatype() = DataType::UInt64;
        H_fixel.datatype().set_byte_order_native();
        H_fixel[Image::Sparse::name_key] = str(typeid(FixelMetric).name());
        H_fixel[Image::Sparse::size_key] = str(sizeof(FixelMetric));
        Image::BufferSparse<FixelMetric> out (path, H_fixel);
        Image::BufferSparse<FixelMetric>::voxel_type v_out (out);
        VoxelAccessor v (accessor);
        Image::LoopInOrder loop (v_out);
        for (loop.start (v_out, v); loop.ok(); loop.next (v_out, v)) {
          if (v.value()) {
            v_out.value().set_size ((*v.value()).num_fixels());
            size_t index = 0;
            for (typename Fixel_map<Fixel>::ConstIterator iter = begin (v); iter; ++iter, ++index) {
              FixelMetric fixel (iter().get_dir(), iter().get_FOD(), iter().get_FOD());
              v_out.value()[index] = fixel;
            }
          }
        }
      }

      template <class Fixel>
      void ModelBase<Fixel>::output_tdi (const std::string& path) const
      {
        const double current_mu = mu();
        Image::Buffer<float> out (path, H);
        Image::Buffer<float>::voxel_type v_out (out);
        VoxelAccessor v (accessor);
        Image::LoopInOrder loop (v_out);
        for (loop.start (v_out, v); loop.ok(); loop.next (v_out, v)) {
          if (v.value()) {
            float value = 0.0;
            for (typename Fixel_map<Fixel>::ConstIterator i = begin (v); i; ++i)
              value += i().get_TD();
            v_out.value() = value * current_mu;
          } else {
            v_out.value() = NAN;
          }
        }
      }

      template <class Fixel>
      void ModelBase<Fixel>::output_tdi_null_lobes (const std::string& path) const
      {
        const double current_mu = mu();
        Image::Buffer<float> out (path, H);
        Image::Buffer<float>::voxel_type v_out (out);
        VoxelAccessor v (accessor);
        Image::LoopInOrder loop (v_out);
        for (loop.start (v_out, v); loop.ok(); loop.next (v_out, v)) {
          if (v.value()) {
            float value = 0.0;
            for (typename Fixel_map<Fixel>::ConstIterator i = begin (v); i; ++i) {
              if (!i().get_FOD())
                value += i().get_TD();
            }
            v_out.value() = value * current_mu;
          } else {
            v_out.value() = NAN;
          }
        }
      }

      template <class Fixel>
      void ModelBase<Fixel>::output_tdi_sh (const std::string& path) const
      {
        const double current_mu = mu();
        const size_t L = 8;
        const size_t N = Math::SH::NforL (L);
        Math::SH::aPSF<float> aPSF (L);
        Image::Header H_sh (H);
        H_sh.set_ndim (4);
        H_sh.dim(3) = N;
        H_sh.stride (3) = 0;
        Image::Buffer<float> out (path, H_sh);
        Image::Buffer<float>::voxel_type v_out (out);
        VoxelAccessor v (accessor);
        Image::LoopInOrder loop (v_out, 0, 3);
        for (loop.start (v_out, v); loop.ok(); loop.next (v_out, v)) {
          if (v.value()) {
            Math::Vector<float> sum;
            sum.resize (N, 0.0);
            for (typename Fixel_map<Fixel>::ConstIterator i = begin (v); i; ++i) {
              if (i().get_FOD()) {
                Math::Vector<float> this_lobe;
                aPSF (this_lobe, i().get_dir());
                for (size_t c = 0; c != N; ++c)
                  sum[c] += i().get_TD() * this_lobe[c];
              }
              for (v_out[3] = 0; v_out[3] != (int)N; ++v_out[3])
                v_out.value() = sum[v_out[3]] * current_mu;
            }
          } else {
            for (v_out[3] = 0; v_out[3] != (int)N; ++v_out[3])
              v_out.value() = NAN;
          }
        }
      }

      template <class Fixel>
      void ModelBase<Fixel>::output_tdi_fixel (const std::string& path) const
      {
        using Image::Sparse::FixelMetric;
        const double current_mu = mu();
        Image::Header H_fixel (H);
        H_fixel.datatype() = DataType::UInt64;
        H_fixel.datatype().set_byte_order_native();
        H_fixel[Image::Sparse::name_key] = str(typeid(FixelMetric).name());
        H_fixel[Image::Sparse::size_key] = str(sizeof(FixelMetric));
        Image::BufferSparse<FixelMetric> out (path, H_fixel);
        Image::BufferSparse<FixelMetric>::voxel_type v_out (out);
        VoxelAccessor v (accessor);
        Image::LoopInOrder loop (v_out);
        for (loop.start (v_out, v); loop.ok(); loop.next (v_out, v)) {
          if (v.value()) {
            v_out.value().set_size ((*v.value()).num_fixels());
            size_t index = 0;
            for (typename Fixel_map<Fixel>::ConstIterator iter = begin (v); iter; ++iter, ++index) {
              FixelMetric fixel (iter().get_dir(), iter().get_FOD(), current_mu * iter().get_TD());
              v_out.value()[index] = fixel;
            }
          }
        }
      }

      template <class Fixel>
      void ModelBase<Fixel>::output_error_images (const std::string& max_abs_diff_path, const std::string& diff_path, const std::string& cost_path) const
      {
        const double current_mu = mu();
        Image::Buffer<float> max_abs_diff (max_abs_diff_path, H), diff (diff_path, H), cost (cost_path, H);
        Image::Buffer<float>::voxel_type v_max_abs_diff (max_abs_diff), v_diff (diff), v_cost (cost);
        VoxelAccessor v (accessor);
        Image::LoopInOrder loop (v);
        for (loop.start (v); loop.ok(); loop.next (v)) {
          v_max_abs_diff[2] = v_diff[2] = v_cost[2] = v[2];
          v_max_abs_diff[1] = v_diff[1] = v_cost[1] = v[1];
          v_max_abs_diff[0] = v_diff[0] = v_cost[0] = v[0];
          if (v.value()) {
            double max_abs_diff = 0.0, diff = 0.0, cost = 0.0;
            for (typename Fixel_map<Fixel>::ConstIterator i = begin (v); i; ++i) {
              const double this_diff = i().get_diff (current_mu);
              max_abs_diff = MAX (max_abs_diff, fabs (this_diff));
              diff += this_diff;
              cost += i().get_cost (current_mu) * i().get_weight();
            }
            v_max_abs_diff.value() = max_abs_diff;
            v_diff.value() = diff;
            v_cost.value() = cost;
          } else {
            v_max_abs_diff.value() = NAN;
            v_diff.value() = NAN;
            v_cost.value() = NAN;
          }
        }
      }

      template <class Fixel>
      void ModelBase<Fixel>::output_error_fixel_images (const std::string& diff_path, const std::string& cost_path) const
      {
        using Image::Sparse::FixelMetric;
        const double current_mu = mu();
        Image::Header H_fixel (H);
        H_fixel.datatype() = DataType::UInt64;
        H_fixel.datatype().set_byte_order_native();
        H_fixel[Image::Sparse::name_key] = str(typeid(FixelMetric).name());
        H_fixel[Image::Sparse::size_key] = str(sizeof(FixelMetric));
        Image::BufferSparse<FixelMetric> out_diff (diff_path, H_fixel);
        Image::BufferSparse<FixelMetric>::voxel_type v_diff (out_diff);
        Image::BufferSparse<FixelMetric> out_cost (cost_path, H_fixel);
        Image::BufferSparse<FixelMetric>::voxel_type v_cost (out_cost);
        VoxelAccessor v (accessor);
        Image::LoopInOrder loop (v_diff);
        for (loop.start (v, v_diff, v_cost); loop.ok(); loop.next (v, v_diff, v_cost)) {
          if (v.value()) {
            v_diff.value().set_size ((*v.value()).num_fixels());
            v_cost.value().set_size ((*v.value()).num_fixels());
            size_t index = 0;
            for (typename Fixel_map<Fixel>::ConstIterator iter = begin (v); iter; ++iter, ++index) {
              FixelMetric fixel_diff (iter().get_dir(), iter().get_FOD(), iter().get_diff (current_mu));
              v_diff.value()[index] = fixel_diff;
              FixelMetric fixel_cost (iter().get_dir(), iter().get_FOD(), iter().get_cost (current_mu));
              v_cost.value()[index] = fixel_cost;
            }
          }
        }
      }

      template <class Fixel>
      void ModelBase<Fixel>::output_scatterplot (const std::string& path) const
      {
        File::OFStream out (path, std::ios_base::out | std::ios_base::trunc);
        const double current_mu = mu();
        out << "FOD amplitude,Track density (unscaled),Track density (scaled),Weight,\n";
        for (typename std::vector<Fixel>::const_iterator i = fixels.begin(); i != fixels.end(); ++i)
          out << str (i->get_FOD()) << "," << str (i->get_TD()) << "," << str (i->get_TD() * current_mu) << "," << str (i->get_weight()) << ",\n";
        out.close();
      }

      template <class Fixel>
      void ModelBase<Fixel>::output_fixel_count_image (const std::string& path) const
      {
        Image::Header H_out (H);
        H_out.datatype() = DataType::UInt8;
        Image::Buffer<uint8_t> image (path, H_out);
        Image::Buffer<uint8_t>::voxel_type v_out (image);
        VoxelAccessor v (accessor);
        Image::LoopInOrder loop (v_out);
        for (loop.start (v_out, v); loop.ok(); loop.next (v_out, v)) {
          if (v.value())
            v_out.value() = (*v.value()).num_fixels();
          else
            v_out.value() = 0;
        }
      }

      template <class Fixel>
      void ModelBase<Fixel>::output_untracked_fixels (const std::string& path_count, const std::string& path_amps) const
      {
        Image::Header H_uint8_t (H);
        H_uint8_t.datatype() = DataType::UInt8;
        Image::Buffer<uint8_t> out_count (path_count, H_uint8_t);
        Image::Buffer<float>   out_amps (path_amps, H);
        Image::Buffer<uint8_t>::voxel_type v_out_count (out_count);
        Image::Buffer<float>  ::voxel_type v_out_amps  (out_amps);
        VoxelAccessor v (accessor);
        Image::LoopInOrder loop (v_out_count);
        for (loop.start (v_out_count, v_out_amps, v); loop.ok(); loop.next (v_out_count, v_out_amps, v)) {
          if (v.value()) {
            uint8_t count = 0;
            float sum = 0.0;
            for (typename Fixel_map<Fixel>::ConstIterator i = begin (v); i; ++i) {
              if (!i().get_TD()) {
                ++count;
                sum += i().get_FOD();
              }
            }
            v_out_count.value() = count;
            v_out_amps .value() = sum;
          } else {
            v_out_count.value() = 0;
            v_out_amps .value() = NAN;
          }
        }
      }



      }
    }
  }
}


#endif


