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

#ifndef __fixel_legacy_metric_h__
#define __fixel_legacy_metric_h__

#include "types.h"

namespace MR
{
  namespace Fixel
  {

    namespace Legacy
    {


      // A class for storing a single quantitative value per fixel
      // This simple class will form the basis of most fixel-based image outputs and statistical analysis
      // Members are:
      // * 'dir': orientation of fixel on unit vector xyz triplet
      // * 'size': parameter related to the size of the fixel (e.g. FOD lobe integral, bolume fraction, FOD peak amplitude)
      // * 'value': the parameteric value of interest associated with the fixel
      class FixelMetric
      { MEMALIGN (FixelMetric)
        public:
          FixelMetric (const Eigen::Vector3f& d, const float s, const float v) :
            dir (d),
            size (s),
            value (v) { }
          FixelMetric () :
            dir (),
            size (0.0),
            value (0.0) { }
          Eigen::Vector3f dir;
          float size;
          float value;
      };

    }
  }
}

#endif




