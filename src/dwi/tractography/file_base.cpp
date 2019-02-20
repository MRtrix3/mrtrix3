/* Copyright (c) 2008-2019 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#include "dwi/tractography/file_base.h"
#include "file/path.h"

namespace MR {
  namespace DWI {
    namespace Tractography {


      void __ReaderBase__::open (const std::string& file, const std::string& type, Properties& prop)
      {
        TrackFileInfo info( file, type, prop );
        dtype = info.dtype;

        in.open (info.path.c_str(), std::ios::in | std::ios::binary);
        if (!in)
          throw Exception ("error opening " + type  + " data file \"" + info.path + "\": " + strerror(errno));
        in.seekg (info.offset);
      }




      void TrackFileInfo::check_dtype() 
      {
        if (dtype == DataType::Undefined)
          throw Exception ("no datatype specified for tracks file \"" + path + "\"");
        if (dtype != DataType::Float32LE && dtype != DataType::Float32BE &&
            dtype != DataType::Float64LE && dtype != DataType::Float64BE)
          throw Exception ("only supported datatype for tracks file are "
              "Float32LE, Float32BE, Float64LE & Float64BE (in " + type  + " file \"" + path + "\")");
      }




      void TrackFileInfo::parse( const std::string& file, const std::string& file_type, Properties& prop ) 
      {
        clear();
        prop.clear();
        type = file_type;

        // initialise KeyValue instance to parse metadata
        const std::string magic ("mrtrix " + type);
        File::KeyValue kv (file, magic.c_str());

        // parse key/value pairs
        while (kv.next()) {
          const std::string key = lowercase (kv.key());
          if (key == "roi" || key == "prior_roi") {
            try {
              vector<std::string> V (split (kv.value(), " \t", true, 2));
              prop.prior_rois.insert (std::pair<std::string,std::string> (V[0], V[1]));
            }
            catch (...) {
              WARN ("invalid ROI specification in " + type  + " file \"" + file + "\" - ignored");
            }
          }
          else if (key == "comment")    prop.comments.push_back (kv.value());
          else if (key == "file")       path = kv.value();
          else if (key == "datatype")   dtype = DataType::parse (kv.value());
          else                          prop[kv.key()] = kv.value();
        }

        // check field "datatype"
        check_dtype();

        // field "file" should be defined
        if (path.empty())
          throw Exception ("missing \"file\" specification for " + type  + " file \"" + file + "\"");

        // its value should be: "filename offset" (no spaces in filename!)
        std::istringstream file_val (path);
        std::string path_val;
        file_val >> path_val;

        if (file_val.good()) {
          try { file_val >> offset; }
          catch (...) {
            throw Exception ("invalid offset specified for file \""
                + path_val + "\" in " + type  + " file \"" + file + "\"");
          }
        }

        // value "." interpreted as "this file"
        if (path_val != ".")
          path = Path::join (Path::dirname (file), path_val);
        else
          path = file;
      }



      void properties_consensus( const std::vector<std::string>& files, const std::string& type, Properties& prop )
      {
        // ensure there is at least one file
        size_t n_files = files.size();
        if (n_files == 0)
          throw Exception("empty list of files for consensus!");

        // read first file
        TrackFileInfo info_ref( files[0], type, prop );

        // create comments buffer and summary counts
        std::unordered_set<std::string> comments_buffer( prop.comments.begin(), prop.comments.end() );
        uint64_t count = prop.value_or_default<uint64_t>("count",0);
        uint64_t total_count = prop.value_or_default<uint64_t>("total_count",0);

        // iterate over remaining files
        TrackFileInfo info_tmp;
        Properties prop_tmp;
        for ( size_t k=1; k < n_files; ++k ) {

          info_tmp.parse( files[k], type, prop_tmp );
          if ( info_tmp.dtype != info_ref.dtype )
            throw Exception("datatype mismatch between " + type  + " files \"" + files[0] + "\" and \"" + files[k] + "\"");

          for (auto& c: prop_tmp.comments)
            if ( comments_buffer.find(c) == comments_buffer.end() ) {
              prop.comments.push_back(c);
              comments_buffer.insert(c);
            }

          for (auto& p: prop_tmp)
            if (p.first == "count")
              count += to<uint64_t>(p.second);
            else if (p.first == "total_count")
              total_count += to<uint64_t>(p.second);
            else {
              auto existing = prop.find (p.first);
              if (existing == prop.end())
                prop.insert(p);
              else if (p.second != existing->second) {
                INFO("variable property \"" + p.first + "\" across " + type + " files");
                existing->second = "variable";
              }
            }
        }

        // final counts
        prop["count"] = str(count);
        prop["total_count"] = str(total_count);
      }

    }
  }
}


