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

#ifndef __dwi_tractography_file_base_h__
#define __dwi_tractography_file_base_h__

#include <iomanip>
#include <map>

#include "types.h"
#include "point.h"
#include "file/key_value.h"
#include "file/ofstream.h"
#include "file/path.h"
#include "dwi/tractography/properties.h"




namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {

      //! \cond skip
      class __ReaderBase__
      {
        public:
          ~__ReaderBase__ () {
            if (in.is_open())
              in.close();
          }

          void open (const std::string& file, const std::string& firstline, Properties& properties);

          void close () { in.close(); }

        protected:

          std::ifstream  in;
          DataType  dtype;
      };


      template <typename T = float> class __WriterBase__
      {
        public:
          typedef T value_type;

          __WriterBase__(const std::string& name) :
            count (0),
            total_count (0),
            name (name), 
            dtype (DataType::from<value_type>()),
            count_offset (0)
          {
            dtype.set_byte_order_native();
            if (dtype != DataType::Float32LE && dtype != DataType::Float32BE &&
                dtype != DataType::Float64LE && dtype != DataType::Float64BE)
                throw Exception ("only supported datatype for tracks file are "
                    "Float32LE, Float32BE, Float64LE & Float64BE");
            if (!App::overwrite_files && Path::exists (name))
              throw Exception ("error creating file \"" + name + "\": file exists (use -force option to force overwrite)");
          }

          ~__WriterBase__()
          {
            File::OFStream out (name, std::ios::in | std::ios::out | std::ios::binary);
            update_counts (out);
          }

          void create (File::OFStream& out, const Properties& properties, const std::string& type) {
            out << "mrtrix " + type + "\nEND\n";

            for (Properties::const_iterator i = properties.begin(); i != properties.end(); ++i) {
              if ((i->first != "count") && (i->first != "total_count"))
                out << i->first << ": " << i->second << "\n";
            }

            for (std::vector<std::string>::const_iterator i = properties.comments.begin();
                i != properties.comments.end(); ++i)
              out << "comment: " << *i << "\n";

            for (size_t n = 0; n < properties.seeds.num_seeds(); ++n)
              out << "roi: seed " << properties.seeds[n]->get_name() << "\n";
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
            data_offset += (4 - (data_offset % 4)) % 4;
            out << "file: . " << data_offset << "\n";
            out << "count: ";
            count_offset = out.tellp();
            out << "0\nEND\n";
            out.seekp (0);
            out << "mrtrix " + type + "    ";
            out.seekp (data_offset);
          }


          size_t count, total_count;


        protected:
          std::string name;
          DataType dtype;
          int64_t  count_offset;


          void verify_stream (const File::OFStream& out) {
            if (!out.good())
              throw Exception ("error writing file \"" + name + "\": " + strerror (errno));
          }

          void update_counts (File::OFStream& out) {
            out.seekp (count_offset);
            out << count << "\ntotal_count: " << total_count << "\nEND\n";
            verify_stream (out);
          }
      };
      //! \endcond


    }
  }
}


#endif

