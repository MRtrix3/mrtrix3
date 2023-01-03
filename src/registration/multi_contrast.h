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

#ifndef __registration_multi_contrast_h__
#define __registration_multi_contrast_h__

#include <vector>

#include "types.h"
#include "math/SH.h"
#include "image.h"
#include "header.h"
#include "adapter/subset.h"
#include "adapter/base.h"
#include "image_io/default.h"
#include "image_io/scratch.h"
#include "file/name_parser.h"
#include "header.h"
#include "formats/list.h"

namespace MR
{
  namespace Registration
  {

    FORCE_INLINE void check_image_output (const std::string& image_name, const Header& reference) {
      vector<std::string> V;
      if (!image_name.size()) throw Exception ("image output path is empty");
        if (Path::exists (image_name) && !App::overwrite_files)
          throw Exception ("output image \"" + image_name + "\" already exists (use -force option to force overwrite)");

        Header H = reference;
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
    }

    struct MultiContrastSetting { NOMEMALIGN
      size_t start; // index to volume in image holding all tissue contrasts
      size_t nvols; // number of volumes preloaded into image holding all tissue contrasts
      ssize_t lmax;  // maximum requested lmax
      bool do_reorientation; // registration treats this image as (possibly 4D) scalar image
      size_t image_nvols; // number of volumes in original image
      ssize_t image_lmax; // lmax available in image (preloaded, might differ from original image), 0 if not an FOD image
      default_type weight;

      MultiContrastSetting (): start (0), nvols (0), do_reorientation (false), weight (1.0) {}
      MultiContrastSetting (size_t image_nvols, bool do_reorientation = false, ssize_t limit_lmax = std::numeric_limits<ssize_t>::max()) :
        start (0), nvols (image_nvols), do_reorientation (do_reorientation), weight (1.0) {
          if (do_reorientation) {
            image_lmax = Math::SH::LforN (image_nvols);
            lmax = image_lmax;
          } else {
            image_lmax = 0;
            lmax = 0;
          }
          lower_lmax (limit_lmax);
        }

      FORCE_INLINE void lower_lmax (ssize_t new_lmax) {
        assert (new_lmax >= 0);
        if (new_lmax < lmax) {
          lmax = new_lmax;
          if (new_lmax > 0) {
            nvols =  Math::SH::NforL (new_lmax);
          } else {
            nvols = 1;
          }
        }
      }
    };

    inline std::ostream& operator << (std::ostream & o, const MultiContrastSetting & a) {
      o << "MultiContrast: [start:" << a.start << ", nvols:" << a.nvols << ", lmax:" << a.lmax << ", image_lmax:" << a.image_lmax
      << ", reorient:" << a.do_reorientation << ", weight:" << a.weight << "]";
      return o;
    }

    void preload_data(vector<Header>& input, Image<default_type>& images, const vector<MultiContrastSetting>& mc_params);

  }
}
#endif
