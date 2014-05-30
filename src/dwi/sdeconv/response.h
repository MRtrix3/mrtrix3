/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 2014

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

#ifndef __dwi_sdeconv_response_h__
#define __dwi_sdeconv_response_h__

#include "math/matrix.h"
#include "math/hermite.h"


namespace MR {
  namespace DWI {

    extern const size_t default_response_coefficients_size[2];
    extern const float default_response_coefficients[];

    //! a class to evaluate the response function for any b-value
    /*! This is initialised using a matrix where the first column corresponds
     * to the b-value, and each column thereafter to the corresponding
     * spherical harmonic coefficient in order of increasing even degree \e l.
     * If supplied with a filename, the matrix will be read as space-delimited
     * ASCII text from that file.
     *
     * The value() method returns the corresponding \e l coefficient at the
     * b-value previously specified by set_bval().
     *
     * If not explicitly initialised, default response function values will be
     * used, as evaluated in Tournier et al., NMR Biomed 26: 1775-86 (2013).
     *
     * If the response function contains only a single row, it is assumed to
     * correspond to a single-shell response, and the first entry is
     * interpreted as the \e l = 0 term (this is consistent with previous
     * implementations). In this case, the set_bval() method has no effect, and
     * the value() method always return the corresponding coefficient. */
    template <typename ValueType>
      class Response {
        public:
          Response () { 
            const Math::Matrix<float> tmp (const_cast<float*> (default_response_coefficients), default_response_coefficients_size[0], default_response_coefficients_size[1]);
            init (tmp);
          }

          Response (const std::string& response_file) {
            load (response_file);
          }

          template <typename X> 
            void init (const Math::Matrix<X>& coefficients) {
              interp.clear();
              coefs = coefficients;
              if (coefs.columns() == 1)
                coefs = Math::transpose (coefs);
              if (!single_shell()) {
                interp.init (coefs.column (0));
                Math::Matrix<ValueType> tmp (coefs.sub (0, coefs.rows(), 1, coefs.columns()));
                coefs = tmp;
              }
            }

          void load (const std::string& response_file) {
            Math::Matrix<ValueType> tmp;
            tmp.load (response_file);
            init (tmp);
          }

          bool single_shell () const { 
            return coefs.rows() <= 1;
          }

          size_t lmax () const { 
            return (coefs.columns()-1)*2;
          }

          void set_bval (ValueType bval) const {
            if (single_shell()) 
              return;
            interp.set (bval);
          }

          ValueType value (int l) const {
            return single_shell() ? coefs(0,l/2) : interp.value (coefs.column (l/2));
          }

        private:
          Math::Matrix<ValueType> coefs;
          mutable Math::HermiteSplines<ValueType> interp;
      };

  }
}


#endif

