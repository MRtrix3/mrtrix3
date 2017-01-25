/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#ifndef __dwi_tractography_file_base_h__
#define __dwi_tractography_file_base_h__

#include <iomanip>
#include <map>

#include "types.h"
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


      template <typename ValueType = float>
        class __WriterBase__
        {
          public:
            typedef ValueType value_type;

            __WriterBase__(const std::string& name) :
              count (0),
              total_count (0),
              name (name), 
              dtype (DataType::from<ValueType>()),
              count_offset (0),
              open_success (false)
          {
            dtype.set_byte_order_native();
            if (dtype != DataType::Float32LE && dtype != DataType::Float32BE &&
                dtype != DataType::Float64LE && dtype != DataType::Float64BE)
              throw Exception ("only supported datatype for tracks file are "
                  "Float32LE, Float32BE, Float64LE & Float64BE");
            App::check_overwrite (name);
          }

            ~__WriterBase__()
            {
              if (open_success) {
                File::OFStream out (name, std::ios::in | std::ios::out | std::ios::binary);
                update_counts (out);
              }
            }

            void create (File::OFStream& out, const Properties& properties, const std::string& type) {
              out << "mrtrix " + type + "\nEND\n";

              for (const auto& i : properties) {
                if ((i.first != "count") && (i.first != "total_count"))
                  out << i.first << ": " << i.second << "\n";
              }

              for (const auto& i : properties.comments) 
                out << "comment: " << i << "\n";

              for (size_t n = 0; n < properties.seeds.num_seeds(); ++n)
                out << "roi: seed " << properties.seeds[n]->get_name() << "\n";
              for (size_t n = 0; n < properties.include.size(); ++n)
                out << "roi: include " << properties.include[n].parameters() << "\n";
              for (size_t n = 0; n < properties.exclude.size(); ++n)
                out << "roi: exclude " << properties.exclude[n].parameters() << "\n";
              for (size_t n = 0; n < properties.mask.size(); ++n)
                out << "roi: mask " << properties.mask[n].parameters() << "\n";

              for (const auto& it : properties.roi)
                out << "roi: " << it.first << " " << it.second << "\n";

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


            uint64_t count, total_count;


          protected:
            std::string name;
            DataType dtype;
            int64_t count_offset;
            bool open_success;


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

