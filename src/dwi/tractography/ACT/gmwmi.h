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


#ifndef __dwi_tractography_act_gmwmi_h__
#define __dwi_tractography_act_gmwmi_h__


#include "image.h"
#include "interp/linear.h"
#include "math/hermite.h"
#include "dwi/tractography/ACT/act.h"
#include "dwi/tractography/ACT/tissues.h"


#define GMWMI_PERTURBATION 0.001 // in mm
#define GMWMI_MAX_ITERS_TO_FIND_BOUNDARY 10
#define GMWMI_HERMITE_TENSION 0.1



namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {

      namespace Seeding
      {
        class Dynamic_ACT_additions;
      }

      namespace ACT
      {


        class GMWMI_finder
        { MEMALIGN(GMWMI_finder)

          protected:
            using Interp = Interp::Linear<Image<float>>;

          public:
            GMWMI_finder (const Image<float>& buffer) :
              interp_template (buffer),
              min_vox (std::min (buffer.spacing(0), std::min (buffer.spacing(1), buffer.spacing(2)))) { }

            GMWMI_finder (const Interp& interp) :
              interp_template (interp),
              min_vox (std::min (interp.spacing(0), std::min (interp.spacing(1), interp.spacing(2)))) { }

            GMWMI_finder (const GMWMI_finder& that) :
              interp_template (that.interp_template),
              min_vox (that.min_vox) { }


            bool find_interface (Eigen::Vector3f&) const;
            Eigen::Vector3f normal (const Eigen::Vector3f&) const;
            bool is_cgm (const Eigen::Vector3f&) const;


            Eigen::Vector3f find_interface (const vector<Eigen::Vector3f>&, const bool) const;
            void crop_track (vector<Eigen::Vector3f>&) const;


          protected:
            const Interp interp_template;
            const float min_vox;


            Tissues get_tissues (const Eigen::Vector3f& p, Interp& interp) const {
              if (!interp.scanner (p))
                return Tissues ();
              return Tissues (interp);
            }

            bool find_interface (Eigen::Vector3f&, Interp&) const;
            Eigen::Vector3f get_normal (const Eigen::Vector3f&, Interp&) const;
            Eigen::Vector3f get_cf_min_step (const Eigen::Vector3f&, Interp&) const;

            Eigen::Vector3f find_interface (const vector<Eigen::Vector3f>&, const bool, Interp&) const;


            friend class Track_extender;
            friend class Tractography::Seeding::Dynamic_ACT_additions;

        };



      }
    }
  }
}

#endif
