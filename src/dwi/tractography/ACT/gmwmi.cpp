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


#include "dwi/tractography/ACT/gmwmi.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace ACT
      {


        bool GMWMI_finder::find_interface (Point<float>& p) const
        {
          Interp interp (interp_template);
          return find_interface (p, interp);
        }



        Point<float> GMWMI_finder::normal (const Point<float>& p) const
        {
          Interp interp (interp_template);
          return get_normal (p, interp);
        }


        bool GMWMI_finder::is_cgm (const Point<float>& p) const
        {
          Interp interp (interp_template);
          interp.scanner2voxel (p);
          const Tissues tissues (interp);
          return (tissues.valid() && (tissues.get_sgm() > tissues.get_cgm()));
        }





        Point<float> GMWMI_finder::find_interface (const std::vector< Point<float> >& tck, const bool end) const
        {
          Interp interp (interp_template);
          return find_interface (tck, end, interp);
        }




        void GMWMI_finder::crop_track (std::vector< Point<float> >& tck) const
        {
          if (tck.size() < 3)
            return;
          Interp interp (interp_template);
          const Point<float> new_first_point = find_interface (tck, false, interp);
          tck[0] = new_first_point;
          const Point<float> new_last_point = find_interface (tck, true, interp);
          tck[tck.size() - 1] = new_last_point;
        }





        bool GMWMI_finder::find_interface (Point<float>& p, Interp& interp) const {

          Tissues tissues;

          Point<float> step (0.0, 0.0, 0.0);
          size_t gradient_iters = 0;

          tissues = get_tissues (p, interp);

          do {
            step = get_cf_min_step (p, interp);
            p += step;
            tissues = get_tissues (p, interp);
          } while (tissues.valid() && step.norm2() && (std::abs (tissues.get_gm() - tissues.get_wm()) > GMWMI_ACCURACY) && (++gradient_iters < GMWMI_MAX_ITERS_TO_FIND_BOUNDARY));

          // Make sure an appropriate cost function minimum has been found, and that
          //   this would be an acceptable termination point if it were processed by the tracking algorithm
          if (!tissues.valid() || tissues.is_csf() || tissues.is_path() || !tissues.get_wm()
              || (std::abs (tissues.get_gm() - tissues.get_wm()) > GMWMI_ACCURACY)) {

            p.invalidate();
            return false;

          }

          if (tissues.get_gm() >= tissues.get_wm())
            return true;

          step = get_cf_min_step (p, interp);
          if (!step.valid())
            return true;
          if (!step.norm2()) {
            p.invalidate();
            return false;
          }

          do {
            step *= 1.5;
            p += step;
            tissues = get_tissues (p, interp);

            if (tissues.valid() && (tissues.get_gm() >= tissues.get_wm()) && (std::abs (tissues.get_gm() - tissues.get_wm()) > GMWMI_ACCURACY))
              return true;

          } while (step.norm() < 0.5 * min_vox);

          p.invalidate();
          return false;

        }



        Point<float> GMWMI_finder::get_normal (const Point<float>& p, Interp& interp) const
        {

          Point<float> normal (0.0, 0.0, 0.0);

          for (size_t axis = 0; axis != 3; ++axis) {

            Point<float> p_minus (p);
            p_minus[axis] -= 0.5 * GMWMI_PERTURBATION;
            const Tissues v_minus = get_tissues (p_minus, interp);

            Point<float> p_plus (p);
            p_plus[axis] += 0.5 * GMWMI_PERTURBATION;
            const Tissues v_plus  = get_tissues (p_plus,  interp);

            normal[axis] = (v_plus.get_wm() - v_plus.get_gm()) - (v_minus.get_wm() - v_minus.get_gm());

          }

          return (normal.normalise());

        }





        Point<float> GMWMI_finder::get_cf_min_step (const Point<float>& p, Interp& interp) const
        {

          Point<float> grad (0.0, 0.0, 0.0);

          for (size_t axis = 0; axis != 3; ++axis) {

            Point<float> p_minus (p);
            p_minus[axis] -= 0.5 * GMWMI_PERTURBATION;
            const Tissues v_minus = get_tissues (p_minus, interp);

            Point<float> p_plus (p);
            p_plus[axis] += 0.5 * GMWMI_PERTURBATION;
            const Tissues v_plus  = get_tissues (p_plus,  interp);

            if (!v_minus.valid() || !v_plus.valid())
              return Point<float> (0.0, 0.0, 0.0);

            grad[axis] = (v_plus.get_gm() - v_plus.get_wm()) - (v_minus.get_gm() - v_minus.get_wm());

          }

          grad *= (1.0 / GMWMI_PERTURBATION);

          if (!grad.norm2())
            return Point<float> (0.0, 0.0, 0.0);

          const Tissues local_tissue = get_tissues (p, interp);
          const float diff = local_tissue.get_gm() - local_tissue.get_wm();

          Point<float> step = -grad * (diff / grad.norm2());

          const float norm = step.norm();
          if (norm > 0.5 * min_vox)
            step *= 0.5 * min_vox / norm;
          return step;

        }





        Point<float> GMWMI_finder::find_interface (const std::vector< Point<float> >& tck, const bool end, Interp& interp) const
        {

          if (tck.size() == 0)
            return Point<float>();

          if (tck.size() == 1)
            return tck.front();

          if (tck.size() == 2)
            return (end ? tck.back() : tck.front());

          // Track is long enough; can do the proper search
          // Need to generate an additional point beyond the end point
          typedef Point<float> PointF;
          size_t last = tck.size() - 1;

          const PointF p_end  (end ? tck.back()  : tck.front());
          const PointF p_prev (end ? tck[last-1] : tck[1]);

          // Before proceeding, make sure that the interface lies somewhere in between these two points
          if (interp.scanner (p_end))
            return p_end;
          const Tissues t_end (interp);
          if (interp.scanner (p_prev))
            return p_end;
          const Tissues t_prev (interp);
          if (! (((t_end.get_gm() > t_end.get_wm()) && (t_prev.get_gm() < t_prev.get_wm()))
              || ((t_end.get_gm() < t_end.get_wm()) && (t_prev.get_gm() > t_prev.get_wm())))) {
            return p_end;
          }

          // Also make sure that the existing endpoint doesn't already obey the criterion
          if (t_end.get_gm() - t_end.get_wm() < GMWMI_ACCURACY)
            return p_end;

          const PointF curvature (end ? ((tck[last]-tck[last-1]) - (tck[last-1]-tck[last-2])) : ((tck[0]-tck[1]) - (tck[1]-tck[2])));
          const PointF extrap ((end ? (tck[last]-tck[last-1]) : (tck[0]-tck[1])) + curvature);
          const PointF p_extrap (p_end + extrap);

          Point<float> domain [4];
          domain[0] = (end ? tck[last-2] : tck[2]);
          domain[1] = p_prev;
          domain[2] = p_end;
          domain[3] = p_extrap;

          Math::Hermite<float> hermite (GMWMI_HERMITE_TENSION);

          float min_mu = 0.0, max_mu = 1.0;
          Point<float> p_best = p_end;
          size_t iters = 0;
          do {
            const float mu =  0.5 * (min_mu + max_mu);
            hermite.set (mu);
            const Point<float> p (hermite.value (domain));
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


