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

          bool operator() (std::vector<value_type>& tck_scalar)
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



      //! class to handle writing tracks to file
      /*! writes track scalar file header as specified in \a properties and individual
       * track scalars to the file specified in \a file. Writing individual scalars is
       * done using the append() method.
       *
       * This class implements a large write-back RAM buffer to hold the track scalar
       * data in RAM, and only commits to file when the buffer capacity is
       * reached. This minimises the number of write() calls, which can
       * otherwise become a bottleneck on distributed or network filesystems.
       * It also helps reduce file fragmentation when multiple processes write
       * to file concurrently. The size of the write-back buffer defaults to
       * 16MB, and can be set in the config file using the
       * TrackWriterBufferSize field (in bytes).
       * */
      template <typename T = float>
      class ScalarWriter : public __WriterBase__<T>
      {
        public:
          typedef T value_type;
          using __WriterBase__<T>::count;
          using __WriterBase__<T>::count_offset;
          using __WriterBase__<T>::total_count;
          using __WriterBase__<T>::name;
          using __WriterBase__<T>::dtype;
          using __WriterBase__<T>::create;
          using __WriterBase__<T>::update_counts;
          using __WriterBase__<T>::verify_stream;

          ScalarWriter (const std::string& file, const Properties& properties) :
            __WriterBase__<T> (file),
            buffer_capacity (File::Config::get_int ("TrackWriterBufferSize", 16777216) / sizeof (value_type)),
            buffer (new value_type [buffer_capacity+1]),
            buffer_size (0)
          {
            std::ofstream out (name.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
            if (!out)
              throw Exception ("error creating track scalars file \"" + name + "\": " + strerror (errno));

            create (out, properties, "track scalars");
            current_offset = out.tellp();
          }

          ~ScalarWriter() {
            commit();
          }


          bool operator() (const std::vector<value_type>& tck_scalar)
          {
            if (tck_scalar.size()) {
              if (buffer_size + tck_scalar.size() > buffer_capacity)
                commit();

              for (typename std::vector<value_type>::const_iterator i = tck_scalar.begin(); i != tck_scalar.end(); ++i)
                add_scalar (*i);
              add_scalar (delimiter());
              ++count;
            }
            ++total_count;
            return true;
          }



        protected:

          const size_t buffer_capacity;
          Ptr<value_type, true> buffer;
          size_t buffer_size;
          int64_t current_offset;

          void add_scalar (const value_type& s) {
            format_scalar (s, buffer[buffer_size++]);
          }

          value_type delimiter () const { return value_type (NAN); }

          void format_scalar (const value_type& s, value_type& destination)
          {
            using namespace ByteOrder;
            if (dtype.is_little_endian()) 
              destination = LE(s); 
            else  
              destination = BE(s); 
          }

          void commit ()
          {
            if (buffer_size == 0)
              return;

            std::ofstream out (name.c_str(), std::ios::in | std::ios::out | std::ios::binary | std::ios::ate);
            if (!out)
              throw Exception ("error re-opening track scalars file \"" + name + "\": " + strerror (errno));

            out.seekp (current_offset, out.beg);
            out.write (reinterpret_cast<char*> (&(buffer[0])), sizeof (value_type)*(buffer_size));
            current_offset = int64_t (out.tellp()) - sizeof(value_type);
            verify_stream (out);
            update_counts (out);
            verify_stream (out);
            buffer_size = 0;
          }


          ScalarWriter (const ScalarWriter& W) : buffer_size (0) { assert (0); }

      };


    }
  }
}


#endif

