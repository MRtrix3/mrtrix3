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


#ifndef __dwi_tractography_algorithms_tensor_det_h__
#define __dwi_tractography_algorithms_tensor_det_h__

// These lines are to silence deprecation warnings with Eigen & GCC v5
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <Eigen/Eigenvalues> 
#pragma GCC diagnostic pop

#include "math/least_squares.h"
#include "dwi/gradient.h"
#include "dwi/tensor.h"
#include "dwi/tractography/tracking/method.h"
#include "dwi/tractography/tracking/shared.h"
#include "dwi/tractography/tracking/types.h"



namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Algorithms
      {


    using namespace MR::DWI::Tractography::Tracking;


    class Tensor_Det : public MethodBase { MEMALIGN(Tensor_Det)
      public:
      class Shared : public SharedBase { MEMALIGN(Shared)
        public:
        Shared (const std::string& diff_path, DWI::Tractography::Properties& property_set) :
          SharedBase (diff_path, property_set) {

          if (is_act() && act().backtrack())
            throw Exception ("Backtracking not valid for deterministic algorithms");

          set_step_size (0.1);
          if (rk4) {
            INFO ("minimum radius of curvature = " + str(step_size / (max_angle_rk4 / (0.5 * Math::pi))) + " mm");
          } else {
            INFO ("minimum radius of curvature = " + str(step_size / ( 2.0 * sin (max_angle / 2.0))) + " mm");
          }

          properties["method"] = "TensorDet";

          try {
            auto grad = DWI::get_valid_DW_scheme (source);
            auto bmat_double = grad2bmatrix<double> (grad);
            binv = Math::pinv (bmat_double).cast<float>();
            bmat = bmat_double.cast<float>();
          } catch (Exception& e) {
            e.display();
            throw Exception ("Tensor-based tracking algorithms expect a DWI series as input");
          }
        }

        Eigen::MatrixXf bmat, binv;
      };






      Tensor_Det (const Shared& shared) :
        MethodBase (shared),
        S (shared),
        source (S.source),
        eig (3),
        M (3,3),
        dt (6) { }
        




      bool init()
      {
        if (!get_data (source))
          return false;
        return do_init();
      }



      term_t next ()
      {
        if (!get_data (source))
          return Tracking::EXIT_IMAGE;
        return do_next();
      }


      float get_metric()
      {
        dwi2tensor (dt, S.binv, values);
        return tensor2FA (dt);
      }


      protected:
      const Shared& S;
      Tracking::Interpolator<Image<float>>::type source;
      Eigen::SelfAdjointEigenSolver<Eigen::Matrix3f> eig;
      Eigen::Matrix3f M;
      Eigen::VectorXf dt;

      void get_EV ()
      {
        M(0,0) = dt[0];
        M(1,1) = dt[1];
        M(2,2) = dt[2];
        M(1,0) = M(0,1) = dt[3];
        M(2,0) = M(0,2) = dt[4];
        M(2,1) = M(1,2) = dt[5];

        eig.computeDirect (M);
        dir = eig.eigenvectors().col(2);
      }


      bool do_init()
      {
        dwi2tensor (dt, S.binv, values);

        if (tensor2FA (dt) < S.init_threshold)
          return false;

        get_EV();

        return true;
      }


      term_t do_next()
      {

        dwi2tensor (dt, S.binv, values);

        if (tensor2FA (dt) < S.threshold)
          return BAD_SIGNAL;

        Eigen::Vector3f prev_dir = dir;

        get_EV();

        float dot = prev_dir.dot (dir);
        if (std::abs (dot) < S.cos_max_angle)
          return HIGH_CURVATURE;

        if (dot < 0.0)
          dir = -dir;

        pos += dir * S.step_size;

        return CONTINUE;

      }


    };

      }
    }
  }
}

#endif


