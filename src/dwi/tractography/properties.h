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

#ifndef __dwi_tractography_properties_h__
#define __dwi_tractography_properties_h__

#include <map>
#include "app.h"
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

          void update_command_history () {
            // Make sure the current command is not concatenated more than once
            const auto command_history = split_lines ((*this)["command_history"]);
            if (!(command_history.size() && command_history.back() == App::command_string))
              add_line ((*this)["command_history"], App::command_string);
          }

          // In use at time of execution
          ROISet include, exclude, mask;
          Seeding::List seeds;

          // As stored within the header of an existing .tck file
          std::multimap<std::string, std::string> prior_rois;

          vector<std::string> comments;


          void clear () {
            std::map<std::string, std::string>::clear();
            seeds.clear();
            include.clear();
            exclude.clear();
            mask.clear();
            prior_rois.clear();
            comments.clear();
          }

          template <typename T> void set (T& variable, const std::string& name) {
            if ((*this)[name].empty()) (*this)[name] = str (variable);
            else variable = to<T> ((*this)[name]);
          }

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




      inline float get_step_size (const Properties& p)
      {
        for (size_t index = 0; index != 2; ++index) {
          const std::string key (index ? "step_size" : "output_step_size");
          auto it = p.find (key);
          if (it != p.end()) {
            try {
              return to<float> (it->second);
            } catch (...) { }
          }
        }
        return NaN;
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

