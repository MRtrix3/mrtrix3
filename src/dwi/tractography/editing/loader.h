/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 2014.

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

#ifndef __dwi_tractography_editing_loader_h__
#define __dwi_tractography_editing_loader_h__


#include <string>
#include <vector>

#include "ptr.h"

#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"


namespace MR {
namespace DWI {
namespace Tractography {
namespace Editing {




class Loader
{

  public:
    Loader (const std::vector<std::string>& files) :
        file_list (files),
        dummy_properties (),
        reader (new Tractography::Reader<> (file_list[0], dummy_properties)),
        file_index (0) { }

    bool operator() (Tractography::TrackData<>&);


  private:
    const std::vector<std::string>& file_list;
    Tractography::Properties dummy_properties;
    Ptr< Tractography::Reader<> > reader;
    size_t file_index;

};



bool Loader::operator() (Tractography::TrackData<>& out)
{

  out.clear();

  if ((*reader) (out))
    return true;

  while (++file_index != file_list.size()) {
    dummy_properties.clear();
    reader = new Tractography::Reader<> (file_list[file_index], dummy_properties);
    if ((*reader) (out))
      return true;
  }

  return false;

}




}
}
}
}

#endif
