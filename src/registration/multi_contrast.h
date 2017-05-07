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
#include "image_io/default.h"
#include "image_io/scratch.h"
#include "file/name_parser.h"
#include "header.h"
#include "formats/list.h"

namespace MR
{
  namespace Registration
  {

    //! split_image_sequence
    /*! split_image_sequence allows to split arguments into any number of vectors of strings. for instance the arguments
     * 1.mif + 2.mif 3.mif + 4.mif + 5.mif + 6.mif get split into two vectors if size 2 and 4 with:
     * vector<std::string> filenames1, filenames2;
     * Registration::split_image_sequence (argument.begin(), argument.end(), filenames1, filenames2);
     * for one sequence of images 1.mif + 2.mif + 3.mif use:
     * Registration::split_image_sequence (argument.begin(), argument.end(), filenames1);
     * adjust the iterators to use a segment of the arguments
     * note that currently core/app.h throws an exception if the number of optional arguments provided
     * is not equal for all arguments. However, split_image_sequence should also be able to handle that.
    */
    // template <typename ParsedArgumentIterator, typename T>
    // void split_image_sequence (ParsedArgumentIterator argument, const ParsedArgumentIterator stop, T& t) {
    //   std::string sep {"+"};
    //   bool previous_was_sep = true;
    //   while (argument != stop) {
    //     const std::string filename {str(*argument)};
    //     if (filename != sep) {
    //       if (!previous_was_sep)
    //         throw Exception ("too many images provided as arguments");
    //       t.push_back(filename);
    //       previous_was_sep = false;
    //     } else {
    //       if (previous_was_sep)
    //         throw Exception ("duplicate image sequence separator ("+sep+") provided");
    //       previous_was_sep = true;
    //     }
    //     argument += 1;
    //   }
    // }

    // template<typename ParsedArgumentIterator, typename T, typename... FurtherT>
    // void split_image_sequence (ParsedArgumentIterator argument, const ParsedArgumentIterator stop, T& t, FurtherT&... fts) {
    //   bool previous_was_image = false;
    //   std::string sep {"+"};
    //   while (argument != stop) {
    //     const std::string filename {str(*argument)};
    //     if (filename == sep) {
    //       if (!previous_was_image)
    //         throw Exception ("duplicate or misplaced image sequence separator ("+sep+") provided");
    //       previous_was_image = false;
    //       argument += 1;
    //       continue;
    //     } else {
    //       if (previous_was_image) {
    //         split_image_sequence (argument, stop, fts...);
    //         return;
    //       }
    //       t.push_back(filename);
    //       previous_was_image = true;
    //       argument += 1;
    //     }
    //   }
    //   if (argument == stop)
    //     throw Exception ("not enough images provided as arguments");
    //   split_image_sequence (argument, stop, fts...) ;
    // }

    // FORCE_INLINE vector<Header> parse_image_sequence_input (const std::string& spec ) {
    //   vector<Header> V;
    //   if (!spec.size()) throw Exception ("image sequence specifier is empty");
    //   try {
    //     for (auto& filename : split(spec,",")) {
    //       V.push_back (Header::open (filename));
    //     }
    //   } catch (Exception& E) {
    //     throw Exception (E, "can't parse image sequence specifier \"" + spec + "\"");
    //   }
    //   return V;
    // }

    // vector<std::string> parse_image_sequence_output (const std::string& output_sequence, const vector<Header>& image_sequence_input);
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
