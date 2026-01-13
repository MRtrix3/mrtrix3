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

#include "file/dicom/patient.h"
#include "memory.h"

namespace MR::File::Dicom {

class Series;
class Patient;

class Tree : public std::vector<std::shared_ptr<Patient>> {
public:
  std::string description;
  void read(std::string_view filename);
  std::shared_ptr<Patient>
  find(std::string_view patient_name, std::string_view patient_ID, std::string_view patient_DOB);

  void sort() {
    std::sort(begin(), end(), compare_ptr_contents());
    for (auto patient : *this) {
      std::sort(patient->begin(), patient->end(), compare_ptr_contents());
      for (auto study : *patient)
        std::sort(study->begin(), study->end(), compare_ptr_contents());
    }
  }

protected:
  void read_dir(std::string_view filename, ProgressBar &progress);
  void read_file(std::string_view filename);
};

std::ostream &operator<<(std::ostream &stream, const Tree &item);

extern std::vector<std::shared_ptr<Series>> (*select_func)(const Tree &tree);

} // namespace MR::File::Dicom
