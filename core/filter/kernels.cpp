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

#include "filter/kernels.h"

namespace MR
{
  namespace Filter
  {
    namespace Kernels
    {


       const kernel_type identity { 0, 0, 0,
                                    0, 0, 0,
                                    0, 0, 0,

                                    0, 0, 0,
                                    0, 1, 0,
                                    0, 0, 0,

                                    0, 0, 0,
                                    0, 0, 0,
                                    0, 0, 0 };



       const kernel_type laplacian { 2.0/26.0,   3.0/26.0, 2.0/26.0,
                                     3.0/26.0,   6.0/26.0, 3.0/26.0,
                                     2.0/26.0,   3.0/26.0, 2.0/26.0,

                                     3.0/26.0,   6.0/26.0, 3.0/26.0,
                                     6.0/26.0, -88.0/26.0, 6.0/26.0,
                                     3.0/26.0,   6.0/26.0, 3.0/26.0,

                                     2.0/26.0,   3.0/26.0, 2.0/26.0,
                                     3.0/26.0,   6.0/26.0, 3.0/26.0,
                                     2.0/26.0,   3.0/26.0, 2.0/26.0 };



       const kernel_type sharpen {  0,  0,  0,
                                    0, -1,  0,
                                    0,  0,  0,

                                    0, -1,  0,
                                   -1,  7, -1,
                                    0, -1,  0,

                                    0,  0,  0,
                                    0, -1,  0,
                                    0,  0,  0 };



       const kernel_type sobel_x { -1.0/16.0, 0, 1.0/16.0,
                                   -2.0/16.0, 0, 2.0/16.0,
                                   -1.0/16.0, 0, 1.0/16.0,

                                   -2.0/16.0, 0, 2.0/16.0,
                                   -4.0/16.0, 0, 4.0/16.0,
                                   -2.0/16.0, 0, 2.0/16.0,

                                   -1.0/16.0, 0, 1.0/16.0,
                                   -2.0/16.0, 0, 2.0/16.0,
                                   -1.0/16.0, 0, 1.0/16.0 };



       const kernel_type sobel_y { -1.0/16.0, -2.0/16.0, -1.0/16.0,
                                           0,         0,         0,
                                    1.0/16.0,  2.0/16.0,  1.0/16.0,

                                   -2.0/16.0, -4.0/16.0, -2.0/16.0,
                                           0,         0,         0,
                                    2.0/16.0,  4.0/16.0,  2.0/16.0,

                                   -1.0/16.0, -2.0/16.0, -1.0/16.0,
                                           0,         0,         0,
                                    1.0/16.0,  2.0/16.0,  1.0/16.0 };



       const kernel_type sobel_z { -1.0/16.0, -2.0/16.0, -1.0/16.0,
                                   -2.0/16.0, -4.0/16.0, -2.0/16.0,
                                   -1.0/16.0, -2.0/16.0, -1.0/16.0,

                                           0,         0,         0,
                                           0,         0,         0,
                                           0,         0,         0,

                                    1.0/16.0,  2.0/16.0,  1.0/16.0,
                                    2.0/16.0,  4.0/16.0,  2.0/16.0,
                                    1.0/16.0,  2.0/16.0,  1.0/16.0 };




      const kernel_type sobel_feldman_x { -3.0/64.0, 0,  3.0/64.0,
                                         -10.0/64.0, 0, 10.0/64.0,
                                          -3.0/64.0, 0,  3.0/64.0,

                                          -6.0/64.0, 0,  6.0/64.0,
                                         -20.0/64.0, 0, 20.0/64.0,
                                          -6.0/64.0, 0,  6.0/64.0,

                                           -3.0/64.0, 0,  3.0/64.0,
                                          -10.0/64.0, 0, 10.0/64.0,
                                           -3.0/64.0, 0,  3.0/64.0 };



      const kernel_type sobel_feldman_y { -3.0/64.0, -10.0/64.0, -3.0/64.0,
                                                  0,          0,         0,
                                           3.0/64.0,  10.0/64.0,  3.0/64.0,

                                          -6.0/64.0, -20.0/64.0, -6.0/64.0,
                                                  0,          0,         0,
                                           6.0/64.0,  20.0/64.0,  6.0/64.0,

                                          -3.0/64.0, -10.0/64.0, -3.0/64.0,
                                                  0,          0,         0,
                                           3.0/64.0,  10.0/64.0,  3.0/64.0 };


      const kernel_type sobel_feldman_z { -3.0/64.0, -10.0/64.0, -3.0/64.0,
                                          -6.0/64.0, -20.0/64.0, -6.0/64.0,
                                          -3.0/64.0, -10.0/64.0, -3.0/64.0,

                                                  0,          0,         0,
                                                  0,          0,         0,
                                                  0,          0,         0,

                                           3.0/64.0,  10.0/64.0,  3.0/64.0,
                                           6.0/64.0,  20.0/64.0,  6.0/64.0,
                                           3.0/64.0,  10.0/64.0,  3.0/64.0 };



    }
  }
}
