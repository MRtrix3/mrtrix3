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

#include "file/path.h"
#include "dwi/tractography/file.h"


namespace MR {
  namespace DWI {
    namespace Tractography {

      void Reader::open (const std::string& file, Properties& properties)
      {
        properties.clear();
        dtype = DataType::Undefined;

        File::KeyValue kv (file, "mrtrix tracks");
        std::string data_file;

        while (kv.next()) {
          std::string key = lowercase (kv.key());
          if (key == "roi") {
            try {
              std::vector<std::string> V (split (kv.value(), " \t", true, 2));

              V[0] = lowercase (V[0]);
              if (V[0] == "seed") properties.seed.add (V[1]);
              else if (V[0] == "include") properties.include.add (V[1]);
              else if (V[0] == "exclude") properties.exclude.add (V[1]);
              else if (V[0] == "mask") properties.mask.add (V[1]);
              else throw 1;
            }
            catch (...) {
              error ("WARNING: invalid ROI specification in tracks file \"" + file + "\" - ignored");
            }
          }
          else if (key == "comment") properties.comments.push_back (kv.value());
          else if (key == "file") data_file = kv.value();
          else if (key == "datatype") dtype.parse (kv.value()); 
          else properties[key] = kv.value();
        }

        if (dtype == DataType::Undefined) throw Exception ("no datatype specified for tracks file \"" + file + "\"");
        if (dtype != DataType::Float32LE && dtype != DataType::Float32BE)
          throw Exception ("only supported datatype for tracks file are Float32LE or Float32BE (in tracks file \"" + file + "\")");

        if (data_file.empty()) throw Exception ("missing \"files\" specification for tracks file \"" + file + "\"");

        std::istringstream files_stream (data_file);
        std::string fname;
        files_stream >> fname;
        off64_t offset = 0;
        if (files_stream.good()) {
          try { files_stream >> offset; }
          catch (...) { throw Exception ("invalid offset specified for file \"" + fname + "\" in tracks file \"" + file + "\""); }
        }

        if (fname != ".") fname = Path::join (Path::dirname (file), fname);
        else fname = file;

        in.open (fname.c_str(), std::ios::in | std::ios::binary);
        if (!in) throw Exception ("error opening tracks data file \"" + fname + "\": " + strerror(errno));
        in.seekg (offset);
      }





      bool Reader::next (std::vector<Point>& tck)
      {
        tck.clear();

        if (!in.is_open()) return (false);
        do {
          Point p = get_next_point();
          if (isinf (p[0])) {
            in.close();
            return (false);
          }
          if (in.eof()) {
            in.close();
            return (true);
          }

          if (isnan (p[0])) return (true);
          tck.push_back (p);
        } while (in.good());

        in.close();
        return (false);
      }












      void Writer::create (const std::string& file, const Properties& properties)
      {
        out.open (file.c_str(), std::ios::out | std::ios::binary);
        if (!out) throw Exception ("error creating tracks file \"" + file + "\": " + strerror (errno));

        out << "mrtrix tracks\nEND\n";
        for (Properties::const_iterator i = properties.begin(); i != properties.end(); ++i) 
          out << i->first << ": " << i->second << "\n";

        for (std::vector<std::string>::const_iterator i = properties.comments.begin(); i != properties.comments.end(); ++i)
          out << "comment: " << *i << "\n";

        for (size_t n = 0; n < properties.seed.size(); ++n) out << "roi: seed " << properties.seed[n].parameters() << "\n";
        for (size_t n = 0; n < properties.include.size(); ++n) out << "roi: include " << properties.include[n].parameters() << "\n";
        for (size_t n = 0; n < properties.exclude.size(); ++n) out << "roi: exclude " << properties.exclude[n].parameters() << "\n";
        for (size_t n = 0; n < properties.mask.size(); ++n) out << "roi: mask " << properties.mask[n].parameters() << "\n";

        out << "datatype: " << dtype.specifier() << "\n";
        off64_t data_offset = off64_t(out.tellp()) + 65;
        out << "file: . " << data_offset << "\n";
        out << "count: ";
        count_offset = out.tellp();
        out << "\nEND\n";
        out.seekp (0);
        out << "mrtrix tracks    ";
        out.seekp (data_offset);
        write_next_point (Point (INFINITY, INFINITY, INFINITY));
      }




      void Writer::close ()
      {
        out.seekp (count_offset);
        out << count << "\ntotal_count: " << total_count << "\nEND\n";
        out.close();
      }






    }
  }
}


