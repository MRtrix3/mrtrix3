/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#ifndef __dwi_tractography_resampling_upsampler_h__
#define __dwi_tractography_resampling_upsampler_h__


#include "dwi/tractography/resampling/resampling.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Resampling {



        class Upsampler : public BaseCRTP<Upsampler>
        { MEMALIGN(Upsampler)

          public:
            Upsampler () :
              data (4, 3) { }

            Upsampler (const size_t os_ratio) :
              data (4, 3) {
                set_ratio (os_ratio);
              }

            Upsampler (const Upsampler& that) :
              M    (that.M),
              temp (M.rows(), 3),
              data (4, 3) { }

            ~Upsampler() { }


            bool operator() (const Streamline<>&, Streamline<>&) const override;
            bool valid () const override { return (M.rows()); }

            void set_ratio (const size_t);
            size_t get_ratio() const { return (M.rows() ? (M.rows() + 1) : 1); }

          private:
            Eigen::MatrixXf M;
            mutable Eigen::MatrixXf temp, data;

            void interp_prepare (Streamline<>&) const;
            void increment (const point_type&) const;

        };



      }
    }
  }
}

#endif



