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

#ifndef __dwi_tractography_properties_h__
#define __dwi_tractography_properties_h__

#include <map>
#include "dwi/tractography/roi.h"

namespace MR {
  namespace DWI {
    namespace Tractography {

      class Properties : public std::map<std::string, std::string> {
        public:
          std::vector<RefPtr<ROI> >  roi;
          std::vector<std::string>          comments;

          void  clear () { std::map<std::string, std::string>::clear(); roi.clear(); comments.clear(); }
      };





      inline std::ostream& operator<< (std::ostream& stream, const Properties& P)
      {
        stream << "ROI: ";
        for (std::vector<RefPtr<ROI> >::const_iterator i = P.roi.begin(); i != P.roi.end(); ++i) stream << *(*i) << ", ";
        stream << "dict: ";
        for (std::map<std::string, std::string>::const_iterator i = P.begin(); i != P.end(); ++i) stream << "[ " << i->first << ": " << i->second << " ], ";
        stream << "comments: ";
        for (std::vector<std::string>::const_iterator i = P.comments.begin(); i != P.comments.end(); ++i) stream << "\"" << *i << "\", ";
        return (stream);
      }

    }
  }
}

#endif

