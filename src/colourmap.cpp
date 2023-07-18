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

          // The Inferno and Viridis colour maps are implemented using a 6th order polynomial approximation of the originals,
          // ported from https://www.shadertoy.com/view/WlfXRN (public domain; CC0 license).

          Entry ("Inferno",
              "const vec3 c0 = vec3(0.0002189403691192265, 0.001651004631001012, -0.01948089843709184);\n"
              "const vec3 c1 = vec3(0.1065134194856116, 0.5639564367884091, 3.932712388889277);\n"
              "const vec3 c2 = vec3(11.60249308247187, -3.972853965665698, -15.9423941062914);\n"
              "const vec3 c3 = vec3(-41.70399613139459, 17.43639888205313, 44.35414519872813);\n"
              "const vec3 c4 = vec3(77.162935699427, -33.40235894210092, -81.80730925738993);\n"
              "const vec3 c5 = vec3(-71.31942824499214, 32.62606426397723, 73.20951985803202);\n"
              "const vec3 c6 = vec3(25.13112622477341, -12.24266895238567, -23.07032500287172);\n"
              "color.rgb = clamp( c0 + amplitude*(c1 + amplitude*(c2 + amplitude*(c3 + amplitude*(c4 + amplitude*(c5 + amplitude*c6))))), 0.0, 1.0);\n",
              [] (float amplitude) {
                Eigen::Array3f c0 (0.0002189403691192265, 0.001651004631001012, -0.01948089843709184);
                Eigen::Array3f c1 (0.1065134194856116, 0.5639564367884091, 3.932712388889277);
                Eigen::Array3f c2 (11.60249308247187, -3.972853965665698, -15.9423941062914);
                Eigen::Array3f c3 (-41.70399613139459, 17.43639888205313, 44.35414519872813);
                Eigen::Array3f c4 (77.162935699427, -33.40235894210092, -81.80730925738993);
                Eigen::Array3f c5 (-71.31942824499214, 32.62606426397723, 73.20951985803202);
                Eigen::Array3f c6 (25.13112622477341, -12.24266895238567, -23.07032500287172);  
                Eigen::Array3f rgb = c0 + amplitude*(c1 + amplitude*(c2 + amplitude*(c3 + amplitude*(c4 + amplitude*(c5 + amplitude*c6)))));
                rgb = rgb.max(0.0).min(1.0);
                return rgb;
              }),
          
          Entry ("Viridis",
              "const vec3 c0 = vec3(0.2777273272234177, 0.005407344544966578, 0.3340998053353061);\n"
              "const vec3 c1 = vec3(0.1050930431085774, 1.404613529898575, 1.384590162594685);\n"
              "const vec3 c2 = vec3(-0.3308618287255563, 0.214847559468213, 0.09509516302823659);\n"
              "const vec3 c3 = vec3(-4.634230498983486, -5.799100973351585, -19.33244095627987);\n"
              "const vec3 c4 = vec3(6.228269936347081, 14.17993336680509, 56.69055260068105);\n"
              "const vec3 c5 = vec3(4.776384997670288, -13.74514537774601, -65.35303263337234);\n"
              "const vec3 c6 = vec3(-5.435455855934631, 4.645852612178535, 26.3124352495832);\n"
              "color.rgb = clamp( c0 + amplitude*(c1 + amplitude*(c2 + amplitude*(c3 + amplitude*(c4 + amplitude*(c5 + amplitude*c6))))), 0.0, 1.0);\n",
              [] (float amplitude) {
                Eigen::Array3f c0 (0.2777273272234177, 0.005407344544966578, 0.3340998053353061);
                Eigen::Array3f c1 (0.1050930431085774, 1.404613529898575, 1.384590162594685);
                Eigen::Array3f c2 (-0.3308618287255563, 0.214847559468213, 0.09509516302823659);
                Eigen::Array3f c3 (-4.634230498983486, -5.799100973351585, -19.33244095627987);
                Eigen::Array3f c4 (6.228269936347081, 14.17993336680509, 56.69055260068105);
                Eigen::Array3f c5 (4.776384997670288, -13.74514537774601, -65.35303263337234);
                Eigen::Array3f c6 (-5.435455855934631, 4.645852612178535, 26.3124352495832);
                Eigen::Array3f rgb = c0 + amplitude*(c1 + amplitude*(c2 + amplitude*(c3 + amplitude*(c4 + amplitude*(c5 + amplitude*c6)))));
                rgb = rgb.max(0.0).min(1.0);
                return rgb;
              }),

          Entry ("PET",
              "color.r = clamp (2.0*amplitude - 0.5, 0.0, 1.0);\n"
              "color.g = clamp (2.0 * (0.25 - abs (amplitude - 0.25)), 0.0, 1.0) + clamp (2.0*amplitude - 1.0, 0.0, 1.0);\n"
              "color.b = 1.0 - (clamp (1.0 - 2.0 * amplitude, 0.0, 1.0) + clamp (1.0 - 4.0 * abs (amplitude - 0.75), 0.0, 1.0));\n",
              [] (float amplitude) { return Eigen::Array3f (clamp (2.0f * amplitude - 0.5f),
                                                            clamp (2.0f * (0.25f - abs (amplitude - 0.25f))) + clamp (2.0f * amplitude - 1.0f),
                                                            1.0f - (clamp (1.0f - 2.0f * amplitude) + clamp (1.0f - 4.0f * abs (amplitude - 0.75f)))); }),

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

