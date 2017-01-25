/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */



#include "dwi/tractography/ACT/gmwmi.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace ACT
      {


        bool GMWMI_finder::find_interface (Eigen::Vector3f& p) const
        {
          Interp interp (interp_template);
          return find_interface (p, interp);
        }



        Eigen::Vector3f GMWMI_finder::normal (const Eigen::Vector3f& p) const
        {
          Interp interp (interp_template);
          return get_normal (p, interp);
        }


        bool GMWMI_finder::is_cgm (const Eigen::Vector3f& p) const
        {
          Interp interp (interp_template);
          // TODO: this is no-op, what was it doing here?!?
          // interp.scanner2voxel (p);
          const Tissues tissues (interp);
          return (tissues.valid() && (tissues.get_sgm() > tissues.get_cgm()));
        }





        Eigen::Vector3f GMWMI_finder::find_interface (const std::vector< Eigen::Vector3f >& tck, const bool end) const
        {
          Interp interp (interp_template);
          return find_interface (tck, end, interp);
        }




        void GMWMI_finder::crop_track (std::vector< Eigen::Vector3f >& tck) const
        {
          if (tck.size() < 3)
            return;
          Interp interp (interp_template);
          const Eigen::Vector3f new_first_point = find_interface (tck, false, interp);
          tck[0] = new_first_point;
          const Eigen::Vector3f new_last_point = find_interface (tck, true, interp);
          tck[tck.size() - 1] = new_last_point;
        }





        bool GMWMI_finder::find_interface (Eigen::Vector3f& p, Interp& interp) const {

          Tissues tissues;

          Eigen::Vector3f step (0.0, 0.0, 0.0);
          size_t gradient_iters = 0;

          tissues = get_tissues (p, interp);

          do {
            step = get_cf_min_step (p, interp);
            p += step;
            tissues = get_tissues (p, interp);
          } while (tissues.valid() && step.squaredNorm() && 
              (std::abs (tissues.get_gm() - tissues.get_wm()) > GMWMI_ACCURACY) && 
              (++gradient_iters < GMWMI_MAX_ITERS_TO_FIND_BOUNDARY));

          // Make sure an appropriate cost function minimum has been found, and that
          //   this would be an acceptable termination point if it were processed by the tracking algorithm
          if (!tissues.valid() || tissues.is_csf() || tissues.is_path() || !tissues.get_wm()
              || (std::abs (tissues.get_gm() - tissues.get_wm()) > GMWMI_ACCURACY)) {

            p = { NaN, NaN, NaN };
            return false;

          }

          if (tissues.get_gm() >= tissues.get_wm())
            return true;

          step = get_cf_min_step (p, interp);
          if (!step.allFinite())
            return true;
          if (!step.squaredNorm()) {
            p = { NaN, NaN, NaN };
            return false;
          }

          do {
            step *= 1.5;
            p += step;
            tissues = get_tissues (p, interp);

            if (tissues.valid() && (tissues.get_gm() >= tissues.get_wm()) && (tissues.get_gm() - tissues.get_wm() < GMWMI_ACCURACY))
              return true;

          } while (step.norm() < 0.5 * min_vox);

          p = { NaN, NaN, NaN };
          return false;

        }



        Eigen::Vector3f GMWMI_finder::get_normal (const Eigen::Vector3f& p, Interp& interp) const
        {

          Eigen::Vector3f normal (0.0, 0.0, 0.0);

          for (size_t axis = 0; axis != 3; ++axis) {

            Eigen::Vector3f p_minus (p);
            p_minus[axis] -= 0.5 * GMWMI_PERTURBATION;
            const Tissues v_minus = get_tissues (p_minus, interp);

            Eigen::Vector3f p_plus (p);
            p_plus[axis] += 0.5 * GMWMI_PERTURBATION;
            const Tissues v_plus  = get_tissues (p_plus,  interp);

            normal[axis] = (v_plus.get_wm() - v_plus.get_gm()) - (v_minus.get_wm() - v_minus.get_gm());

          }

          return normal.normalized();

        }





        Eigen::Vector3f GMWMI_finder::get_cf_min_step (const Eigen::Vector3f& p, Interp& interp) const
        {

          Eigen::Vector3f grad (0.0, 0.0, 0.0);

          for (size_t axis = 0; axis != 3; ++axis) {

            Eigen::Vector3f p_minus (p);
            p_minus[axis] -= 0.5 * GMWMI_PERTURBATION;
            const Tissues v_minus = get_tissues (p_minus, interp);

            Eigen::Vector3f p_plus (p);
            p_plus[axis] += 0.5 * GMWMI_PERTURBATION;
            const Tissues v_plus  = get_tissues (p_plus,  interp);

            if (!v_minus.valid() || !v_plus.valid())
              return { 0.0, 0.0, 0.0 };

            grad[axis] = (v_plus.get_gm() - v_plus.get_wm()) - (v_minus.get_gm() - v_minus.get_wm());

          }

          grad *= (1.0 / GMWMI_PERTURBATION);

          if (!grad.squaredNorm())
            return { 0.0, 0.0, 0.0 };

          const Tissues local_tissue = get_tissues (p, interp);
          const float diff = local_tissue.get_gm() - local_tissue.get_wm();

          Eigen::Vector3f step = -grad * (diff / grad.squaredNorm());

          const float norm = step.norm();
          if (norm > 0.5 * min_vox)
            step *= 0.5 * min_vox / norm;
          return step;

        }





        Eigen::Vector3f GMWMI_finder::find_interface (const std::vector<Eigen::Vector3f>& tck, const bool end, Interp& interp) const
        {

          if (tck.size() == 0)
            return { NaN, NaN, NaN };

          if (tck.size() == 1)
            return tck.front();

          if (tck.size() == 2)
            return (end ? tck.back() : tck.front());

          // Track is long enough; can do the proper search
          // Need to generate an additional point beyond the end point
          size_t last = tck.size() - 1;

          const Eigen::Vector3f p_end  (end ? tck.back()  : tck.front());
          const Eigen::Vector3f p_prev (end ? tck[last-1] : tck[1]);

          // Before proceeding, make sure that the interface lies somewhere in between these two points
          if (!interp.scanner (p_end))
            return p_end;
          const Tissues t_end (interp);
          if (!interp.scanner (p_prev))
            return p_end;
          const Tissues t_prev (interp);
          if (! (((t_end.get_gm() > t_end.get_wm()) && (t_prev.get_gm() < t_prev.get_wm()))
              || ((t_end.get_gm() < t_end.get_wm()) && (t_prev.get_gm() > t_prev.get_wm())))) {
            return p_end;
          }

          // Also make sure that the existing endpoint doesn't already obey the criterion
          if (t_end.get_gm() - t_end.get_wm() < GMWMI_ACCURACY)
            return p_end;

          const Eigen::Vector3f curvature (end ? ((tck[last]-tck[last-1]) - (tck[last-1]-tck[last-2])) : ((tck[0]-tck[1]) - (tck[1]-tck[2])));
          const Eigen::Vector3f extrap ((end ? (tck[last]-tck[last-1]) : (tck[0]-tck[1])) + curvature);
          const Eigen::Vector3f p_extrap (p_end + extrap);

          Eigen::Vector3f domain [4];
          domain[0] = (end ? tck[last-2] : tck[2]);
          domain[1] = p_prev;
          domain[2] = p_end;
          domain[3] = p_extrap;

          Math::Hermite<float> hermite (GMWMI_HERMITE_TENSION);

          float min_mu = 0.0, max_mu = 1.0;
          Eigen::Vector3f p_best = p_end;
          size_t iters = 0;
          do {
            const float mu =  0.5 * (min_mu + max_mu);
            hermite.set (mu);
            const Eigen::Vector3f p (hermite.value (domain));
            interp.scanner (p);
            const Tissues t (interp);
            if (t.get_wm() > t.get_gm()) {
              min_mu = mu;
            } else {
              max_mu = mu;
              p_best = p;
              if (t.get_gm() - t.get_wm() < GMWMI_ACCURACY)
                return p_best;
            }
          } while (++iters != GMWMI_MAX_ITERS_TO_FIND_BOUNDARY);

          return p_best;

        }






      }
    }
  }
}


