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
#include "dwi/tractography/properties.h"

namespace MR {
  namespace DWI {
    namespace Tractography {

      //! \cond skip
      class __ReaderBase__ 
      {
        public:
          void open (const std::string& file, Properties& properties);
        protected:
          std::ifstream  in;
          DataType  dtype;
          size_t  count;
      };
      //! \endcond



      template <typename T = float> class Reader : public __ReaderBase__
      {
        public:
          typedef T value_type;

          Reader () { }
          Reader (const Reader& R) { assert (0); }

          Reader (const std::string& file, Properties& properties) {
            open (file, properties);
          }

          bool next (std::vector<Point<value_type> >& tck)
          {
            tck.clear();

            if (!in.is_open()) return (false);
            do {
              Point<value_type> p = get_next_point();
              if (isinf (p[0])) {
                in.close();
                return (false);
              }
              if (in.eof()) {
                in.close();
                return (true);
              }

              if (isnan (p[0])) return (true);
              tck.push_back (p);
            } while (in.good());

            in.close();
            return (false);
          }

          void close () { in.close(); }

        protected:
          using __ReaderBase__::in;
          using __ReaderBase__::dtype;
          using __ReaderBase__::count;

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
            }
            return (Point<value_type>());
          }

      };




      template <typename T = float> class Writer 
      {
        public:
          typedef T value_type;

          Writer () : 
            count (0), total_count (0), dtype (DataType::from<value_type>()) {
              dtype.set_byte_order_native(); 
              if (dtype != DataType::Float32LE && dtype != DataType::Float32BE && 
                  dtype != DataType::Float64LE && dtype != DataType::Float64BE)
                throw Exception ("only supported datatype for tracks file are "
                    "Float32LE, Float32BE, Float64LE & Float64BE");
            }

          Writer (const std::string& file, const Properties& properties) {
            create (file, properties);
          }

          Writer (const Writer& W) { assert (0); }

          void create (const std::string& file, const Properties& properties)
          {
            out.open (file.c_str(), std::ios::out | std::ios::binary);
            if (!out) 
              throw Exception ("error creating tracks file \"" + file + "\": " + strerror (errno));

            out << "mrtrix tracks\nEND\n";
            for (Properties::const_iterator i = properties.begin(); i != properties.end(); ++i) 
              out << i->first << ": " << i->second << "\n";

            for (std::vector<std::string>::const_iterator i = properties.comments.begin(); 
                i != properties.comments.end(); ++i)
              out << "comment: " << *i << "\n";

            for (size_t n = 0; n < properties.seed.size(); ++n)
              out << "roi: seed " << properties.seed[n].parameters() << "\n";
            for (size_t n = 0; n < properties.include.size(); ++n)
              out << "roi: include " << properties.include[n].parameters() << "\n";
            for (size_t n = 0; n < properties.exclude.size(); ++n)
              out << "roi: exclude " << properties.exclude[n].parameters() << "\n";
            for (size_t n = 0; n < properties.mask.size(); ++n)
              out << "roi: mask " << properties.mask[n].parameters() << "\n";

            for (std::multimap<std::string,std::string>::const_iterator 
                it = properties.roi.begin(); it != properties.roi.end(); ++it) 
              out << "roi: " << it->first << " " << it->second << "\n";

            out << "datatype: " << dtype.specifier() << "\n";
            int64_t data_offset = int64_t(out.tellp()) + 65;
            out << "file: . " << data_offset << "\n";
            out << "count: ";
            count_offset = out.tellp();
            out << "\nEND\n";
            out.seekp (0);
            out << "mrtrix tracks    ";
            out.seekp (data_offset);
            write_next_point (Point<value_type> (INFINITY, INFINITY, INFINITY));
          }


          void append (const std::vector<Point<value_type> >& tck)
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


          void close ()
          {
            out.seekp (count_offset);
            out << count << "\ntotal_count: " << total_count << "\nEND\n";
            out.close();
          }

          size_t count, total_count;

        protected:
          std::ofstream  out;
          DataType dtype;
          int64_t  count_offset;

          void write_next_point (const Point<value_type>& p) 
          {
            using namespace ByteOrder;
            value_type x[3];
            if (dtype.is_little_endian()) { x[0] = LE(p[0]); x[1] = LE(p[1]); x[2] = LE(p[2]); }
            else { x[0] = BE(p[0]); x[1] = BE(p[1]); x[2] = BE(p[2]); }
            out.write ((const char*) x, 3*sizeof(value_type));
          }
      };


    }
  }
}


#endif

