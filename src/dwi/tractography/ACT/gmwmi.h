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


#include "point.h"

#include "image/buffer.h"
#include "image/voxel.h"
#include "image/interp/linear.h"

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
            typedef Image::Interp::Linear< Image::Buffer<float>::voxel_type > Interp;

          public:
            GMWMI_finder (Image::Buffer<float>& buffer) :
              interp_template (Image::Buffer<float>::voxel_type (buffer)),
              min_vox (minvalue (buffer.vox(0), buffer.vox(1), buffer.vox(2))) { }

            GMWMI_finder (const GMWMI_finder& that) :
              interp_template (that.interp_template),
              min_vox (that.min_vox) { }


            bool find_interface (Point<float>&) const;
            Point<float> normal (const Point<float>&) const;
            bool is_cgm (const Point<float>&) const;


            Point<float> find_interface (const std::vector< Point<float> >&, const bool) const;
            void crop_track (std::vector< Point<float> >&) const;


          protected:
            const Interp interp_template;
            const float min_vox;


            Tissues get_tissues (const Point<float>& p, Interp& interp) const {
              if (interp.scanner (p))
                return Tissues ();
              return Tissues (interp);
            }

            bool find_interface (Point<float>&, Interp&) const;
            Point<float> get_normal (const Point<float>&, Interp&) const;
            Point<float> get_cf_min_step (const Point<float>&, Interp&) const;

            Point<float> find_interface (const std::vector< Point<float> >&, const bool, Interp&) const;


            friend class Track_extender;

        };



      }
    }
  }
}

#endif
