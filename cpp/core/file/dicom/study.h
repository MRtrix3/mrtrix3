/* Copyright (c) 2008-2026 the MRtrix3 contributors.
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

#pragma once

#include "file/dicom/series.h"
#include "memory.h"

namespace MR::File::Dicom {

class Patient;
class Series;

class Study : public std::vector<std::shared_ptr<Series>> {
public:
  Study(Patient *parent,
        std::string_view study_name,
        std::string_view study_ID,
        std::string_view study_UID,
        std::string_view study_date,
        std::string_view study_time)
      : patient(parent), name(study_name), ID(study_ID), UID(study_UID), date(study_date), time(study_time) {}

  Patient *patient;
  std::string name, ID, UID, date, time;

  std::shared_ptr<Series> find(std::string_view series_name,
                               size_t series_number,
                               std::string_view image_type,
                               std::string_view series_ref_UID,
                               std::string_view series_modality,
                               std::string_view series_date,
                               std::string_view series_time);

  bool operator<(const Study &s) const {
    if (date != s.date)
      return date < s.date;
    if (time != s.time)
      return time < s.time;
    if (name != s.name)
      return name < s.name;
    if (ID != s.ID)
      return ID < s.ID;
    return UID < s.UID;
  }
};

std::ostream &operator<<(std::ostream &stream, const Study &item);

} // namespace MR::File::Dicom
