/* Copyright (c) 2008-2025 the MRtrix3 contributors.
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

#include "file/dicom/study.h"
#include "memory.h"

namespace MR::File::Dicom {

class Study;

class Patient : public std::vector<std::shared_ptr<Study>> {
public:
  Patient(std::string_view patient_name, std::string_view patient_ID, std::string_view patient_DOB)
      : name(patient_name), ID(patient_ID), DOB(patient_DOB) {}
  std::string name, ID, DOB;

  std::shared_ptr<Study> find(std::string_view study_name,
                              std::string_view study_ID = "",
                              std::string_view study_UID = "",
                              std::string_view study_date = "",
                              std::string_view study_time = "");

  bool operator<(const Patient &s) const {
    if (name != s.name)
      return name < s.name;
    if (ID != s.ID)
      return ID < s.ID;
    return DOB < s.DOB;
  }
};

std::ostream &operator<<(std::ostream &stream, const Patient &item);

} // namespace MR::File::Dicom
