/*
   Copyright 2008 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 04/07/09.

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

#ifndef __file_gz_h__
#define __file_gz_h__

#include <cassert>
#include <cstring>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <zlib.h>

#include "mrtrix.h"
#include "exception.h"
#include "file/confirm.h"
#include "file/path.h"

namespace MR {
  namespace File {
    namespace GZ {

      class Reader {
        public:
          Reader () : gz (NULL), file (NULL) { }
          Reader (const std::string& fname) : gz (NULL), file (NULL) { open (fname); }
          ~Reader () { close(); }

          std::string  name () const  { return (filename); }

          void open (const std::string& fname) { 
            close();
            filename.clear();
            int fd = open64 (fname.c_str(), O_RDONLY);
            if (fd < 0) throw Exception ("error opening file \"" + fname + "\": " + strerror (errno));
            if (Path::has_suffix (fname, ".gz")) {
              gz = gzdopen (fd, "rb");
              if (!gz) throw Exception ("error opening file \"" + fname + "\": insufficient memory");
            }
            else {
              file = fdopen (fd, "rb");
              if (!file) throw Exception ("error opening file \"" + fname + "\": " + strerror (errno));
            }
            filename = fname;
          }

          void close () {
            filename.clear(); 
            try {
              if (gz) if (gzclose (gz)) throw;
              if (file) if (fclose (file)) throw;
              gz = NULL;
              file = NULL;
            }
            catch (...) { throw Exception ("error closing file \"" + filename + "\": " + error()); }
          }

          bool is_open () const { return (gz || file); }
          bool eof () const { assert (gz || file); return (gz ? gzeof (gz) : feof (file)); }
          off64_t tell () const { assert (gz || file); return (gz ? gztell (gz) : ftell (file)); }

          void seek (off64_t offset) { 
            assert (gz || file);
            if (gz) {
              z_off_t pos = gzseek (gz, offset, SEEK_SET); 
              if (pos >= 0) return;
            }
            else {
              off64_t pos = fseek64 (file, offset, SEEK_SET); 
              if (pos >= 0) return;
            }
            throw Exception ("error seeking in file \"" + filename + "\": " + error()); 
          }

          std::string getline () {
            assert (gz || file);
            std::string string;
            char buf[64];
            do {
              char* status = ( gz ? gzgets (gz, buf, 64) : fgets (file, buf, 64) );
              if (!status) {
                if (eof()) break;
                throw Exception ("error reading from file \"" + filename + "\": " + error());
              }
              string += buf;
            } while (strlen(buf) >= 63);

            if (string.size() > 0) if (string[string.size()-1] == 015) string.resize (string.size()-1);
            return (string);
          }

          int read (char* s, size_t n) {
            assert (gz || file);
            int n_read = gzread (gz, s, n); 
            if (n_read < 0) throw Exception ("error reading from file \"" + filename + "\": " + error());
            return (n_read);
          }

        protected:
          gzFile       gz;
          FILE*        file;
          std::string  filename; 

          const char*  error () {
            int error_number;
            const char* s = gzerror (gz, &error_number);
            if (error_number == Z_ERRNO) s = strerror (errno);
            return (s);
          }
      };










      class Writer : public Reader {
        public:
          Writer () { }
          Writer (const std::string& fname) { open (fname); }

          void open (const std::string& fname) {
            close();
            filename.clear();
            int fd = open64 (fname.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0755);
            if (fd < 0) {
              if (errno == EEXIST) {
                if (confirm ("File \"" + fname + "\" exists. Overwrite?")) {
                  fd = open64 (fname.c_str(), O_WRONLY | O_TRUNC, 0755);
                  if (fd < 0) throw Exception ("error opening file \"" + fname + "\": " + strerror(errno));
                }
                else throw Exception ("operation aborted by user");
              }
              else throw Exception ("error opening file \"" + fname + "\": " + strerror(errno));
            }
            gz = gzdopen (fd, "wb");
            if (!gz) throw Exception ("error opening file \"" + fname + "\": " + error());
            filename = fname;
          }

          void write (const std::string& string) {
            int n_written = gzputs (gz, string.c_str());
            if (n_written < 0) throw Exception ("error writing to file \"" + filename + "\": " + error());
          }

          void write (const char* p, size_t n) {
            int n_written = gzwrite (gz, p, n);
            if (n_written <= 0) throw Exception ("error writing to file \"" + filename + "\": " + error());
          }

      };


    }
  }
}

#endif


