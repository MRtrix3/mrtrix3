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


#ifndef __image_check__h__
#define __image_check__h__

#include "image_helpers.h"
#include "algo/loop.h"

namespace MR
{

   //! check image headers are the same (dimensions, spacing & transform)
  template <class HeaderType1, class HeaderType2>
    inline void check_headers (HeaderType1& in1, HeaderType2& in2)
    {
      check_dimensions (in1, in2);
      for (size_t i = 0; i < in1.ndim(); ++i) {
        if (std::isfinite (in1.spacing(i)))
          if (in1.spacing(i) != in2.spacing(i))
            throw Exception ("images \"" + in1.name() + "\" and \"" + in2.name() + "\" do not have matching voxel spacings " +
                                           str(in1.spacing(i)) + " vs " + str(in2.spacing(i)));
      }
      for (size_t i  = 0; i < 3; ++i) {
        for (size_t j  = 0; j < 4; ++j) {
          if (std::abs (in1.transform().matrix()(i,j) - in2.transform().matrix()(i,j)) > 0.001)
            throw Exception ("images \"" + in1.name() + "\" and \"" + in2.name() + "\" do not have matching header transforms "
                               + "\n" + str(in1.transform().matrix()) + "vs \n " + str(in2.transform().matrix()) + ")");
        }
      }
    }


  //! check images are the same within a absolute tolerance
  template <class ImageType1, class ImageType2>
    inline void check_images_abs (ImageType1& in1, ImageType2& in2, const double tol = 0.0)
    {
      check_headers (in1, in2);
      ThreadedLoop (in1)
      .run ([&tol] (const decltype(in1)& a, const decltype(in2)& b) {
          if (std::abs (cdouble (a.value()) - cdouble (b.value())) > tol)
            throw Exception ("images \"" + a.name() + "\" and \"" + b.name() + "\" do not match within absolute precision of " + str(tol)
                 + " (" + str(cdouble (a.value())) + " vs " + str(cdouble (b.value())) + ")");
          }, in1, in2);
    }


  //! check images are the same within a fractional tolerance
  template <class ImageType1, class ImageType2>
    inline void check_images_frac (ImageType1& in1, ImageType2& in2, const double tol = 0.0) {

      check_headers (in1, in2);
      ThreadedLoop (in1)
      .run ([&tol] (const decltype(in1)& a, const decltype(in2)& b) {
          if (std::abs ((cdouble (a.value()) - cdouble (b.value())) / (0.5 * (cdouble (a.value()) + cdouble (b.value())))) > tol)
          throw Exception ("images \"" + a.name() + "\" and \"" + b.name() + "\" do not match within fractional precision of " + str(tol)
               + " (" + str(cdouble (a.value())) + " vs " + str(cdouble (b.value())) + ")");
          }, in1, in2);
    }

  //! check images are the same within a tolerance defined by a third image
  template <class ImageType1, class ImageType2, class ImageTypeTol>
    inline void check_images_tolimage (ImageType1& in1, ImageType2& in2, ImageTypeTol& tol) {

      check_headers (in1, in2);
      check_headers (in1, tol);
      ThreadedLoop (in1)
      .run ([] (const decltype(in1)& a, const decltype(in2)& b, const decltype(tol)& t) {
          if (std::abs (cdouble (a.value()) - cdouble (b.value())) > t.value())
          throw Exception ("images \"" + a.name() + "\" and \"" + b.name() + "\" do not match within precision of \"" + t.name() + "\""
               + " (" + str(cdouble (a.value())) + " vs " + str(cdouble (b.value())) + ", tolerance " + str(t.value()) + ")");
          }, in1, in2, tol);
    }

  //! check images are the same within a fractional tolerance relative to the maximum value in the voxel
  template <class ImageType1, class ImageType2>
    inline void check_images_voxel (ImageType1& in1, ImageType2& in2, const double tol = 0.0)
    {
      auto func = [&tol] (decltype(in1)& a, decltype(in2)& b) {
        double maxa = 0.0, maxb = 0.0;
        for (auto l = Loop(3) (a, b); l; ++l) {
          maxa = std::max (maxa, std::abs (cdouble(a.value())));
          maxb = std::max (maxb, std::abs (cdouble(b.value())));
        }
        const double threshold = tol * 0.5 * (maxa + maxb);
        for (auto l = Loop(3) (a, b); l; ++l) {
          if (std::abs (cdouble (a.value()) - cdouble (b.value())) > threshold)
            throw Exception ("images \"" + a.name() + "\" and \"" + b.name() + "\" do not match within " + str(tol) + " of maximal voxel value"
                           + " (" + str(cdouble (a.value())) + " vs " + str(cdouble (b.value())) + ")");
        }
      };

      ThreadedLoop (in1, 0, 3).run (func, in1, in2);
    }

    //! check image headers are the same (dimensions, spacing & transform)
   template <class HeaderType1, class HeaderType2>
     inline bool headers_match (HeaderType1& in1, HeaderType2& in2)
     {
       if (!dimensions_match (in1, in2))
         return false;
       if (!spacings_match (in1, in2))
         return false;
       if (!transforms_match (in1, in2))
         return false;
       return true;
     }


   //! check images are the same within a absolute tolerance
   template <class ImageType1, class ImageType2>
     inline bool images_match_abs (ImageType1& in1, ImageType2& in2, const double tol = 0.0)
     {
       headers_match (in1, in2);
       for (auto i = Loop (in1)(in1, in2); i; ++i)
         if (std::abs (cdouble (in1.value()) - cdouble (in2.value())) > tol)
           return false;
       return true;
     }






}

#endif





