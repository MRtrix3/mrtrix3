/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see www.mrtrix.org
 *
 */

#ifndef __algo_histogram_h__
#define __algo_histogram_h__

#include <vector>
#include <cmath>

#include "image_helpers.h"
#include "algo/loop.h"

namespace MR
{
  namespace Algo
  {
    namespace Histogram
    {


      extern const App::OptionGroup Options;


      class Calibrator
      {

        public:
          Calibrator (const size_t number_of_bins = 0, const bool ignorezero = false) :
              min (std::numeric_limits<default_type>::infinity()),
              max (-std::numeric_limits<default_type>::infinity()),
              bin_width (NaN),
              num_bins (number_of_bins),
              ignore_zero (ignorezero) { }

          // template <class T, class  = typename T::value_type>
          // template <class T> bool operator() (const T val);

          template <typename value_type>
          typename std::enable_if<std::is_arithmetic<value_type>::value, bool>::type operator() (const value_type val) {
            if (std::isfinite(val) && !(ignore_zero && val == 0.0)) {
              min = std::min (min, default_type(val));
              max = std::max (max, default_type(val));
              if (!num_bins)
                data.push_back (default_type(val));
            }
            return true;
          }

          template <class T>
          FORCE_INLINE typename std::enable_if<!std::is_arithmetic<T>::value, bool>::type operator() (const T& val) {
            return (*this) (typename T::value_type (val));
          }

          void from_file (const std::string&);

          void finalize (const size_t num_volumes, const bool is_integer);

          default_type get_bin_centre (const size_t i) const {
            assert (i < num_bins);
            return get_min() + (get_bin_width() * (i + 0.5));
          }

          default_type get_bin_width() const { return bin_width; }
          size_t get_num_bins() const { return num_bins; }
          default_type get_min() const { return min; }
          default_type get_max() const { return max; }
          bool get_ignore_zero() const { return ignore_zero; }


        private:
          default_type min, max, bin_width;
          size_t num_bins;
          const bool ignore_zero;
          std::vector<default_type> data;

          default_type get_iqr();

      };




      class Data
      {
        public:

          typedef Eigen::Array<size_t, Eigen::Dynamic, 1> vector_type;
          typedef Eigen::Array<default_type, Eigen::Dynamic, 1> cdf_type;

          Data (const Calibrator& calibrate) :
              info (calibrate),
              list (vector_type::Zero (info.get_num_bins())) { }

          template <typename value_type>
          bool operator() (const value_type val) {
            if (std::isfinite(val) && !(info.get_ignore_zero() && val == 0.0)) {
              const size_t pos = bin (val);
              if (pos != size_t(list.size()))
                ++list[pos];
            }
            return true;
          }

          template <typename value_type>
          size_t bin (const value_type val) const {
            size_t pos = std::floor ((val - info.get_min()) / info.get_bin_width());
            if (pos > size_t(list.size())) return size();
            return pos;
          }


          size_t operator[] (const size_t index) const {
            assert (index < size_t(list.size()));
            return list[index];
          }
          size_t size() const {
            return list.size();
          }
          const Calibrator& get_calibration() const { return info; }

          const vector_type& pdf() const { return list; }
          cdf_type cdf() const;

          default_type first_min () const;

          default_type entropy () const;

        protected:
          const Calibrator info;
          vector_type list;
          friend class Kernel;
      };


      // Convenience functions for calibrating (& histograming) basic input images
      template <class ImageType>
      void calibrate (Calibrator& result, ImageType& image)
      {
        for (auto l = Loop(image) (image); l; ++l)
          result (image.value());
        result.finalize (image.ndim() > 3 ? image.size(3) : 1, std::is_integral<typename ImageType::value_type>::value);
      }

      template <class ImageType, class MaskType>
      void calibrate (Calibrator& result, ImageType& image, MaskType& mask)
      {
        if (!mask.valid()) {
          calibrate (result, image);
          return;
        }
        if (!dimensions_match (image, mask))
          throw Exception ("Image and mask for histogram generation do not match");
        for (auto l = Loop(image) (image, mask); l; ++l) {
          if (mask.value())
            result (image.value());
        }
        result.finalize (image.ndim() > 3 ? image.size(3) : 1, std::is_integral<typename ImageType::value_type>::value);
      }

      template <class ImageType>
      Data generate (ImageType& image, const size_t num_bins, const bool ignore_zero = false)
      {
        Calibrator calibrator (num_bins, ignore_zero);
        calibrate (calibrator, image);
        return generate (calibrator, image);
      }

      template <class ImageType, class MaskType>
      Data generate (ImageType& image, MaskType& mask, const size_t num_bins, const bool ignore_zero = false)
      {
        Calibrator calibrator (num_bins, ignore_zero);
        calibrate (calibrator, image, mask);
        return generate (calibrator, image, mask);
      }

      template <class ImageType>
      Data generate (const Calibrator& calibrator, ImageType& image)
      {
        Data result (calibrator);
        for (auto l = Loop(image) (image); l; ++l)
          result (typename ImageType::value_type (image.value()));
        return result;
      }

      template <class ImageType, class MaskType>
      Data generate (const Calibrator& calibrator, ImageType& image, MaskType& mask)
      {
        if (!mask.valid())
          return generate (calibrator, image);
        Data result (calibrator);
        for (auto l = Loop(image) (image, mask); l; ++l) {
          if (mask.value())
            result (typename ImageType::value_type (image.value()));
        }
        return result;
      }




      class Matcher
      {

          typedef Eigen::Array<default_type, Eigen::Dynamic, 1> vector_type;

        public:
          Matcher (const Data& input, const Data& target);

          default_type operator() (const default_type) const;

        private:
          const Calibrator calib_input, calib_target;
          vector_type mapping;

      };





    }
  }
}

#endif
