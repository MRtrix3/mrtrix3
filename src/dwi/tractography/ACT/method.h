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

#ifndef __dwi_tractography_act_method_h__
#define __dwi_tractography_act_method_h__



#include "point.h"

#include "dwi/tractography/ACT/act.h"
#include "dwi/tractography/ACT/tissues.h"

#include "dwi/tractography/tracking/shared.h"
#include "dwi/tractography/tracking/types.h"

#include "image/interp/linear.h"


#define GMWMI_NORMAL_PERTURBATION 0.001


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace ACT
      {


      using namespace MR::DWI::Tractography::Tracking;

        class ACT_Method_additions {

          public:
            ACT_Method_additions (const SharedBase& shared) :
                sgm_depth (0),
                seed_in_sgm (false),
                sgm_seed_to_wm (false),
                act_image (shared.act().voxel) { }

            ACT_Method_additions (const ACT_Method_additions&) = delete;
            ACT_Method_additions() = delete;


            const Tissues& tissues() const { return tissue_values; }


            term_t check_structural (const Point<value_type>& pos)
            {
              if (!fetch_tissue_data (pos))
                return EXIT_IMAGE;

              if (tissues().is_csf())
                return (sgm_depth ? EXIT_SGM : ENTER_CSF);

              if (tissues().is_gm()) {
                if (tissues().get_cgm() >= tissues().get_sgm())
                  return ENTER_CGM;
                ++sgm_depth;
              } else if (sgm_depth) {
                if (seed_in_sgm && !sgm_seed_to_wm) {
                  sgm_seed_to_wm = true;
                  sgm_depth = 0;
                  return CONTINUE;
                }
                return EXIT_SGM;
              }

              return CONTINUE;
            }


            bool check_seed (const Point<value_type>& pos)
            {
              sgm_depth = 0;

              if (!fetch_tissue_data (pos))
                return false;

              if (tissues().is_sgm()) {
                seed_in_sgm = true;
                sgm_seed_to_wm = false;
                return true;
              }

              seed_in_sgm = false;

              if ((tissues().is_csf()) || !tissues().get_wm() || ((tissues().get_gm() - tissues().get_wm()) >= GMWMI_ACCURACY))
                return false;

              return true;
            }


            bool seed_is_unidirectional (const Point<value_type>& pos, Point<value_type>& dir)
            {
              // Tissue values should have already been acquired for the seed point when this function is run
              if (tissues().is_sgm()) return false;
              if ((tissues().get_wm() >= tissues().get_gm()) || (tissues().get_sgm() >= tissues().get_cgm()))
                return false;

              const Tissues tissues_at_pos (tissues());

              const Point<float> pos_plus  (pos + (dir * GMWMI_NORMAL_PERTURBATION));
              fetch_tissue_data (pos_plus);
              const Tissues tissues_plus (tissues());

              const Point<float> pos_minus (pos - (dir * GMWMI_NORMAL_PERTURBATION));
              fetch_tissue_data (pos_minus);
              const Tissues& tissues_minus (tissues());

              const float gradient = (tissues_plus.get_gm() - tissues_plus.get_wm()) - (tissues_minus.get_gm() - tissues_minus.get_wm());
              if (gradient > 0.0)
                dir = -dir;

              // Restore the tissue values to those at the seed point
              tissue_values = tissues_at_pos;
              return true;
            }


            bool fetch_tissue_data (const Point<value_type>& pos)
            {
              act_image.scanner (pos);
              if (!act_image) {
                tissue_values.reset();
                return false;
              }
              return tissue_values.set (act_image);
            }


            bool in_pathology() const { return (tissue_values.valid() && tissue_values.is_path()); }


            size_t sgm_depth;
            bool seed_in_sgm;
            bool sgm_seed_to_wm;


          private:
            Image::Interp::Linear< Image::Buffer<float>::voxel_type > act_image;
            Tissues tissue_values;

        };


      }
    }
  }
}

#endif

