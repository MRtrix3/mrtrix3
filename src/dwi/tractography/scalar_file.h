/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */

#ifndef __dwi_tractography_scalar_file_h__
#define __dwi_tractography_scalar_file_h__

#include <map>

#include "types.h"
#include "file/config.h"
#include "file/key_value.h"
#include "file/ofstream.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/file_base.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {




      //! convenience function to verify that tck/tsf files match
      /*! in order to be interpreted correctly, track scalar files must match some
       * corresponding streamline data (.tck) file; this is handled using the timestamp
       * field in the Properties class. Alternatively two .tsf files may be processed,
       * but these must both correspond to the same .tck file (even if that file is
       * not explicitly read).
       *
       * Furthermore, in some contexts it may be necessary to ensure that two
       * files contain the same number of streamlines (or scalar data
       * corresponding to the same number of streamlines). This check is also
       * provided: if \a abort_on_fail is true, a mis-match of the 'count'
       * field results in an exception being thrown, otherwise only a warning
       * is issued and processing is free to continue.
       *
       * The \a type argument is used to specify the type of files being
       * compared, so that more appropriate information can be shown to the
       * user in the event of a mismatch.  */
      inline void check_properties_match (const Properties& p_tck, const Properties& p_tsf, const std::string& type, bool abort_on_fail = true)
      {
        check_timestamps (p_tck, p_tsf, type);
        check_counts (p_tck, p_tsf, type, abort_on_fail);
      }






      template <typename T = float> class ScalarReader : public __ReaderBase__
      { NOMEMALIGN
        public:
          typedef T value_type;

          ScalarReader (const std::string& file, Properties& properties) {
            open (file, "track scalars", properties);
          }

          bool operator() (vector<value_type>& tck_scalar)
          {
            tck_scalar.clear();

            if (!in.is_open())
              return false;
            do {
              value_type val = get_next_scalar();
              if (std::isinf (val)) {
                in.close();
                return false;
              }
              if (in.eof()) {
                in.close();
                return false;
              }

              if (std::isnan (val))
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

          ScalarReader (const ScalarReader&) = delete;

      };



      //! class to handle writing track scalars to file
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
      { NOMEMALIGN
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
          using __WriterBase__<T>::open_success;

          ScalarWriter (const std::string& file, const Properties& properties) :
            __WriterBase__<T> (file),
            buffer_capacity (File::Config::get_int ("TrackWriterBufferSize", 16777216) / sizeof (value_type)),
            buffer (new value_type [buffer_capacity+1]),
            buffer_size (0)
          {
            File::OFStream out;
            try {
              out.open (name, std::ios::out | std::ios::binary | std::ios::trunc);
            } catch (Exception& e) {
              throw Exception (e, "Unable to create output track scalar file");
            }

            // Do NOT set Properties timestamp here! (Must match corresponding .tck file)
            const_cast<Properties&> (properties).set_version_info();
            create (out, properties, "track scalars");
            open_success = true;
            current_offset = out.tellp();
          }

          ~ScalarWriter() {
            commit();
          }


          bool operator() (const vector<value_type>& tck_scalar)
          {
            if (tck_scalar.size()) {
              if (buffer_size + tck_scalar.size() > buffer_capacity)
                commit();

              for (typename vector<value_type>::const_iterator i = tck_scalar.begin(); i != tck_scalar.end(); ++i)
                add_scalar (*i);
              add_scalar (delimiter());
              ++count;
            }
            ++total_count;
            return true;
          }


          template <typename matrix_type>
          bool operator() (const Eigen::Matrix<matrix_type, Eigen::Dynamic, 1>& data)
          {
            if (data.size()) {
              if (buffer_size + data.size() > buffer_capacity)
                commit();

              for (int i = 0; i != data.size(); ++i)
                add_scalar (value_type(data[i]));
              add_scalar (delimiter());
              ++count;
            }
            ++total_count;
            return true;
          }



        protected:

          const size_t buffer_capacity;
          std::unique_ptr<value_type[]> buffer;
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
            if (buffer_size == 0 || !open_success)
              return;
            File::OFStream out (name, std::ios::in | std::ios::out | std::ios::binary | std::ios::ate);
            out.seekp (current_offset, out.beg);
            out.write (reinterpret_cast<char*> (buffer.get()), sizeof(value_type)*buffer_size);
            current_offset = int64_t (out.tellp());
            verify_stream (out);
            update_counts (out);
            verify_stream (out);
            buffer_size = 0;
          }


          ScalarWriter (const ScalarWriter&) = delete;

      };


    }
  }
}


#endif

