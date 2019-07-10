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


#include "math/math.h"

namespace MR
{
  namespace Math
  {


    //! Compute the inverse of the error function
    /**  Implementation based on Boost Math (https://github.com/boostorg/math)
     * While the erfinv() function is exposed in the API, it will
     * encounter floating-point precision issues if provided with
     * input values greater than approx. +0.999999 or smaller than
     * approx. -0.999999. If such extreme values are likely to arise
     * during processing, it is highly recommended that programmers
     * instead operate natively on the values q = 1.0 - p and/or
     * qc = 1.0 + p, and pass the appropriate value (q or qc) directly
     * to the erfcinv() function. This will ensure full use of
     * available floating-point precision within these functions,
     * which is otherwise lost if performing one of the above conversions
     * (either implicitly within one of these functions, or explicitly
     * by the programmer prior to invoking erfinv()).
     */
    default_type erfinv  (const default_type);


    //! Compute the inverse of the complementary error function
    //*  Implementation based on Boost Math (https://github.com/boostorg/math) */
    default_type erfcinv (const default_type);



  }
}

