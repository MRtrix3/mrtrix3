/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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

#include "dwi/tractography/properties.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {



      void check_timestamps (const Properties& a, const Properties& b, const std::string& type)
      {
        Properties::const_iterator stamp_a = a.find ("timestamp");
        Properties::const_iterator stamp_b = b.find ("timestamp");
        if (stamp_a == a.end() || stamp_b == b.end())
          throw Exception ("unable to verify " + type + " pair: missing timestamp");
        if (stamp_a->second != stamp_b->second)
          throw Exception ("invalid " + type + " combination - timestamps do not match");
      }



      void check_counts (const Properties& a, const Properties& b, const std::string& type, bool abort_on_fail)
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










      void Properties::set_timestamp ()
      {
        (*this)["timestamp"] = str (Timer::current_time(), TRACTOGRAPHY_FILE_TIMESTAMP_PRECISION);
      }



      void Properties::set_version_info () {
        (*this)["mrtrix_version"] = App::mrtrix_version;
        if (App::project_version)
          (*this)["project_version"] = App::project_version;
      }



      void Properties::update_command_history () {
        // Make sure the current command is not concatenated more than once
        const auto command_history = split_lines ((*this)["command_history"]);
        if (!(command_history.size() && command_history.back() == App::command_history_string))
          add_line ((*this)["command_history"], App::command_history_string);
      }



      void Properties::clear() {
        KeyValues::clear();
        seeds.clear();
        include.clear();
        exclude.clear();
        mask.clear();
        ordered_include.clear();
        prior_rois.clear();
        comments.clear();
      }



      float Properties::get_stepsize() const
      {
        auto it = KeyValues::find ("step_size");
        if (it != KeyValues::end()) {
          try {
            return to<float> (it->second);
          } catch (...) { }
        }
        return NaN;
      }



      void Properties::compare_stepsize_rois() const
      {
        const float step_size = get_stepsize();
        if (!std::isfinite (step_size) || !step_size)
          return;

        auto f = [] (const ROISetBase& rois, const std::string& type, const float threshold)
        {
          for (size_t i = 0; i != rois.size(); ++i) {
            if (rois[i].min_featurelength() < threshold) {
              WARN ("Streamline step size is large compared to " + type + " ROI \"" + rois[i].parameters() + "; "
                    + "risk of streamlines passing through ROI without an intersecting vertex");
            }
          }
        };

        f (include, "include", step_size);
        f (exclude, "exclude", step_size);
        f (mask, "mask", step_size);
        f (ordered_include, "ordered include", step_size);
        // Small seeds are not an issue
      }






      std::ostream& operator<< (std::ostream& stream, const Properties& P)
      {
        stream << "seeds: " << P.seeds;
        stream << "include: " << P.include << ", ordered_include: " << P.ordered_include << ", exclude: " << P.exclude << ", mask: " << P.mask << ", dict: ";
        for (KeyValues::const_iterator i = P.begin(); i != P.end(); ++i)
          stream << "[ " << i->first << ": " << i->second << " ], ";
        stream << "comments: ";
        for (vector<std::string>::const_iterator i = P.comments.begin(); i != P.comments.end(); ++i)
          stream << "\"" << *i << "\", ";
        return (stream);
      }




    }
  }
}
