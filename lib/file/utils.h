/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


#ifndef __file_ops_h__
#define __file_ops_h__

#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "debug.h"
#include "app.h"
#include "mrtrix.h"
#include "types.h"
#include "file/path.h"
#include "file/config.h"


namespace MR
{
  namespace File
  {

    namespace
    {
      inline char random_char ()
      {
        char c = rand () % 62;
        if (c < 10) return c+48;
        if (c < 36) return c+55;
        return c+61;
      }

      const std::string& tmpfile_dir () {
        static const std::string __tmpfile_dir = File::Config::get ("TmpFileDir", ".");
        return __tmpfile_dir;
      }


      const std::string& tmpfile_prefix () {
        static const std::string __tmpfile_prefix = File::Config::get ("TmpFilePrefix", "mrtrix-tmp-");
        return __tmpfile_prefix;
      }

    }




    inline void create (const std::string& filename, int64_t size = 0)
    { 
      int fid = open (filename.c_str(), O_CREAT | O_RDWR | ( App::overwrite_files ? O_TRUNC : O_EXCL ), 0644);
      if (fid < 0) {
        std::string mesg ("error creating file \"" + filename + "\": " + strerror (errno));
        if (errno == EEXIST) 
          mesg += " (use -force option to force overwrite)";
        throw Exception (mesg);
      }

      if (size) size = ftruncate (fid, size);
      close (fid);

      if (size) 
        throw Exception ("cannot resize file \"" + filename + "\": " + strerror (errno));
    }



    inline void resize (const std::string& filename, int64_t size)
    {
      DEBUG ("resizing file \"" + filename + "\" to " + str (size) + "...");

      int fd = open (filename.c_str(), O_RDWR, 0644);
      if (fd < 0)
        throw Exception ("error opening file \"" + filename + "\" for resizing: " + strerror (errno));
      int status = ftruncate (fd, size);
      close (fd);
      if (status)
        throw Exception ("cannot resize file \"" + filename + "\": " + strerror (errno));
    }




    inline bool is_tempfile (const std::string& name, const char* suffix = NULL)
    {
      if (Path::basename (name).compare (0, tmpfile_prefix().size(), tmpfile_prefix())) 
        return false;
      if (suffix) 
        if (!Path::has_suffix (name, suffix)) 
          return false;
      return true;
    }




    inline std::string create_tempfile (int64_t size = 0, const char* suffix = NULL)
    {
      DEBUG ("creating temporary file of size " + str (size));

      std::string filename (Path::join (tmpfile_dir(), tmpfile_prefix()) + "XXXXXX.");
      int rand_index = filename.size() - 7;
      if (suffix) filename += suffix;

      int fid;
      do {
        for (int n = 0; n < 6; n++)
          filename[rand_index+n] = random_char();
        fid = open (filename.c_str(), O_CREAT | O_RDWR | O_EXCL, 0644);
      } while (fid < 0 && errno == EEXIST);

      if (fid < 0)
        throw Exception (std::string ("error creating temporary file in current working directory: ") + strerror (errno));




      int status = size ? ftruncate (fid, size) : 0;
      close (fid);
      if (status) throw Exception ("cannot resize file \"" + filename + "\": " + strerror (errno));
      return filename;
    }


    inline void mkdir (const std::string& folder) 
    {
      if (::mkdir (folder.c_str()
#ifndef MRTRIX_WINDOWS
            , 0755
#endif
            ))
        throw Exception ("error creating folder \"" + folder + "\": " + strerror (errno));
    }

    inline void unlink (const std::string& file) 
    {
      if (::unlink (file.c_str()))
        throw Exception ("error deleting file \"" + file + "\": " + strerror (errno));;
    }

    inline void rmdir (const std::string& folder, bool recursive = false)
    {
      if (recursive) {
        Path::Dir dir (folder);
        std::string entry;
        while ((entry = dir.read_name()).size()) {
          std::string path = Path::join (folder, entry);
          if (Path::is_dir (path))
            rmdir (path, true);
          else 
            unlink (path);
        }
      }
      DEBUG ("deleting folder \"" + folder + "\"...");
      if (::rmdir (folder.c_str()))
        throw Exception ("error deleting folder \"" + folder + "\": " + strerror (errno));
    }


  }
}

#endif

