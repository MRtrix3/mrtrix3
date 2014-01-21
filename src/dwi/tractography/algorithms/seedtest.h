/*
   Copyright 2011 Brain Research Institute, Melbourne, Australia

   Written by Robert E. Smith, 2012.

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

#ifndef __dwi_tractography_seedtest_h__
#define __dwi_tractography_seedtest_h__

#include "point.h"
#include "dwi/tractography/method.h"
#include "dwi/tractography/shared.h"



namespace MR {
namespace DWI {
namespace Tractography {

class Seedtest : public MethodBase {

  public:

  class Shared : public SharedBase {
    public:
    Shared (const std::string& diff_path, DWI::Tractography::Properties& property_set) :
        SharedBase (diff_path, property_set)
    {
      set_step_size (1.0);
      min_num_points = 1;
      max_num_points = 2;
      unidirectional = true;
      properties["method"] = "Seedtest";
    }
  };

  Seedtest (const Shared& shared) :
    MethodBase (shared),
    S (shared) { }


  bool init () { return true; }
  term_t next () { return EXIT_IMAGE; }


  protected:
    const Shared& S;

};

}
}
}

#endif


