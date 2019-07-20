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
#include "mrtrix.h"
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

          bool has(const std::string& name) const {
            return this->find(name) != this->end();
          }


          std::string get(const std::string& name) const {
            auto p = this->find(name);
            if (p == this->end())
              throw Exception("Undefined property: \"" + name + "\"");
            else 
              return p->second;
          }

          template <typename T> 
          T value_or_default(const std::string& name, const T& defval) const {
            auto p = this->find(name);
            return p == this->end() ? defval : to<T>(p->second);
          }

          template <typename T> 
          void set (T& variable, const std::string& name) {
            if ((*this)[name].empty()) (*this)[name] = str (variable);
            else variable = to<T> ((*this)[name]);
          }

          void erase_if_present(const std::string& name) {
            auto p = this->find(name);
            if (p != this->end())
              this->erase(p);
          }

          inline float get_step_size() const {
            return value_or_default<float>( "output_step_size", value_or_default<float>( "step_size", NaN ) );
          }

          void load_ROIs ();
      };


      // JH: kept for now, but should be removed in favour of: p.get_step_size()
      inline float get_step_size (const Properties& p) { 
        DEBUG( "Function get_step_size is deprecated. Call properties.get_step_size() instead." );
        return p.get_step_size(); 
      }


      // JH: unused for now
      void check_compatible(const Properties& a, const Properties& b, const std::string& type);


      void check_timestamps (const Properties& a, const Properties& b, const std::string& type);


      void check_counts (const Properties& a, const Properties& b, const std::string& type, bool abort_on_fail);


      std::ostream& operator<< (std::ostream& stream, const Properties& P);

    }
  }
}

#endif

