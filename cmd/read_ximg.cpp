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

#include "command.h"
#include "file/ximg/image.h"


using namespace MR;
using namespace App;

void usage ()
{

  DESCRIPTION 
    + "output XIMG fields in human-readable format.";


  ARGUMENTS
    + Argument ("file",
                "the XIMG file to be scanned.")
    .allow_multiple().type_file ();
}

class Listing {
  public:
    typedef std::map<std::string, VecPtr<File::ImageSlice::Base> > ListType;

    void read (const std::string& filename) 
    {
      try {
        if (Path::is_dir (filename)) {
          Path::Dir dir (filename);
          std::string entry;
          while ((entry = dir.read_name()).size()) 
            read (Path::join (filename, entry));
        }
        else {
          std::string key;
          File::ImageSlice::Base* entry = try_read<File::ImageSlice::XIMG> (filename, key);
          if (entry) {
            if (list.find (key) == list.end()) 
              list[key] = VecPtr<File::ImageSlice::Base>();
            list[key].push_back (entry);
          }
        }
      }
      catch (Exception& E) {
        E.display(2);
      }
    }

    void print () const {
      size_t n = 0;
      for (ListType::const_iterator i = list.begin(); i != list.end(); ++i)
        std::cout << "[" << n++ << "]: " << i->second.size() << " images \"" << i->first << "\"\n";
    }

  protected:
    ListType list;

    template <class R> File::ImageSlice::Base* try_read (const std::string& filename, std::string& key) const {
      try { return R::read (filename, key); }
      catch (Exception& E) { E.display (2); }
      return NULL;
    }
};


void run ()
{
  Listing list;
  for (size_t n = 0; n < argument.size(); ++n) 
    list.read (argument[n]);

  list.print();
}

