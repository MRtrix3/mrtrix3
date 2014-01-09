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
        class Streamline : public std::vector< Point<T> >
      {
        public:
          typedef T value_type;
          Streamline () : weight (value_type (1.0)) { }

          Streamline (size_t size) : 
            std::vector< Point<value_type> > (size), 
            weight (value_type (1.0)) { }

          Streamline (const std::vector< Point<value_type> >& tck) :
            std::vector< Point<value_type> > (tck),
            weight (1.0) { }

          value_type weight;
      };


      template <typename T = float> 
        class Reader : public __ReaderBase__
      {
        public:
          typedef T value_type;

          Reader (const std::string& file, Properties& properties) :
            current_index (0) {
              open (file, "tracks", properties);
              App::Options opt = App::get_options ("tck_weights_in");
              if (opt.size()) {
                weights.load (opt[0][0]);
                if (weights.size() != to<size_t> (properties["count"])) // TODO: turn this into Exception once count write-out at commit-time works.
                  WARN ("number of weights does not match number of tracks in file");
                DEBUG ("loaded " + str(weights.size()) + " track weights from file \"" + std::string (opt[0][0]) + "\"");
              }
            }


          //! fetch next track from file
          bool operator() (Streamline<value_type>& tck) {
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
                if (weights.size()) {
                  if (current_index >= weights.size())
                    return false;
                  tck.weight = weights[current_index++];
                } 
                else 
                  tck.weight = 1.0;

                return true;
              }

              tck.push_back (p);
            } while (in.good());

            in.close();
            return false;
          }



        protected:
          using __ReaderBase__::in;
          using __ReaderBase__::dtype;

          size_t current_index;
          Math::Vector<value_type> weights;

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







      //! class to handle unbuffered writing of tracks to file
      /*! writes track header as specified in \a properties and individual
       * tracks to the file specified in \a file. Writing individual tracks is
       * done using the operator() method.
       *
       * This class implements unbuffered write to file, which is useful when a
       * large number of Writers are used concurrently, so that the memory
       * footprint of the buffered version would become prohibitive. 
       * */
      template <typename T = float> 
        class UnbufferedWriter : public __WriterBase__ <T>
      {
        public:
          typedef T value_type;
          using __WriterBase__<T>::count;
          using __WriterBase__<T>::total_count;
          using __WriterBase__<T>::name;
          using __WriterBase__<T>::dtype;
          using __WriterBase__<T>::create;

          UnbufferedWriter (const std::string& file, const Properties& properties) :
            __WriterBase__<T> (file) {
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

          //! append track to file
          bool operator() (const Streamline<value_type>& tck) {
            if (tck.size()) {
              Point<value_type> buffer [tck.size()+2];
              for (size_t n = 0; n < tck.size(); ++n)
                format_point (barrier(), buffer[n]); 
              format_point (delimiter(), buffer[tck.size()]);
              format_point (barrier(), buffer[tck.size()+1]);
              int64_t prev_barrier_addr = barrier_addr;

              std::ofstream out (name.c_str(), std::ios::in | std::ios::out | std::ios::binary | std::ios::ate);
              if (!out)
                throw Exception ("error re-opening tracks file \"" + name + "\": " + strerror (errno));

              out.write (reinterpret_cast<char*> (&(buffer[1])), sizeof(Point<value_type>) * (tck.size()+1));
              if (!out.good())
                throw Exception ("error writing tracks file \"" + name + "\": " + strerror (errno));
              barrier_addr = int64_t (out.tellp()) - sizeof(Point<value_type>);
              out.seekp (prev_barrier_addr, out.beg);
              out.write (reinterpret_cast<char*> (&(buffer[0])), sizeof(Point<value_type>));
              if (!out.good())
                throw Exception ("error writing tracks file \"" + name + "\": " + strerror (errno));

              if (weights_name.size()) {
                std::ofstream out_weights (weights_name.c_str(), std::ios::in | std::ios::out | std::ios::binary | std::ios::ate);
                if (!out_weights)
                  throw Exception ("error re-opening streamline weights file \"" + weights_name + "\": " + strerror (errno));
                out_weights << tck.weight << " ";
                if (!out_weights.good())
                  throw Exception (std::string ("error writing streamline weights file: ") + strerror (errno));
              }

              ++count;
            }
            ++total_count;
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
          int64_t barrier_addr;

          std::string weights_name;

          Point<value_type> delimiter () const { return Point<value_type> (NAN, NAN, NAN); } 
          Point<value_type> barrier () const { return Point<value_type> (INFINITY, INFINITY, INFINITY); }

          void format_point (const Point<value_type>& p, Point<value_type>& destination) 
          {
            using namespace ByteOrder;
            if (dtype.is_little_endian()) { destination[0] = LE(p[0]); destination[1] = LE(p[1]); destination[2] = LE(p[2]); }
            else { destination[0] = BE(p[0]); destination[1] = BE(p[1]); destination[2] = BE(p[2]); }
          }

          UnbufferedWriter (const UnbufferedWriter& W) : barrier_addr (0) { assert (0); }
      };








      //! class to handle writing tracks to file
      /*! writes track header as specified in \a properties and individual
       * tracks to the file specified in \a file. Writing individual tracks is
       * done using the operator() method.
       *
       * This class implements a large write-back RAM buffer to hold the track
       * data in RAM, and only commits to file when the buffer capacity is
       * reached. This minimises the number of write() calls, which can
       * otherwise become a bottleneck on distributed or network filesystems.
       * It also helps reduce file fragmentation when multiple processes write
       * to file concurrently. The size of the write-back buffer defaults to
       * 16MB, and can be set in the constructor, or in the config file using
       * the TrackWriterBufferSize field (in bytes). 
       * */
      template <typename T = float> 
        class Writer : public UnbufferedWriter <T>
      {
        public:
          typedef T value_type;
          using __WriterBase__<T>::count;
          using __WriterBase__<T>::total_count;
          using __WriterBase__<T>::name;
          using __WriterBase__<T>::update_counts;
          using UnbufferedWriter<T>::barrier_addr;
          using UnbufferedWriter<T>::format_point;
          using UnbufferedWriter<T>::weights_name;
          using UnbufferedWriter<T>::barrier;
          using UnbufferedWriter<T>::delimiter;

          Writer (const std::string& file, const Properties& properties, size_t default_buffer_capacity = 16777216) :
            UnbufferedWriter<T> (file, properties), 
            buffer_capacity (File::Config::get_int ("TrackWriterBufferSize", default_buffer_capacity) / sizeof (Point<value_type>)),
            buffer (new Point<value_type> [buffer_capacity+2]),
            buffer_size (0) { }

          ~Writer() {
            commit();
          }

          //! append track to file
          bool operator() (const Streamline<value_type>& tck) {
            if (tck.size()) {
              if (buffer_size + tck.size() > buffer_capacity)
                commit();

              for (typename std::vector<Point<value_type> >::const_iterator i = tck.begin(); i != tck.end(); ++i)
                add_point (*i);
              add_point (delimiter());

              if (weights_name.size())
                weights_buffer += str (tck.weight) + ' ';

              ++count;
            }
            ++total_count;
            return true;
          }


        protected:
          const size_t buffer_capacity;

          Ptr<Point<value_type>,true> buffer;
          size_t buffer_size;
          std::string weights_buffer;


          void add_point (const Point<value_type>& p) {
            format_point (p, buffer[buffer_size++]);
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
            barrier_addr = int64_t (out.tellp()) - sizeof(Point<value_type>);
            out.seekp (prev_barrier_addr, out.beg);
            out.write (reinterpret_cast<char*> (&(buffer[0])), sizeof(Point<value_type>));
            update_counts (out);
            if (!out.good())
              throw Exception ("error writing tracks file \"" + name + "\": " + strerror (errno));

            buffer_size = 0;


            if (weights_name.size()) {
              std::ofstream out_weights (weights_name.c_str(), std::ios::in | std::ios::out | std::ios::binary | std::ios::ate);
              if (!out_weights)
                throw Exception ("error re-opening streamline weights file \"" + weights_name + "\": " + strerror (errno));
              out_weights << weights_buffer;
              if (!out_weights.good())
                throw Exception (std::string ("error writing streamline weights file: ") + strerror (errno));
              weights_buffer.clear();
            }

          }

          Writer (const Writer& W) : buffer_size (0) { assert (0); }
    };






    }
  }
}


#endif

