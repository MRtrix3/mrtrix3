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

#include "point.h"
#include "file/key_value.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/mds.h"

namespace MR {
  namespace DWI {
    namespace Tractography {

      class Reader {
        public:
          void open (const std::string& file, Properties& properties);
          bool next (std::vector<Point>& tck);
          void close ();

        protected:
          Ptr<MDS> mds;
          std::ifstream  in;
          DataType       dtype;
          uint          count;

          Point get_next_point ()
          { 
            using namespace ByteOrder;
            Point p;
            in.read ((char*) &p, sizeof (Point));
            if (dtype == DataType::Float32LE) { p[0] = LE(p[0]); p[1] = LE(p[1]); p[2] = LE(p[2]); }
            else { p[0] = BE(p[0]); p[1] = BE(p[1]); p[2] = BE(p[2]); }
            return (p);
          }
      };




      class Writer {
        public:
          Writer () : count (0), total_count (0), dtype (DataType::Float32) { dtype.set_byte_order_native(); }

          void create (const std::string& file, const Properties& properties);
          void append (const std::vector<Point>& tck)
          {
            off64_t current (out.tellp());
            current -= 3*sizeof(float);
            if (tck.size()) {
              for (std::vector<Point>::const_iterator i = tck.begin()+1; i != tck.end(); ++i) write_next_point (*i);
              write_next_point (Point (NAN, NAN, NAN));
            }
            write_next_point (Point (INFINITY, INFINITY, INFINITY));
            off64_t end (out.tellp());
            out.seekp (current);
            write_next_point (tck.size() ? tck[0] : Point (NAN, NAN, NAN));
            out.seekp (end);
            
            count++;
          }
          void close ();

          uint count, total_count;

        protected:
          std::ofstream  out;
          DataType dtype;
          off64_t  count_offset;

          void write_next_point (const Point& p) 
          {
            using namespace ByteOrder;
            float x[3];
            if (dtype == DataType::Float32LE) { x[0] = LE(p[0]); x[1] = LE(p[1]); x[2] = LE(p[2]); }
            else { x[0] = BE(p[0]); x[1] = BE(p[1]); x[2] = BE(p[2]); }
            out.write ((const char*) x, 3*sizeof(float));
          }
      };


    }
  }
}


#endif

