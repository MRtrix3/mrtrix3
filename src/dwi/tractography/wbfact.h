/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 25/10/09.

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

#ifndef __dwi_tractography_WBFACT_h__
#define __dwi_tractography_WBFACT_h__

#include "dwi/bootstrap.h"
#include "dwi/tractography/fact.h"


namespace MR {
  namespace DWI {
    namespace Tractography {

      class WBFACT : public FACT {
        public:
          class Shared : public FACT::Shared {
            public:
              Shared (const std::string& source_name, DWI::Tractography::Properties& property_set) :
                FACT::Shared (source_name, property_set) {

                properties["method"] = "WBFACT";

                mult (Hat, bmat, binv);
              }

              Math::Matrix<value_type> Hat;
          };






          WBFACT (const Shared& shared) :
            FACT (shared),
            S (shared),
            source (Bootstrap<SourceBufferType::voxel_type,WildBootstrap> (S.source_voxel, WildBootstrap (S.Hat, rng))) { }

          WBFACT (const WBFACT& F) :
            FACT (F.S),
            S (F.S),
            source (Bootstrap<SourceBufferType::voxel_type,WildBootstrap> (S.source_voxel, WildBootstrap (S.Hat, rng))) { }




          bool init () {
            source.clear();

            if (!source.get (pos, values))
              return false;

            return do_init();
          }



          bool next () {
            if (!source.get (pos, values))
              return false;

            return do_next();
          }

        protected:

          class WildBootstrap {
            public:
              WildBootstrap (const Math::Matrix<value_type>& hat_matrix, Math::RNG& random_number_generator) :
                H (hat_matrix),
                rng (random_number_generator),
                residuals (H.rows()),
                log_signal (H.rows()) { }

              void operator() (value_type* data) {
                for (size_t i = 0; i < residuals.size(); ++i)
                  log_signal[i] = data[i] > value_type (0.0) ? -Math::log (data[i]) : value_type (0.0);

                mult (residuals, H, log_signal);

                for (size_t i = 0; i < residuals.size(); ++i) {
                  residuals[i] = Math::exp (-residuals[i]) - data[i];
                  data[i] += rng.uniform_int (2) ? residuals[i] : -residuals[i];
                }
              }

            private:
              const Math::Matrix<value_type>& H;
              Math::RNG& rng;
              Math::Vector<value_type> residuals, log_signal;
          };

          class Interp : public Interpolator<Bootstrap<SourceBufferType::voxel_type,WildBootstrap> >::type {
            public:
              Interp (const Bootstrap<SourceBufferType::voxel_type,WildBootstrap>& bootstrap_vox) :
                Interpolator<Bootstrap<SourceBufferType::voxel_type,WildBootstrap> >::type (bootstrap_vox) { 
                  for (size_t i = 0; i < 8; ++i)
                    raw_signals.push_back (new value_type [dim (3)]);
                }

              VecPtr<value_type,true> raw_signals;

              bool get (const Point<value_type>& pos, std::vector<value_type>& data) {
                scanner (pos);

                if (out_of_bounds) {
                  std::fill (data.begin(), data.end(), NAN);
                  return false;
                }

                std::fill (data.begin(), data.end(), 0.0);

                if (faaa) {
                  get_values (raw_signals[0]); // aaa
                  for (size_t i = 0; i < data.size(); ++i)
                    data[i] +=  faaa * raw_signals[0][i];
                }

                ++(*this)[2];
                if (faab) {
                  get_values (raw_signals[1]); // aab
                  for (size_t i = 0; i < data.size(); ++i)
                    data[i] +=  faab * raw_signals[1][i];
                }

                ++(*this)[1];
                if (fabb) {
                  get_values (raw_signals[3]); // abb
                  for (size_t i = 0; i < data.size(); ++i)
                    data[i] +=  fabb * raw_signals[3][i];
                }

                --(*this)[2];
                if (faba) {
                  get_values (raw_signals[2]); // aba
                  for (size_t i = 0; i < data.size(); ++i)
                    data[i] +=  faba * raw_signals[2][i];
                }

                ++(*this)[0];
                if (fbba) {
                  get_values (raw_signals[6]); // bba
                  for (size_t i = 0; i < data.size(); ++i)
                    data[i] +=  fbba * raw_signals[6][i];
                }

                --(*this)[1];
                if (fbaa) {
                  get_values (raw_signals[4]); // baa
                  for (size_t i = 0; i < data.size(); ++i)
                    data[i] +=  fbaa * raw_signals[4][i];
                }

                ++(*this)[2];
                if (fbab) {
                  get_values (raw_signals[5]); // bab
                  for (size_t i = 0; i < data.size(); ++i)
                    data[i] +=  fbab * raw_signals[5][i];
                }

                ++(*this)[1];
                if (fbbb) {
                  get_values (raw_signals[7]); // bbb
                  for (size_t i = 0; i < data.size(); ++i)
                    data[i] +=  fbbb * raw_signals[7][i];
                }

                --(*this)[0];
                --(*this)[1];
                --(*this)[2];

                return !isnan (data[0]);
              }
          };


          const Shared& S;
          Interp source;

      };

    }
  }
}

#endif



