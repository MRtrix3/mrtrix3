/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 2012.

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

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
      namespace ACT
      {


        class GMWMI_finder
        {

          protected:
            typedef Interp::Linear<Image<float>> Interp;

          public:
            GMWMI_finder (Image<float>& buffer) :
              interp_template (buffer),
              min_vox (minvalue (buffer.voxsize(0), buffer.voxsize(1), buffer.voxsize(2))) { }

            GMWMI_finder (const GMWMI_finder& that) :
              interp_template (that.interp_template),
              min_vox (that.min_vox) { }


            bool find_interface (Eigen::Vector3d&) const;
            Eigen::Vector3d normal (const Eigen::Vector3d&) const;
            bool is_cgm (const Eigen::Vector3d&) const;


            Eigen::Vector3d find_interface (const std::vector<Eigen::Vector3d>&, const bool) const;
            void crop_track (std::vector<Eigen::Vector3d>&) const;


          protected:
            const Interp interp_template;
            const float min_vox;


            Tissues get_tissues (const Eigen::Vector3d& p, Interp& interp) const {
              if (interp.scanner (p))
                return Tissues ();
              return Tissues (interp);
            }

            bool find_interface (Eigen::Vector3d&, Interp&) const;
            Eigen::Vector3d get_normal (const Eigen::Vector3d&, Interp&) const;
            Eigen::Vector3d get_cf_min_step (const Eigen::Vector3d&, Interp&) const;

            Eigen::Vector3d find_interface (const std::vector<Eigen::Vector3d>&, const bool, Interp&) const;


            friend class Track_extender;

        };



      }
    }
  }
}

#endif
