/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

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

#ifndef __dwi_tractography_file_h__
#define __dwi_tractography_file_h__

#include <fstream>
#include <map>

#include "types.h"
#include "point.h"
#include "file/key_value.h"
#include "dwi/tractography/file_base.h"
#include "dwi/tractography/properties.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {

      template <typename T = float> class Reader : public __ReaderBase__
      {
        public:
          typedef T value_type;

          Reader (const std::string& file, Properties& properties) {
            open (file, "tracks", properties);
          }

          bool next (std::vector< Point<value_type> >& tck)
          {
            tck.clear();

            if (!in.is_open())
              return false;
            do {
              Point<value_type> p = get_next_point();
              if (isinf (p[0])) {
                in.close();
                return false;
              }
              if (in.eof()) {
                in.close();
                return false;
              }

              if (isnan (p[0]))
                return true;
              tck.push_back (p);
            } while (in.good());

            in.close();
            return false;
          }

          bool operator() (std::vector< Point<value_type> >& tck)
          {
            return next (tck);
          }



        protected:
          using __ReaderBase__::in;
          using __ReaderBase__::dtype;

          Point<value_type> get_next_point ()
          { 
            using namespace ByteOrder;
            switch (dtype()) {
              case DataType::Float32LE: 
                {
                  Point<float> p;
                  in.read ((char*) &p, sizeof (p));
                  return (Point<value_type> (LE(p[0]), LE(p[1]), LE(p[2])));
                }
              case DataType::Float32BE:
                {
                  Point<float> p;
                  in.read ((char*) &p, sizeof (p));
                  return (Point<value_type> (BE(p[0]), BE(p[1]), BE(p[2])));
                }
              case DataType::Float64LE:
                {
                  Point<double> p;
                  in.read ((char*) &p, sizeof (p));
                  return (Point<value_type> (LE(p[0]), LE(p[1]), LE(p[2])));
                }
              case DataType::Float64BE:
                {
                  Point<double> p;
                  in.read ((char*) &p, sizeof (p));
                  return (Point<value_type> (BE(p[0]), BE(p[1]), BE(p[2])));
                }
              default:
                assert (0);
                break;
            }
            return (Point<value_type>());
          }

          Reader (const Reader& R) { assert (0); }

      };




      template <typename T = float> class Writer : public __WriterBase__ <T>
      {
        public:
          typedef T value_type;
          using __WriterBase__<T>::count;
          using __WriterBase__<T>::total_count;
          using __WriterBase__<T>::out;
          using __WriterBase__<T>::dtype;
          using __WriterBase__<T>::create;

          Writer (const std::string& file, const Properties& properties)
          {
            create (file, properties, "tracks");
            write_next_point (Point<value_type> (INFINITY, INFINITY, INFINITY));
          }

          void append (const std::vector< Point<value_type> >& tck)
          {
            if (tck.size()) {
              int64_t current (out.tellp());
              current -= sizeof (Point<value_type>);
              if (tck.size()) {
                for (typename std::vector<Point<value_type> >::const_iterator i = tck.begin()+1; i != tck.end(); ++i)
                  write_next_point (*i);
                write_next_point (Point<value_type> (NAN, NAN, NAN));
              }
              write_next_point (Point<value_type> (INFINITY, INFINITY, INFINITY));
              int64_t end (out.tellp());
              out.seekp (current);
              write_next_point (tck.size() ? tck[0] : Point<value_type> (NAN, NAN, NAN));
              out.seekp (end);

              count++;
            }
            total_count++;
          }

          bool operator() (const std::vector< Point<value_type> >& tck)
          {
            append (tck);
            return true;
          }


        protected:

          void write_next_point (const Point<value_type>& p) 
          {
            using namespace ByteOrder;
            value_type x[3];
            if (dtype.is_little_endian()) { x[0] = LE(p[0]); x[1] = LE(p[1]); x[2] = LE(p[2]); }
            else { x[0] = BE(p[0]); x[1] = BE(p[1]); x[2] = BE(p[2]); }
            out.write ((const char*) x, 3*sizeof(value_type));
          }

          Writer (const Writer& W) { assert (0); }

      };

    }
  }
}


#endif

