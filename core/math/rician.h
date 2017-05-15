/* Copyright (c) 2008-2017 the MRtrix3 contributors.
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


#ifndef __math_rician_h__
#define __math_rician_h__

#include "math/math.h"
#include "math/bessel.h"
#include "math/vector.h"

namespace MR
{
  namespace Math
  {
    namespace Rician
    {

      template <typename T> inline T lnP (const T measured, const T actual, const T one_over_noise_squared)
      {
        T nm = one_over_noise_squared * measured;
        T s = abs (actual);
        return 0.5 * one_over_noise_squared * pow2 (measured - s) - log (nm * Bessel::I0_scaled (nm * s));
      }


      template <typename T> inline T lnP (const T measured, T actual, const T one_over_noise_squared, T& dP_dactual)
      {
        assert (measured >= 0.0);
        assert (one_over_noise_squared > 0.0);

        actual = abs (actual);
        T nm = one_over_noise_squared * measured;
        T nms = nm * actual;
        T F0 = Bessel::I0_scaled (nms);
        T m_a = measured - actual;
        T nm_a = one_over_noise_squared * m_a;
        T F1_F0 = (Bessel::I1_scaled (nms) - F0) / F0;
        assert (nms >= 0.0); // (nms < 0.0) F1_F0 = -F1_F0;
        dP_dactual = -nm_a - nm * F1_F0;
        return 0.5 * nm_a * m_a - log (nm * F0);
      }




      template <typename T> inline T lnP (const T measured, const T actual, const T one_over_noise_squared, T& dP_dactual, T& dP_dN)
      {
        assert (measured >= 0.0);
        assert (one_over_noise_squared > 0.0);

        bool actual_is_positive = (actual >= 0.0);
        T actual_pos = abs (actual);
        T nm = one_over_noise_squared * measured;
        T nms = nm * actual_pos;
        T F0 = Bessel::I0_scaled (nms);
        T m_a = measured - actual_pos;
        T nm_a = one_over_noise_squared * m_a;
        T F1_F0 = (Bessel::I1_scaled (nms) - F0) / F0;
        if (nms < 0.0) F1_F0 = -F1_F0;
        dP_dactual = -nm_a - nm * F1_F0;
        actual_pos *= measured * F1_F0;
        dP_dN = 0.5 * pow2 (m_a) - 1.0/one_over_noise_squared + (actual_is_positive ? -actual_pos : actual_pos);
        return 0.5 * nm_a * m_a - log (nm * F0);
      }





      template <typename T> inline T lnP (const int N, const T* measured, const T* actual, const T one_over_noise_squared, T* dP_dactual, T& dP_dN)
      {
        assert (one_over_noise_squared > 0.0);

        T lnP = 0.0;
        dP_dN = -T (N) / one_over_noise_squared;

        for (int i = 0; i < N; i++) {
          assert (measured[i] >= 0.0);

          T actual_pos = abs (actual[i]);
          T nm = one_over_noise_squared * measured[i];
          T nms = nm * actual_pos;
          T F0 = Bessel::I0_scaled (nms);
          T m_a = measured[i] - actual_pos;
          T nm_a = one_over_noise_squared * m_a;
          T F1_F0 = (Bessel::I1_scaled (nms) - F0) / F0;
          dP_dactual[i] = -nm_a - nm * F1_F0;
          if (actual[i] < 0.0) dP_dactual[i] = -dP_dactual[i];
          dP_dN += 0.5 * pow2 (m_a) - measured[i] * actual_pos * F1_F0;
          lnP += 0.5 * nm_a * m_a - log (nm * F0);
        }

        return lnP;
      }



      template <typename T> inline T lnP (const Vector<T>& measured, const Vector<T>& actual, const T one_over_noise_squared, Vector<T>& dP_dactual)
      {
        assert (one_over_noise_squared > 0.0);
        assert (measured.size() == actual.size());
        assert (measured.size() == dP_dactual.size());

        T lnP = 0.0;

        for (size_t i = 0; i < measured.size(); i++) {
          assert (measured[i] > 0.0);

          T actual_pos = abs (actual[i]);
          T nm = one_over_noise_squared * measured[i];
          T nms = nm * actual_pos;
          T F0 = Bessel::I0_scaled (nms);
          T m_a = measured[i] - actual_pos;
          T nm_a = one_over_noise_squared * m_a;
          T F1_F0 = (Bessel::I1_scaled (nms) - F0) / F0;
          dP_dactual[i] = -nm_a - nm * F1_F0;
          if (actual[i] < 0.0) dP_dactual[i] = -dP_dactual[i];
          lnP += 0.5 * nm_a * m_a - log (nm * F0);
          assert (std::isfinite (lnP));
        }

        return lnP;
      }


      template <typename T> inline T lnP (const Vector<T>& measured, const Vector<T>& actual, const T one_over_noise_squared, Vector<T>& dP_dactual, T& dP_dN)
      {
        assert (one_over_noise_squared > 0.0);
        assert (measured.size() == actual.size());
        assert (measured.size() == dP_dactual.size());

        T lnP = 0.0;
        dP_dN = -T (measured.size()) / one_over_noise_squared;

        for (size_t i = 0; i < measured.size(); i++) {
          assert (measured[i] > 0.0);

          T actual_pos = std::abs (actual[i]);
          T nm = one_over_noise_squared * measured[i];
          T nms = nm * actual_pos;
          T F0 = Bessel::I0_scaled (nms);
          T m_a = measured[i] - actual_pos;
          T nm_a = one_over_noise_squared * m_a;
          T F1_F0 = (Bessel::I1_scaled (nms) - F0) / F0;
          dP_dactual[i] = -nm_a - nm * F1_F0;
          if (actual[i] < 0.0) dP_dactual[i] = -dP_dactual[i];
          dP_dN += 0.5 * pow2 (m_a) - measured[i] * actual_pos * F1_F0;
          lnP += 0.5 * nm_a * m_a - std::log (nm * F0);
          assert (std::isfinite (dP_dN));
          assert (std::isfinite (lnP));
        }

        return lnP;
      }

    }
  }
}

#endif
