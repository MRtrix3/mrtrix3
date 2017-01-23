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
#ifndef __stats_h_
#define __stats_h_


#include "app.h"
#include "math/median.h"


#define DEFAULT_HISTOGRAM_BINS 100


namespace MR
{

  namespace Stats
  {

    extern const char * field_choices[];
    extern const App::OptionGroup Options;

    typedef float value_type;
    typedef cfloat complex_type;


    class CalibrateHistogram { NOMEMALIGN
      public:
        CalibrateHistogram (int nbins) : min (std::numeric_limits<value_type>::infinity()), max (-std::numeric_limits<value_type>::infinity()), width (0.0), bins (nbins) { }

        value_type min, max, width;
        int bins;

        void operator() (value_type val) {
          if (std::isfinite (val)) {
            if (val < min) min = val;
            if (val > max) max = val;
          }
        }

        void init (std::ostream& stream) {
          width = (max - min) / float (bins+1);
          for (int i = 0; i < bins; i++)
            stream << (min + width/2.0) + i* width << " ";
          stream << "\n";
        }
    };



    class Stats { NOMEMALIGN
      public:
        Stats (bool is_complex = false) :
          mean (0.0, 0.0),
          std (0.0, 0.0),
          min (INFINITY, INFINITY),
          max (-INFINITY, -INFINITY),
          count (0),
          dump (NULL),
          is_complex (is_complex) { }

        void generate_histogram (const CalibrateHistogram& cal) {
          hmin = cal.min;
          hwidth = cal.width;
          hist.resize (cal.bins);
        }

        void dump_to (std::ostream& stream) {
          dump = &stream;
        }

        void write_histogram (std::ostream& stream) {
          for (size_t i = 0; i < hist.size(); ++i)
            stream << hist[i] << " ";
          stream << "\n";
        }


        void operator() (complex_type val) {
          if (std::isfinite (val.real()) && std::isfinite (val.imag())) {
            mean += val;
            std += cdouble (val.real()*val.real(), val.imag()*val.imag());
            if (min.real() > val.real()) min = complex_type (val.real(), min.imag());
            if (min.imag() > val.imag()) min = complex_type (min.real(), val.imag());
            if (max.real() < val.real()) max = complex_type (val.real(), max.imag());
            if (max.imag() < val.imag()) max = complex_type (max.real(), val.imag());
            count++;

            if (dump)
              *dump << str(val) << "\n";

            if (!is_complex)
              values.push_back(val.real());


            if (hist.size()) {
              int bin = int ( (val.real()-hmin) / hwidth);
              if (bin < 0)
                bin = 0;
              else if (bin >= int (hist.size()))
                bin = hist.size()-1;
              hist[bin]++;
            }
          }
        }

        template <class ImageType> void print (ImageType& ima, const vector<std::string>& fields) {

          if (count) {
            mean /= double (count);
            std = complex_type (sqrt (std.real()/double(count) - mean.real()*mean.real()),
                sqrt (std.imag()/double(count) - mean.imag()*mean.imag()));
          }

          std::sort (values.begin(), values.end());

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
              else if (fields[n] == "median") std::cout << Math::median (values) << " ";
              else if (fields[n] == "std") std::cout << str(std) << " ";
              else if (fields[n] == "min") std::cout << str(min) << " ";
              else if (fields[n] == "max") std::cout << str(max) << " ";
              else if (fields[n] == "count") std::cout << count << " ";
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
            s += "] ";

            int width = is_complex ? 24 : 12;
            std::cout << std::setw(15) << std::right << s << " ";

            std::cout << std::setw(width) << std::right << ( count ? str(mean) : "N/A" );

            if (!is_complex) {
              std::cout << " " << std::setw(width) << std::right << ( count ? str(Math::median (values)) : "N/A" );
            }
            std::cout << " " << std::setw(width) << std::right << ( count > 1 ? str(std) : "N/A" )
              << " " << std::setw(width) << std::right << ( count ? str(min) : "N/A" )
              << " " << std::setw(width) << std::right << ( count ? str(max) : "N/A" )
              << " " << std::setw(12) << std::right << count << "\n";
          }

        }

      private:
        cdouble mean, std;
        complex_type min, max;
        size_t count;
        value_type hmin, hwidth;
        vector<size_t> hist;
        std::ostream* dump;
        bool is_complex;
        vector<float> values;
    };



    inline void print_header (bool is_complex)
    {
      int width = is_complex ? 24 : 12;
      std::cout << std::setw(15) << std::right << "volume"
        << " " << std::setw(width) << std::right << "mean";
      if (!is_complex)
        std::cout << " " << std::setw(width) << std::right << "median";
      std::cout  << " " << std::setw(width) << std::right << "std. dev."
        << " " << std::setw(width) << std::right << "min"
        << " " << std::setw(width) << std::right << "max"
        << " " << std::setw(12) << std::right << "count\n";
    }

  }

}



#endif

