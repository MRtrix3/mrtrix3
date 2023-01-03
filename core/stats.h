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

#ifndef __stats_h_
#define __stats_h_


#include "app.h"
#include "file/ofstream.h"
#include "math/median.h"


namespace MR
{

  namespace Stats
  {

    extern const char * field_choices[];
    extern const App::OptionGroup Options;

    using value_type = default_type;
    using complex_type = cdouble;


    class Stats { NOMEMALIGN
      public:
        Stats (const bool is_complex = false, const bool ignorezero = false) :
            mean (0.0, 0.0),
            delta (0.0, 0.0),
            delta2 (0.0, 0.0),
            m2 (0.0, 0.0),
            std (0.0, 0.0),
            std_rv (0.0, 0.0),
            min (INFINITY, INFINITY),
            max (-INFINITY, -INFINITY),
            count (0),
            is_complex (is_complex),
            ignore_zero (ignorezero) { }


        void operator() (complex_type val) {
          if (std::isfinite (val.real()) && std::isfinite (val.imag()) && !(ignore_zero && val.real() == 0.0 && val.imag() == 0.0)) {
            if (min.real() > val.real()) min = complex_type (val.real(), min.imag());
            if (min.imag() > val.imag()) min = complex_type (min.real(), val.imag());
            if (max.real() < val.real()) max = complex_type (val.real(), max.imag());
            if (max.imag() < val.imag()) max = complex_type (max.real(), val.imag());
            count++;
            // Welford's online algorithm for variance calculation:
            delta = val - mean;
            mean += cdouble(delta.real() / count, delta.imag() / count);
            delta2 = val - mean;
            m2 += cdouble(delta.real() * delta2.real(), delta.imag() * delta2.imag());
            if (!is_complex)
              values.push_back(val.real());
          }
        }

        template <class ImageType> void print (ImageType& ima, const vector<std::string>& fields) {

          if (count > 1) {
            std = complex_type(sqrt (m2.real() / value_type (count - 1)), sqrt (m2.imag() / value_type (count - 1)));
            std_rv = complex_type(sqrt((m2.real() + m2.imag()) / value_type (count - 1)));
            std::sort (values.begin(), values.end());
          }
          if (fields.size()) {
            if (!count) {
              if (fields.size() == 1 && fields.front() == "count") {
                std::cout << "0\n";
                return;
              } else {
                throw Exception ("Cannot output statistic of interest; no values read (empty mask?)");
              }
            }
            for (size_t n = 0; n < fields.size(); ++n) {
              if (fields[n] == "mean") std::cout << str(mean) << " ";
              else if (fields[n] == "median") std::cout << ( values.size() > 0 ? str(Math::median (values)) : "N/A" ) << " ";
              else if (fields[n] == "std") std::cout << ( count > 1 ? str(std) : "N/A" ) << " ";
              else if (fields[n] == "std_rv") std::cout << ( count > 1 ? str(std_rv) : "N/A" ) << " ";
              else if (fields[n] == "min") std::cout << str(min) << " ";
              else if (fields[n] == "max") std::cout << str(max) << " ";
              else if (fields[n] == "count") std::cout << count << " ";
              else throw Exception("stats type not supported: " + fields[n]);
            }
            std::cout << "\n";

          }
          else {

            std::string s = "[ ";
            if (ima.ndim() > 3)
              for (size_t n = 3; n < ima.ndim(); n++)
                s += str (ima.index(n)) + " ";
            else
              s += "0 ";
            s += "]";

            int width = is_complex ? 20 : 10;
            std::cout << std::setw(12) << std::right << s << " ";

            std::cout << std::setw(width) << std::right << ( count ? str(mean) : "N/A" );

            if (!is_complex) {
              std::cout << " " << std::setw(width) << std::right;
              if (count)
                std::cout << Math::median (values);
              else
                std::cout << "N/A";
            }
            std::cout << " " << std::setw(width) << std::right << ( count > 1 ? str(std) : "N/A" )
              << " " << std::setw(width) << std::right << ( count ? str(min) : "N/A" )
              << " " << std::setw(width) << std::right << ( count ? str(max) : "N/A" )
              << " " << std::setw(10) << std::right << count << "\n";
          }

        }

      private:
        complex_type mean, delta, delta2, m2, std, std_rv, min, max;
        size_t count;
        const bool is_complex, ignore_zero;
        vector<float> values;
    };



    inline void print_header (bool is_complex)
    {
      int width = is_complex ? 20 : 10;
      std::cout << std::setw(12) << std::right << "volume"
        << " " << std::setw(width) << std::right << "mean";
      if (!is_complex)
        std::cout << " " << std::setw(width) << std::right << "median";
      std::cout  << " " << std::setw(width) << std::right << "std"
        << " " << std::setw(width) << std::right << "min"
        << " " << std::setw(width) << std::right << "max"
        << " " << std::setw(10) << std::right << "count" << "\n";
    }

  }

}



#endif

