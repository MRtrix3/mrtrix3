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

#include "registration/multi_contrast.h"
#include "header.h"
#include "image_io/default.h"
#include "image_io/scratch.h"
#include "file/name_parser.h"
#include "formats/list.h"

namespace MR
{
  namespace Registration
  {
    vector<std::string> parse_image_sequence_output (const std::string& output_sequence, const vector<Header>& image_sequence_input) {
      vector<std::string> V;
      if (!output_sequence.size()) throw Exception ("image sequence specifier is empty");

      auto output_image_names = split (output_sequence, ",");
      if (output_image_names.size() != image_sequence_input.size())
        throw Exception ("image output sequence has to be of equal length to input image sequence. output sequence:\n" + str(output_sequence));

      for (size_t idx = 0; idx < output_image_names.size(); idx++) {
        std::string image_name = output_image_names[idx];

        if (Path::exists (image_name) && !App::overwrite_files)
          throw Exception ("output image \"" + image_name + "\" already exists (use -force option to force overwrite)");

        Header H = image_sequence_input[idx];
        File::NameParser parser;
        parser.parse (image_name);
        vector<int> Pdim (parser.ndim());

        H.name() = image_name;

        const Formats::Base** format_handler = Formats::handlers;
        for (; *format_handler; format_handler++)
          if ((*format_handler)->check (H, H.ndim() - Pdim.size()))
            break;

        if (!*format_handler) {
          const std::string basename = Path::basename (image_name);
          const size_t extension_index = basename.find_last_of (".");
          if (extension_index == std::string::npos)
            throw Exception ("unknown format for image \"" + image_name + "\" (no file extension specified)");
          else
            throw Exception ("unknown format for image \"" + image_name + "\" (unsupported file extension: " + basename.substr (extension_index) + ")");
        }
        V.push_back (image_name);
      }
      return V;
    }

    class CopyFunctor4D {
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

    class CopyFunctor3D {
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
          std::vector<size_t> from (ndim, 0);
          std::vector<size_t> size (ndim, 1);
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
      // display<Image<default_type>> (images);
    }

  }
}
