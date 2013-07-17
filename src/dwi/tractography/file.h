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

      template <typename T = float> 
        class Reader : public __ReaderBase__
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




      //! class to handle writing tracks to file
      /*! writes track header as specified in \a properties and individual
       * tracks to the file specified in \a file. Writing individual tracks is
       * done using the append() method.
       *
       * This class implements a large write-back RAM buffer to hold the track
       * data in RAM, and only commits to file when the buffer capacity is
       * reached. This minimises the number of write() calls, which can
       * otherwise become a bottleneck on distributed or network filesystems.
       * It also helps reduce file fragmentation when multiple processes write
       * to file concurrently. The size of the write-back buffer defaults to
       * 16MB, and can be set in the config file using the
       * TrackWriterBufferSize field (in bytes). 
       * */
      template <typename T = float> 
        class Writer : public __WriterBase__ <T>
      {
        public:
          typedef T value_type;
          using __WriterBase__<T>::count;
          using __WriterBase__<T>::total_count;
          using __WriterBase__<T>::out;
          using __WriterBase__<T>::dtype;
          using __WriterBase__<T>::create;

          Writer (const std::string& file, const Properties& properties) :
            buffer_capacity (File::Config::get_int ("TrackWriterBufferSize", 16777216) / sizeof (Point<value_type>)),
            buffer (new Point<value_type> [buffer_capacity+2]),
            buffer_size (0)
          {
            create (file, properties, "tracks");
            barrier_addr = out.tellp();
            write_point (barrier());
          }

          ~Writer() {
            commit();
          }

          void append (const std::vector< Point<value_type> >& tck)
          {
            if (tck.size()) {
              if (buffer_size + tck.size() > buffer_capacity)
                commit();

              for (typename std::vector<Point<value_type> >::const_iterator i = tck.begin(); i != tck.end(); ++i)
                add_point (*i);
              add_point (delimiter());
              ++count;
            }
            ++total_count;
          }

          bool operator() (const std::vector< Point<value_type> >& tck)
          {
            append (tck);
            return true;
          }


        protected:
          const size_t buffer_capacity;

          Ptr<Point<value_type>,true> buffer;
          size_t buffer_size;
          int64_t barrier_addr;

          void add_point (const Point<value_type>& p) 
          {
            format_point (p, buffer[buffer_size++]);
          }

          Point<value_type> delimiter () const { return Point<value_type> (NAN, NAN, NAN); } 
          Point<value_type> barrier () const { return Point<value_type> (INFINITY, INFINITY, INFINITY); }

          void format_point (const Point<value_type>& p, Point<value_type>& destination) 
          {
            using namespace ByteOrder;
            if (dtype.is_little_endian()) { destination[0] = LE(p[0]); destination[1] = LE(p[1]); destination[2] = LE(p[2]); }
            else { destination[0] = BE(p[0]); destination[1] = BE(p[1]); destination[2] = BE(p[2]); }
          }

          void write_point (const Point<value_type>& p) 
          {
            Point<value_type> x;
            format_point (p, x);
            out.write ((char*) &x[0], sizeof (x));
            if (!out.good())
              throw Exception ("error writing track file \"" + this->name + "\": " + strerror (errno));
          }


          void commit () 
          {
            if (buffer_size == 0) 
              return;
            add_point (barrier());
            int64_t prev_barrier_addr = barrier_addr;
            out.write ((char*) &(buffer[1]), sizeof (Point<value_type>)*(buffer_size-1));
            if (!out.good())
              throw Exception ("error writing track file \"" + this->name + "\": " + strerror (errno));
            barrier_addr = int64_t (out.tellp()) - sizeof(Point<value_type>);
            out.seekp (prev_barrier_addr, out.beg);
            out.write ((char*) &(buffer[0]), sizeof(Point<value_type>));
            out.seekp (0, out.end);
            if (!out.good())
              throw Exception ("error writing track file \"" + this->name + "\": " + strerror (errno));
            buffer_size = 0;
          }

          Writer (const Writer& W) : buffer_size (0), barrier_addr (0) { assert (0); }

      };

    }
  }
}


#endif

