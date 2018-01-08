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


#ifndef __dwi_tractography_resampling_arc_h__
#define __dwi_tractography_resampling_arc_h__


#include "dwi/tractography/resampling/resampling.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Resampling {



        // Also handles resampling along a fixed line
        class Arc : public BaseCRTP<Arc> { MEMALIGN(Arc)

            using value_type = float;
            using point_type = Eigen::Vector3f;

          private:
            class Plane { MEMALIGN(Plane)
              public:
                Plane (const point_type& pos, const point_type& dir) :
                    n (dir)
                {
                  n.normalize();
                  d = n.dot (pos);
                }
                value_type dist (const point_type& pos) const { return n.dot (pos) - d; }
              private:
                point_type n;
                value_type d;
            };

            vector<Plane> planes;

          public:
            Arc (const size_t n, const point_type& s, const point_type& e) :
                nsamples (n),
                start (s),
                mid (0.5*(s+e)),
                end (e),
                idx_start (0),
                idx_end (0)
            {
              init_line();
            }

            Arc (const size_t n, const point_type& s, const point_type& w, const point_type& e) :
                nsamples (n),
                start (s),
                mid (0.5*(s+e)),
                end (e),
                idx_start (0),
                idx_end (0)
            {
              init_arc (w);
            }

            bool operator() (const Streamline<>&, Streamline<>&) const override;
            bool valid() const override { return nsamples; }

          private:
            const size_t nsamples;
            const point_type start, mid, end;

            mutable size_t idx_start, idx_end;
            mutable point_type start_dir, mid_dir, end_dir;

            void init_line();
            void init_arc (const point_type&);


            int state (const point_type&) const;

        };



      }
    }
  }
}

#endif



