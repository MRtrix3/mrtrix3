/* Copyright (c) 2008-2017 the MRtrix3 contributors.
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


#ifndef __dwi_tractography_properties_h__
#define __dwi_tractography_properties_h__

#include <map>
#include "timer.h"
#include "dwi/tractography/roi.h"
#include "dwi/tractography/seeding/list.h"


#define TRACTOGRAPHY_FILE_TIMESTAMP_PRECISION 20


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {


      class Properties : public std::map<std::string, std::string> { MEMALIGN(Properties)
        public:

          Properties () { 
            set_timestamp();
          }

          void set_timestamp () {
            (*this)["timestamp"] = str (Timer::current_time(), TRACTOGRAPHY_FILE_TIMESTAMP_PRECISION);
          }

          void set_version_info () {
            (*this)["mrtrix_version"] = App::mrtrix_version;
            if (App::project_version)
              (*this)["project_version"] = App::project_version;
          }

          ROISet include, exclude, mask;
          Seeding::List seeds;
          vector<std::string> comments;
          std::multimap<std::string, std::string> roi;


          void  clear () { 
            std::map<std::string, std::string>::clear(); 
            seeds.clear();
            include.clear();
            exclude.clear();
            mask.clear();
            comments.clear(); 
            roi.clear();
          }

          template <typename T> void set (T& variable, const std::string& name) {
            if ((*this)[name].empty()) (*this)[name] = str (variable);
            else variable = to<T> ((*this)[name]);
          }

          void load_ROIs ();
      };


      inline void check_timestamps (const Properties& a, const Properties& b, const std::string& type) 
      {
        Properties::const_iterator stamp_a = a.find ("timestamp");
        Properties::const_iterator stamp_b = b.find ("timestamp");
        if (stamp_a == a.end() || stamp_b == b.end())
          throw Exception ("unable to verify " + type + " pair: missing timestamp");
        if (stamp_a->second != stamp_b->second)
          throw Exception ("invalid " + type + " combination - timestamps do not match");
      }




      inline void check_counts (const Properties& a, const Properties& b, const std::string& type, bool abort_on_fail) 
      {
        Properties::const_iterator count_a = a.find ("count");
        Properties::const_iterator count_b = b.find ("count");
        if ((count_a == a.end()) || (count_b == b.end())) {
          std::string mesg = "unable to validate " + type + " pair: missing count field";
          if (abort_on_fail) throw Exception (mesg);
          else WARN (mesg);
        }
        if (to<size_t>(count_a->second) != to<size_t>(count_b->second)) {
          std::string mesg = type + " files do not contain same number of elements";
          if (abort_on_fail) throw Exception (mesg);
          else WARN (mesg);
        }
      }





      inline std::ostream& operator<< (std::ostream& stream, const Properties& P)
      {
        stream << "seeds: " << P.seeds;
        stream << "include: " << P.include << ", exclude: " << P.exclude << ", mask: " << P.mask << ", dict: ";
        for (std::map<std::string, std::string>::const_iterator i = P.begin(); i != P.end(); ++i)
          stream << "[ " << i->first << ": " << i->second << " ], ";
        stream << "comments: ";
        for (vector<std::string>::const_iterator i = P.comments.begin(); i != P.comments.end(); ++i)
          stream << "\"" << *i << "\", ";
        return (stream);
      }




    }
  }
}

#endif

