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
#include <vector>

#include "app.h"
#include "types.h"
#include "point.h"
#include "file/key_value.h"
#include "file/path.h"
#include "file/utils.h"
#include "dwi/tractography/file_base.h"
#include "dwi/tractography/properties.h"
#include "math/vector.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {


      template <typename T = float>
      class TrackData : public std::vector< Point<T> >
      {
        public:
          typedef T value_type;
          TrackData () :
              std::vector< Point<value_type> > (),
              index (-1),
              weight (value_type (1.0)) { }
          TrackData (const std::vector< Point<value_type> >& tck) :
              std::vector< Point<value_type> > (tck),
              index (-1),
              weight (1.0) { }
          size_t index;
          value_type weight;
      };


      template <typename T = float> 
        class Reader : public __ReaderBase__
      {
        public:
          typedef T value_type;

          Reader (const std::string& file, Properties& properties) :
              track_index (0)
          {
            open (file, "tracks", properties);
            App::Options opt = App::get_options ("tck_weights_in");
            if (opt.size()) {
              const std::string path = opt[0][0];
              weights.load (path);
              DEBUG ("Imported track weights file " + path + " with " + str(weights.size()) + " elements");
            }
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

              if (isnan (p[0])) {
                ++track_index;
                return true;
              }
              tck.push_back (p);
            } while (in.good());

            in.close();
            return false;
          }
          bool operator() (std::vector< Point<value_type> >& tck) { return next (tck); }


          bool next_data (TrackData<value_type>& tck)
          {
            tck.clear();
            tck.index = track_index;
            if (weights.size()) {
              if (tck.index >= weights.size())
                return false;
              tck.weight = weights[tck.index];
            } else {
              tck.weight = 1.0;
            }
            return next (tck);
          }
          bool operator() (TrackData<value_type>& tck) { return next_data (tck); }



        protected:
          using __ReaderBase__::in;
          using __ReaderBase__::dtype;

          Math::Vector<value_type> weights;
          size_t track_index;

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
          using __WriterBase__<T>::name;
          using __WriterBase__<T>::dtype;
          using __WriterBase__<T>::create;

          Writer (const std::string& file, const Properties& properties) :
            __WriterBase__<T> (file), 
            buffer_capacity (File::Config::get_int ("TrackWriterBufferSize", 16777216) / sizeof (Point<value_type>)),
            buffer (new Point<value_type> [buffer_capacity+2]),
            buffer_size (0)
          {
            std::ofstream out (name.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
            if (!out)
              throw Exception ("error creating tracks file \"" + name + "\": " + strerror (errno));

            create (out, properties, "tracks");
            barrier_addr = out.tellp();

            Point<value_type> x;
            format_point (barrier(), x);
            out.write (reinterpret_cast<char*> (&x[0]), sizeof (x));
            if (!out.good())
              throw Exception ("error writing tracks file \"" + name + "\": " + strerror (errno));
          
            App::Options opt = App::get_options ("tck_weights_out");
            if (opt.size())
              set_weights_path (opt[0][0]);
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
          bool operator() (const std::vector< Point<value_type> >& tck) { 
            append (tck); 
            return true;
          }

          void append (const TrackData<value_type>& tck)
          {
            append (std::vector< Point<value_type> > (tck));
            if (weights_name.size() && tck.size())
              weights_buffer += str (tck.weight) + ' ';
          }
          bool operator() (const TrackData<value_type>& tck) { 
            append (tck); 
            return true;
          }

          void set_weights_path (const std::string& path) {
            if (weights_name.size())
              throw Exception ("Cannot change output streamline weights file path");
            weights_name = path;
            if (Path::exists (weights_name))
              File::unlink (weights_name);
          }


        protected:
          const size_t buffer_capacity;

          Ptr<Point<value_type>,true> buffer;
          size_t buffer_size;
          int64_t barrier_addr;

          std::string weights_name;
          std::string weights_buffer;

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


          void commit () 
          {
            if (buffer_size == 0) 
              return;
            add_point (barrier());
            int64_t prev_barrier_addr = barrier_addr;

            std::ofstream out (name.c_str(), std::ios::in | std::ios::out | std::ios::binary | std::ios::ate);
            if (!out)
              throw Exception ("error re-opening tracks file \"" + name + "\": " + strerror (errno));

            out.write (reinterpret_cast<char*> (&(buffer[1])), sizeof (Point<value_type>)*(buffer_size-1));
            if (!out.good())
              throw Exception ("error writing tracks file \"" + name + "\": " + strerror (errno));
            barrier_addr = int64_t (out.tellp());
            out.seekp (prev_barrier_addr, out.beg);
            out.write (reinterpret_cast<char*> (&(buffer[0])), sizeof(Point<value_type>));
            barrier_addr -= sizeof(Point<value_type>);
            if (!out.good())
              throw Exception ("error writing tracks file \"" + name + "\": " + strerror (errno));
            buffer_size = 0;

            if (weights_name.size()) {
              std::ofstream out_weights (weights_name.c_str(), std::ios::in | std::ios::out | std::ios::binary | std::ios::ate);
              if (!out_weights)
                throw Exception ("error re-opening streamline weights file \"" + weights_name + "\": " + strerror (errno));
              out_weights << weights_buffer;
              weights_buffer.clear();
              if (!out_weights.good())
                throw Exception (std::string ("error writing streamline weights file: ") + strerror (errno));
            }
          }

          Writer (const Writer& W) : buffer_size (0), barrier_addr (0) { assert (0); }

      };

    }
  }
}


#endif

