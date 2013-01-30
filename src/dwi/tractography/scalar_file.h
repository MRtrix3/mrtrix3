/*
    Copyright 2013 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt, 29/01/13.

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

#ifndef __dwi_tractography_scalar_file_h__
#define __dwi_tractography_scalar_file_h__

#include <fstream>
#include <map>

#include "types.h"
#include "point.h"
#include "file/key_value.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/file_base.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {


      template <typename T = float> class ScalarReader : public __ReaderBase__
      {
        public:
          typedef T value_type;

          ScalarReader (const std::string& file, Properties& properties) {
            open (file, "track scalars", properties);
          }

          bool next (std::vector<value_type>& tck_scalar)
          {
            tck_scalar.clear();

            if (!in.is_open())
              return false;
            do {
              value_type val = get_next_scalar();
              if (isinf (val)) {
                in.close();
                return false;
              }
              if (in.eof()) {
                in.close();
                return false;
              }

              if (isnan (val))
                return true;
              tck_scalar.push_back (val);
            } while (in.good());

            in.close();
            return false;
          }

          bool operator() (std::vector< Point<value_type> >& tck_scalar)
          {
            return next (tck_scalar);
          }

        protected:
          using __ReaderBase__::in;
          using __ReaderBase__::dtype;

          value_type get_next_scalar ()
          {
            using namespace ByteOrder;
            switch (dtype()) {
              case DataType::Float32LE:
              {
                float val;
                in.read ((char*) &val, sizeof (val));
                return (value_type (LE(val)));
              }
              case DataType::Float32BE:
              {
                float val;
                in.read ((char*) &val, sizeof (val));
                return (value_type (BE(val)));
              }
              case DataType::Float64LE:
              {
                double val;
                in.read ((char*) &val, sizeof (val));
                return (value_type (LE(val)));
              }
              case DataType::Float64BE:
              {
                double val;
                in.read ((char*) &val, sizeof (val));
                return (value_type (BE(val)));
              }
              default:
                assert (0);
                break;
            }
            return (value_type (NAN));
          }

          ScalarReader (const ScalarReader& R) { assert (0); }

      };


      template <typename T = float> class ScalarWriter : public __WriterBase__
      {
        public:
          typedef T value_type;
          using __WriterBase__<T>::count;
          using __WriterBase__<T>::total_count;
          using __WriterBase__<T>::out;
          using __WriterBase__<T>::dtype;
          using __WriterBase__<T>::create;

          ScalarWriter (const std::string& file, const Properties& properties)
          {
            create_header (file, properties);
          }


          void append (const std::vector<value_type>& tck_scalar)
          {
            if (tck_scalar.size()) {
              int64_t current (out.tellp());
              current -= sizeof (value_type);
              if (tck_scalar.size()) {
                for (typename std::vector<value_type>::const_iterator i = tck_scalar.begin()+1; i != tck_scalar.end(); ++i)
                  write_next_point (*i);
                write_next_point (value_type(NAN));
              }
              write_next_point (value_type (INFINITY));
              int64_t end (out.tellp());
              out.seekp (current);
              write_next_point (tck_scalar.size() ? tck_scalar[0] : value_type (NAN));
              out.seekp (end);

              count++;
            }
            total_count++;
          }

          bool operator() (const std::vector<value_type>& tck_scalar)
          {
            append (tck_scalar);
            return true;
          }


          size_t count, total_count;


        protected:
          std::ofstream  out;
          DataType dtype;
          int64_t  count_offset;

          void create_header (const std::string& file, const Properties& properties)
          {
            create (file, properties);
            write_next_point (value_type (INFINITY));
          }

          void write_next_point (const value_type& val)
          {
            using namespace ByteOrder;
            value_type x;
            if (dtype.is_little_endian()) { x = LE(val); }
            else { x = BE(val);}
            out.write ((const char*) x, sizeof(value_type));
          }


          ScalarWriter (const ScalarWriter& W) { assert (0); }

      };


    }
  }
}


#endif

