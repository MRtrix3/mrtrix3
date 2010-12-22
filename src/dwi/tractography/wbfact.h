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
              Shared (Image::Header& source, DWI::Tractography::Properties& property_set) :
                FACT::Shared (source, property_set) {

                properties["method"] = "WBFACT";

                mult (Hat, bmat, binv);
              }

              Math::Matrix<value_type> Hat;
          };






          WBFACT (const Shared& shared) :
            FACT (shared),
            S (shared),
            wb_functor (S.Hat, rng),
            wb_set (source, wb_functor) {
            for (size_t i = 0; i < 8; ++i)
              raw_signals.push_back (new value_type [source.dim (3)]);
          }

          WBFACT (const WBFACT& F) :
            FACT (F.S),
            S (F.S),
            wb_functor (S.Hat, rng),
            wb_set (source, wb_functor) {
            for (size_t i = 0; i < 8; ++i)
              raw_signals.push_back (new value_type [source.dim (3)]);
          }




          bool init () {
            wb_set.clear();

            if (!get_WB_data ())
              return false;

            return do_init();
          }



          bool next () {
            if (!get_WB_data ())
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

          class Interp : public DataSet::Interp::Linear<Bootstrap<StorageType,WildBootstrap> > {
            public:
              Interp (Bootstrap<StorageType,WildBootstrap>& set) :
                DataSet::Interp::Linear<Bootstrap<StorageType,WildBootstrap> > (set) { }

              void get (
                value_type* data,
                size_t num,
                const value_type* aaa,
                const value_type* aab,
                const value_type* aba,
                const value_type* abb,
                const value_type* baa,
                const value_type* bab,
                const value_type* bba,
                const value_type* bbb) const {
                if (out_of_bounds) {
                  for (size_t i = 0; i < num; ++i)
                    data[i] = NAN;
                  return;
                }

                memset (data, 0, num*sizeof (value_type));

                if (faaa)
                  for (size_t i = 0; i < num; ++i)
                    data[i] +=  faaa * aaa[i];

                if (faab)
                  for (size_t i = 0; i < num; ++i)
                    data[i] +=  faab * aab[i];

                if (faba)
                  for (size_t i = 0; i < num; ++i)
                    data[i] +=  faba * aba[i];

                if (fabb)
                  for (size_t i = 0; i < num; ++i)
                    data[i] +=  fabb * abb[i];

                if (fbaa)
                  for (size_t i = 0; i < num; ++i)
                    data[i] +=  fbaa * baa[i];

                if (fbab)
                  for (size_t i = 0; i < num; ++i)
                    data[i] +=  fbab * bab[i];

                if (fbba)
                  for (size_t i = 0; i < num; ++i)
                    data[i] +=  fbba * bba[i];

                if (fbbb)
                  for (size_t i = 0; i < num; ++i)
                    data[i] +=  fbbb * bbb[i];

              }
          };





          const Shared& S;
          WildBootstrap wb_functor;
          Bootstrap<StorageType,WildBootstrap> wb_set;
          VecPtr<value_type,true> raw_signals;

          bool get_WB_data () {
            interp.scanner (pos);
            if (!interp)
              return false;
            wb_set.get_values (raw_signals[0]); // aaa
            ++wb_set[2];
            wb_set.get_values (raw_signals[1]); // aab
            ++wb_set[1];
            wb_set.get_values (raw_signals[3]); // abb
            --wb_set[2];
            wb_set.get_values (raw_signals[2]); // aba
            ++wb_set[0];
            wb_set.get_values (raw_signals[6]); // bba
            --wb_set[1];
            wb_set.get_values (raw_signals[4]); // baa
            ++wb_set[2];
            wb_set.get_values (raw_signals[5]); // bab
            ++wb_set[1];
            wb_set.get_values (raw_signals[7]); // bbb
            --wb_set[0];
            --wb_set[1];
            --wb_set[2];

            const DataSet::Interp::Linear<StorageType>* std_interp = &interp;
            reinterpret_cast<const Interp*> (std_interp)->get (values, source.dim (3),
                raw_signals[0], raw_signals[1], raw_signals[2], raw_signals[3],
                raw_signals[4], raw_signals[5], raw_signals[6], raw_signals[7]);

            return !isnan (values[0]);
          }

      };

    }
  }
}

#endif



