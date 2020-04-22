/* Copyright (c) 2008-2019 the MRtrix3 contributors.
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

#include "colourmap.h"

namespace MR
{
  namespace ColourMap
  {
    const char* Entry::default_amplitude = "color.r";

    namespace {
      float clamp (const float i) { return std::max (0.0f, std::min (1.0f, i)); }
    }


    const Entry maps[] = {
      Entry ("Gray",
              "color.rgb = vec3 (amplitude);\n",
              [] (float amplitude) { return Eigen::Array3f (clamp (amplitude), clamp (amplitude), clamp (amplitude)); }),

          Entry ("Hot",
              "color.rgb = vec3 (2.7213 * amplitude, 2.7213 * amplitude - 1.0, 3.7727 * amplitude - 2.7727);\n",
              [] (float amplitude) { return Eigen::Array3f (clamp (2.7213f * amplitude),
                                                            clamp (2.7213f * amplitude - 1.0f),
                                                            clamp (3.7727f * amplitude - 2.7727f)); }),

          Entry ("Cool",
              "color.rgb = 1.0 - (vec3 (2.7213 * (1.0 - amplitude), 2.7213 * (1.0 - amplitude) - 1.0, 3.7727 * (1.0 - amplitude) - 2.7727));\n",
              [] (float amplitude) { return Eigen::Array3f (clamp (1.0f - (2.7213f * (1.0f - amplitude))),
                                                            clamp (1.0f - (2.7213f * (1.0f - amplitude) - 1.0f)),
                                                            clamp (1.0f - (3.7727f * (1.0f - amplitude) - 2.7727f))); }),

          Entry ("Jet",
              "color.rgb = 1.5 - 4.0 * abs (1.0 - amplitude - vec3(0.25, 0.5, 0.75));\n",
              [] (float amplitude) { return Eigen::Array3f (clamp (1.5f - 4.0f * abs (1.0f - amplitude - 0.25f)),
                                                            clamp (1.5f - 4.0f * abs (1.0f - amplitude - 0.5f)),
                                                            clamp (1.5f - 4.0f * abs (1.0f - amplitude - 0.75f))); }),

          Entry ("PET",
              "color.r = 2.0*amplitude - 0.5;\n"
              "color.g = clamp (2.0 * (0.25 - abs (amplitude - 0.25)), 0.0, 1.0) + clamp (2.0*amplitude - 1.0, 0.0, 1.0);\n"
              "color.b = 1.0 - (clamp (1.0 - 2.0 * amplitude, 0.0, 1.0) + clamp (1.0 - 4.0 * abs (amplitude - 0.75), 0.0, 1.0));\n",
              [] (float amplitude) { return Eigen::Array3f (clamp (2.0f * amplitude - 0.5f),
                                                            clamp (0.25f - abs (amplitude - 0.25f)) + clamp (2.0f * (amplitude - 0.5)),
                                                            clamp (1.0f - 2.0f * amplitude) + clamp (1.0 - 4.0 * abs (amplitude - 0.75))); }),

          Entry ("Colour",
              "color.rgb = amplitude * colourmap_colour;\n",
              Entry::basic_map_fn(),
              NULL, false, true),

          Entry ("RGB",
              "color.rgb = scale * (abs(color.rgb) - offset);\n",
              Entry::basic_map_fn(),
              "length (color.rgb)",
              true, false, true),

          Entry ("Complex",
              "float C = atan (color.g, color.r) / 1.047197551196598;\n"
              "if (C < -2.0) color.rgb = vec3 (0.0, -C-2.0, 1.0);\n"
              "else if (C < -1.0) color.rgb = vec3 (C+2.0, 0.0, 1.0);\n"
              "else if (C < 0.0) color.rgb = vec3 (1.0, 0.0, -C);\n"
              "else if (C < 1.0) color.rgb = vec3 (1.0, C, 0.0);\n"
              "else if (C < 2.0) color.rgb = vec3 (2.0-C, 1.0, 0.0);\n"
              "else color.rgb = vec3 (0.0, 1.0, C-2.0);\n"
              "color.rgb = scale * (amplitude - offset) * color.rgb;\n",
              Entry::basic_map_fn(),
              "length (color.rg)",
              true),

          Entry (NULL, NULL, Entry::basic_map_fn(), NULL, true)
        };


  }
}

