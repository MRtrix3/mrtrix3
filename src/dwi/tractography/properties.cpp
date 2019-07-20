/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#include "dwi/tractography/properties.h"


namespace MR {
  namespace DWI {
    namespace Tractography {

      void Properties::load_ROIs ()
      {
        using iter = std::multimap<std::string,std::string>::const_iterator;

        std::pair<iter,iter> range = prior_rois.equal_range ("include");
        for (iter it = range.first; it != range.second; ++it) include.add (it->second);
        range = prior_rois.equal_range ("exclude");
        for (iter it = range.first; it != range.second; ++it) exclude.add (it->second);
        range = prior_rois.equal_range ("mask");
        for (iter it = range.first; it != range.second; ++it) mask.add (it->second);

        prior_rois.clear();
      }





      void check_compatible (const Properties& a, const Properties& b, const std::string& type)
      {
        if ( a.get_step_size() != b.get_step_size() )
          WARN ("mismatch between " + type + " properties (step-size)");

        for (auto& p: {"source","act","method","threshold"})
          if (a.has(p) && b.has(p) && a.get(p) != b.get(p))
            WARN ("mismatch between " + type + " properties (" + p + ")");
      }

      
      

      void check_timestamps (const Properties& a, const Properties& b, const std::string& type)
      {
        auto stamp_a = a.find ("timestamp");
        auto stamp_b = b.find ("timestamp");
        if (stamp_a == a.end() || stamp_b == b.end())
          throw Exception ("unable to verify " + type + " pair: missing timestamp");
        if (stamp_a->second != stamp_b->second)
          throw Exception ("invalid " + type + " combination - timestamps do not match");
      }




      void check_counts (const Properties& a, const Properties& b, const std::string& type, bool abort_on_fail)
      {
        auto count_a = a.find ("count");
        auto count_b = b.find ("count");
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




      std::ostream& operator<< (std::ostream& stream, const Properties& P)
      {
        stream << "seeds: " << P.seeds;
        stream << "include: " << P.include << ", exclude: " << P.exclude << ", mask: " << P.mask << ", dict: ";
        for (auto& Pk: P)
          stream << "[ " << Pk.first << ": " << Pk.second << " ], ";

        stream << "comments: ";
        for (auto& c: P.comments)
          stream << "\"" << c << "\", ";

        return stream;
      }

    }
  }
}

