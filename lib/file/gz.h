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
#include "types.h"
#include "exception.h"
#include "file/path.h"

namespace MR
{
  namespace File
  {

    class GZ
    {
      public:
        GZ () : gz (NULL) { }
        GZ (const std::string& fname, const char* mode) : gz (NULL) {
          open (fname, mode);
        }
        ~GZ () {
          close();
        }

        const std::string&  name () const  {
          return (filename);
        }

        void open (const std::string& fname, const char* mode) {
          close();
          filename = fname;
          if (!MR::Path::exists (filename))
            throw Exception ("cannot access file \"" + filename + "\": No such file or directory");

          gz = gzopen (filename.c_str(), mode);
          if (!gz)
            throw Exception ("error opening file \"" + filename + "\": insufficient memory");
        }

        void close () {
          if (gz) {
            if (gzclose (gz))
              throw Exception ("error closing file \"" + filename + "\": " + error());
            filename.clear();
            gz = NULL;
          }
        }

        bool is_open () const {
          return (gz);
        }
        bool eof () const {
          assert (gz);
          return (gzeof (gz));
        }
        int64_t tell () const {
          assert (gz);
          return (gztell (gz));
        }

        void seek (int64_t offset) {
          assert (gz);
          z_off_t pos = gzseek (gz, offset, SEEK_SET);
          if (pos < 0)
            throw Exception ("error seeking in file \"" + filename + "\": " + error());
        }

        int read (char* s, size_t n) {
          assert (gz);
          int n_read = gzread (gz, s, n);
          if (n_read < 0)
            throw Exception ("error reading from file \"" + filename + "\": " + error());
          return (n_read);
        }

        void write (const char* s, size_t n) {
          assert (gz);
          if (gzwrite (gz, s, n) <= 0)
            throw Exception ("error writing to file \"" + filename + "\": " + error());
        }

        void write (const std::string& s) {
          assert (gz);
          if (gzputs (gz, s.c_str()) < 0)
            throw Exception ("error writing to file \"" + filename + "\": " + error());
        }

        std::string getline () {
          assert (gz);
          std::string string;
          char buf[64];
          do {
            buf[0] = 0;
            char* status = gzgets (gz, buf, 64);
            if (!status) {
              if (eof()) break;
              throw Exception ("error reading from file \"" + filename + "\": " + error());
            }
            string += buf;
          }
          while (strlen (buf) >= 63);
          if (string.size() && (string[string.size()-1] == 015 || string[string.size()-1] == '\n'))
            string.resize (string.size()-1);
          return (string);
        }

        template <typename T> T get () {
          T val;
          if (read (&val, sizeof (T)) != sizeof (T))
            throw Exception ("error reading from file \"" + filename + "\": " + error());
          return (val);
        }

        template <typename T> T get (int64_t offset) {
          seek (offset);
          return (get<T>());
        }

        template <typename T> T* get (T* buf, size_t n) {
          if (read (buf, n*sizeof (T)) != n*sizeof (T))
            throw Exception ("error reading from file \"" + filename + "\": " + error());
          return (buf);
        }

        template <typename T> T* get (int64_t offset, T* buf, size_t n) {
          seek (offset);
          return (get<T> (buf, n));
        }

      protected:
        gzFile       gz;
        std::string  filename;

        const char*  error () {
          int error_number;
          const char* s = gzerror (gz, &error_number);
          if (error_number == Z_ERRNO) s = strerror (errno);
          return s;
        }
    };

  }
}

#endif


