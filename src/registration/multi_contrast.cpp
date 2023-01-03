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

#include "registration/multi_contrast.h"

namespace MR
{
  namespace Registration
  {

    class CopyFunctor4D { NOMEMALIGN
    public:
      CopyFunctor4D (size_t out_start_vol, size_t nvols) :
        start_vol (out_start_vol),
        nvols (nvols) { }
      template <class VoxeltypeIn, class VoxeltypeOut>
        void operator() (VoxeltypeIn& in, VoxeltypeOut& out) {
          auto loop = Loop (3);
          auto l = loop (in);
          for (size_t ivol = 0; ivol < nvols; ++ivol) {
            assert(l);
            out.index(3) = in.index(3) + start_vol;
            out.value() = in.value();
            ++l;
          }
        }
    protected:
      size_t start_vol, nvols;
    };


    class CopyFunctor3D { NOMEMALIGN
    public:
      CopyFunctor3D (size_t out_start_vol) :
        start_vol (out_start_vol) { }
      template <class VoxeltypeIn, class VoxeltypeOut>
        void operator() (VoxeltypeIn& in, VoxeltypeOut& out) {
          // assign_pos_of(in,0,3).to(out);
          assert(out.index(0)==in.index(0));
          assert(out.index(1)==in.index(1));
          assert(out.index(2)==in.index(2));
          out.index(3) = start_vol;
          out.value() = in.value();
          out.index(3) = 0; // TODO do we need this?
        }
    protected:
      size_t start_vol;
    };


    void preload_data(vector<Header>& input, Image<default_type>& images, const vector<MultiContrastSetting>& mc_params) {
      const size_t n_images = input.size();
      assert (mc_params.size() == input.size());
      size_t sumvols (0);
      for (auto& n : mc_params)
        sumvols += n.nvols;
      Header h1 = Header (input[0]);
      if (sumvols  > 1) {
        h1.ndim() = 4;
        h1.size(3) = sumvols;
      } else
        h1.ndim() = 3;

      {
        LogLevelLatch log_level (0);
        images = Image<default_type>::scratch (h1).with_direct_io(Stride::contiguous_along_axis (3));
      }

      if (sumvols == 1) {
        auto image_in = input[0].get_image<default_type>();
        threaded_copy(image_in, images, 0, 3, 2);
      } else {
        for (size_t idx = 0; idx < n_images; idx++) {
          size_t ndim = input[idx].ndim();
          vector<size_t> from (ndim, 0);
          vector<size_t> size (ndim, 1);
          for (size_t dim = 0; dim < 3; ++dim)
            size[dim] = input[idx].size(dim);
          if (ndim==4)
            size[3] = mc_params[idx].nvols;

          auto image_in = input[idx].get_image<default_type>();
          INFO (str("index " + str(mc_params[idx].start))+": "+
            str(mc_params[idx].nvols) + " volumes from " + image_in.name());
          if (ndim == 4) {
            Adapter::Subset<Image<default_type>> subset (image_in, from, size);
            auto loop = ThreadedLoop (subset, 0, 3);
            loop.run (CopyFunctor4D(mc_params[idx].start, mc_params[idx].nvols), subset, images);
          } else {
            auto loop = ThreadedLoop (image_in, 0, 3);
            assert(mc_params[idx].nvols == 1);
            loop.run (CopyFunctor3D(mc_params[idx].start), image_in, images);
          }
        }
      }
    }

  }
}
