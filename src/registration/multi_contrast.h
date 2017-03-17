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

#ifndef __registration_multi_contrast_h__
#define __registration_multi_contrast_h__

#include <vector>

#include "types.h"
#include "math/SH.h"
#include "image.h"
#include "header.h"
#include "adapter/subset.h"
#include "adapter/base.h"

namespace MR
{
  namespace Registration
  {

    FORCE_INLINE vector<Header> parse_image_sequence_input (const std::string& spec ) {
      vector<Header> V;
      if (!spec.size()) throw Exception ("image sequence specifier is empty");
      try {
        for (auto& filename : split(spec,",")) {
          V.push_back (Header::open (filename));
        }
      } catch (Exception& E) {
        throw Exception (E, "can't parse image sequence specifier \"" + spec + "\"");
      }
      return V;
    }

    vector<std::string> parse_image_sequence_output (const std::string& output_sequence, const vector<Header>& image_sequence_input);

    struct MultiContrastSetting { NOMEMALIGN
      size_t start; // index to volume in image holding all tissue contrasts
      size_t nvols; // number of volumes preloaded into image holding all tissue contrasts
      ssize_t lmax;  // maximum requested lmax
      bool do_reorientation;
      size_t image_nvols; // number of volumes in original image
      ssize_t image_lmax; // lmax available in image, 0 if not an FOD image
      default_type weight;

      MultiContrastSetting (): start (0), nvols (0), do_reorientation (false), weight(1.0) {}
      MultiContrastSetting (size_t image_nvols, bool do_reorientation = false, ssize_t limit_lmax = std::numeric_limits<ssize_t>::max()) :
        start (0), nvols (image_nvols), do_reorientation (do_reorientation), weight(1.0) {
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
        if (do_reorientation && new_lmax < lmax) {
          lmax = new_lmax;
          if (new_lmax > 0) {
            nvols =  Math::SH::NforL (new_lmax);
          } else {
            nvols = 1;
            do_reorientation = 0;
          }
        }
      }
    };

    inline std::ostream& operator << (std::ostream & o, const MultiContrastSetting & a) {
      o << "MultiContrast: [start:" << a.start << ", nvols:" << a.nvols << ", lmax:" << a.lmax
      << ", reorient:" << a.do_reorientation << ", weight:" << a.weight << "]";
      return o;
    }

    void preload_data(vector<Header>& input, Image<default_type>& images, const vector<MultiContrastSetting>& mc_params);

  }
}
#endif
