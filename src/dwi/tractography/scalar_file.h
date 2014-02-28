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




      //! functions for comparing tsf files
      /*! in order to be interpreted correctly, track scalar files must match some
       * corresponding streamline data (.tck) file; this is handled using the timestamp
       * field in the Properties class. Alternatively two .tsf files may be processed,
       * but these must both correspond to the same .tck file (even if that file is
       * not explicitly read).
       *
       * Furthermore, in some contexts it may be necessary to ensure that two files
       * contain the same number of streamlines (or scalar data corresponding to the
       * same number of streamlines). This check is also provided; in the "validate_*"
       * functions a mis-match of the 'count' field results in an exception being
       * thrown, whereas in the "check_*" functibothons, only a warning is thrown, and
       * processing is free to continue.
       *
       * Note that the only difference between those functions comparing a .tck file
       * to a .tsf file, and those comparing a pair of .tsf files, is the information
       * provided to the user if the comparison fails.
       * */
      void validate_tck_tsf_pair (const Properties& p_tck, const Properties& p_tsf)
      {
        if (p_tsf.timestamp != p_tck.timestamp)
          throw Exception ("input scalar file does not correspond to the input track file");
        Properties::const_iterator count_tck = p_tck.find ("count");
        Properties::const_iterator count_tsf = p_tsf.find ("count");
        if ((count_tck == p_tck.end()) || (count_tsf == p_tsf.end()))
          throw Exception ("Unable to validate .tck / .tsf file pair due to missing count field");
        if (to<size_t>(count_tsf->second) != to<size_t>(count_tck->second))
          throw Exception ("input scalar file does not contain same number of elements as input track file");
      }

      void check_tck_tsf_pair (const Properties& p_tck, const Properties& p_tsf)
      {
        if (p_tsf.timestamp != p_tck.timestamp)
          throw Exception ("input scalar file does not correspond to the input track file");
        Properties::const_iterator count_tck = p_tck.find ("count");
        Properties::const_iterator count_tsf = p_tsf.find ("count");
        if ((count_tck == p_tck.end()) || (count_tsf == p_tsf.end())) {
          WARN ("Missing count field in .tck / .tsf file pair; unable to compare");
        } else if (to<size_t>(count_tsf->second) != to<size_t>(count_tck->second)) {
          WARN ("input scalar file does not contain same number of elements as input track file");
        }
      }

      void validate_tsf_pair (const Properties& p_one, const Properties& p_two)
      {
        if (p_one.timestamp != p_two.timestamp)
          throw Exception ("input scalar files do not correspond to the same track file");
        Properties::const_iterator count_one = p_one.find ("count");
        Properties::const_iterator count_two = p_two.find ("count");
        if ((count_one == p_one.end()) || (count_two == p_two.end()))
          throw Exception ("Unable to validate track scalar file pair due to missing count field");
        if (to<size_t>(count_one->second) != to<size_t>(count_two->second))
          throw Exception ("input scalar files do not contain the same number of elements");
      }

      void check_tsf_pair (const Properties& p_one, const Properties& p_two)
      {
        if (p_one.timestamp != p_two.timestamp)
          throw Exception ("input scalar files do not correspond to the same track file");
        Properties::const_iterator count_one = p_one.find ("count");
        Properties::const_iterator count_two = p_two.find ("count");
        if ((count_one == p_one.end()) || (count_two == p_two.end())) {
          WARN ("Unable to validate track scalar file pair due to missing count field");
        } else if (to<size_t>(count_one->second) != to<size_t>(count_two->second)) {
          WARN ("input scalar files do not contain the same number of elements");
        }
      }






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

            // Do NOT set Properties timestamp here! (Must match corresponding .tck file)

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
            current_offset = int64_t (out.tellp());
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

