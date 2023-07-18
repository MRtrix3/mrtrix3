/* Copyright (c) 2008-2023 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#ifndef __image_check__h__
#define __image_check__h__

#include <set>

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
          if (abs ((in1.spacing(i) - in2.spacing(i)) / (in1.spacing(i) + in2.spacing(i))) > 1e-4)
            throw Exception ("images \"" + in1.name() + "\" and \"" + in2.name() + "\" do not have matching voxel spacings on axis " + str(i) +
                " (" + str(in1.spacing(i)) + " vs " + str(in2.spacing(i)) + ")");
      }
      for (size_t i  = 0; i < 3; ++i) {
        for (size_t j  = 0; j < 4; ++j) {
          if (abs (in1.transform().matrix()(i,j) - in2.transform().matrix()(i,j)) > 0.001)
            throw Exception ("images \"" + in1.name() + "\" and \"" + in2.name() + "\" do not have matching header transforms:\n" +
                str(in1.transform().matrix()) + "\nvs:\n " + str(in2.transform().matrix()) + ")");
        }
      }
    }


  //! check images are the same within a absolute tolerance
  template <class ImageType1, class ImageType2>
    inline void check_images_abs (ImageType1& in1, ImageType2& in2, const double tol = 0.0)
    {
      check_headers (in1, in2);
      ThreadedLoop (in1)
      .run ([&tol] (const ImageType1& a, const ImageType2& b) {
          if (abs (cdouble (a.value()) - cdouble (b.value())) > tol)
            throw Exception ("images \"" + a.name() + "\" and \"" + b.name() + "\" do not match within absolute precision of " + str(tol)
                 + " (" + str(cdouble (a.value())) + " vs " + str(cdouble (b.value())) + ")");
          }, in1, in2);
    }


  //! check images are the same within a fractional tolerance
  template <class ImageType1, class ImageType2>
    inline void check_images_frac (ImageType1& in1, ImageType2& in2, const double tol = 0.0) {

      check_headers (in1, in2);
      ThreadedLoop (in1)
      .run ([&tol] (const ImageType1& a, const ImageType2& b) {
          if (abs ((cdouble (a.value()) - cdouble (b.value())) / (0.5 * (cdouble (a.value()) + cdouble (b.value())))) > tol)
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
      .run ([] (const ImageType1& a, const ImageType2& b, const ImageTypeTol& t) {
          if (abs (cdouble (a.value()) - cdouble (b.value())) > t.value())
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
          maxa = std::max (maxa, abs (cdouble(a.value())));
          maxb = std::max (maxb, abs (cdouble(b.value())));
        }
        const double threshold = tol * 0.5 * (maxa + maxb);
        for (auto l = Loop(3) (a, b); l; ++l) {
          if (abs (cdouble (a.value()) - cdouble (b.value())) > threshold)
            throw Exception ("images \"" + a.name() + "\" and \"" + b.name() + "\" do not match within " + str(tol) + " of maximal voxel value"
                           + " (" + str(cdouble (a.value())) + " vs " + str(cdouble (b.value())) + ")");
        }
      };

      ThreadedLoop (in1, 0, 3).run (func, in1, in2);
    }

  //! check headers contain the same key-value entries
  template <class HeaderType1, class HeaderType2>
    inline void check_keyvals (const HeaderType1& in1, const HeaderType2& in2)
    {
      const static std::set<std::string> reserved { "command_history", "mrtrix_version", "project_version" };
      auto it1 = in1.keyval().cbegin();
      auto it2 = in2.keyval().cbegin();
      Exception errors;
      while (it1 != in1.keyval().end() || it2 != in2.keyval().end()) {
        while (it1 != in1.keyval().end() && reserved.find (it1->first) != reserved.end())
          ++it1;
        while (it2 != in2.keyval().end() && reserved.find (it2->first) != reserved.end())
          ++it2;

        if (it1 == in1.keyval().end() || it2 == in2.keyval().end())
          break;

        if (it1 == in1.keyval().end()) {
          errors.push_back ("Key \"" + it2->first + "\" in image \"" + in2.name() + "\" not present in \"" + in1.name() + "\"");
          ++it2;
        }
        else if (it2 == in2.keyval().end()) {
          errors.push_back ("Key \"" + it1->first + "\" in image \"" + in1.name() + "\" not present in \"" + in2.name() + "\"");
          ++it1;
        }
        else if (it1->first < it2->first) {
          errors.push_back ("Key \"" + it1->first + "\" in image \"" + in1.name() + "\" not present in \"" + in2.name() + "\"");
          ++it1;
        }
        else if (it1->first > it2->first) {
          errors.push_back ("Key \"" + it2->first + "\" in image \"" + in2.name() + "\" not present in \"" + in1.name() + "\"");
          ++it2;
        }
        else {
          if (it1->second != it2->second)
            errors.push_back ("Key \"" + it1->first + "\" has different values between images");
          ++it1;
          ++it2;
        }
      }

      if (errors.num() > 0)
        throw errors;
    }

    //! check image headers are the same (dimensions, spacing & transform)
   template <class HeaderType1, class HeaderType2>
     inline bool headers_match (HeaderType1& in1, HeaderType2& in2)
     {
       if (!dimensions_match (in1, in2))
         return false;
       if (!spacings_match (in1, in2, 1e-6)) // implicitly checked in voxel_grids_match_in_scanner_space but with different tolerance
         return false;
       if (!voxel_grids_match_in_scanner_space (in1, in2))
         return false;
       return true;
     }


   //! check images are the same within a absolute tolerance
   template <class ImageType1, class ImageType2>
     inline bool images_match_abs (ImageType1& in1, ImageType2& in2, const double tol = 0.0)
     {
       if (!headers_match (in1, in2))
         return false;
       for (auto i = Loop (in1)(in1, in2); i; ++i)
         if (abs (cdouble (in1.value()) - cdouble (in2.value())) > tol)
           return false;
       return true;
     }






}

#endif





