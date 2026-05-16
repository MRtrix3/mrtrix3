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

#include "file/dicom/tree.h"
#include "file/dicom/element.h"
#include "file/dicom/image.h"
#include "file/dicom/patient.h"
#include "file/dicom/quick_scan.h"
#include "file/dicom/series.h"
#include "file/dicom/study.h"
#include "file/path.h"

namespace MR::File::Dicom {

std::shared_ptr<Patient>
Tree::find(std::string_view patient_name, std::string_view patient_ID, std::string_view patient_DOB) {
  for (size_t n = 0; n < size(); n++) {
    bool match = true;
    if (patient_name == (*this)[n]->name) {
      if (!patient_ID.empty() && !(*this)[n]->ID.empty() && patient_ID != (*this)[n]->ID)
        match = false;
      if (match && !patient_DOB.empty() && !(*this)[n]->DOB.empty() && patient_DOB != (*this)[n]->DOB)
        match = false;
      if (match)
        return (*this)[n];
    }
  }

  push_back(std::shared_ptr<Patient>(new Patient(patient_name, patient_ID, patient_DOB)));
  return back();
}

void Tree::read_dir(const std::filesystem::path &dirpath, ProgressBar &progress) {
  try {
    for (const auto &entry : std::filesystem::directory_iterator(dirpath)) {
      if (std::filesystem::is_directory(entry.path()))
        read_dir(entry.path(), progress);
      else {
        try {
          read_file(entry.path());
        } catch (Exception &E) {
          E.display(3);
        }
      }
      ++progress;
    }
  } catch (Exception &E) {
    throw Exception(E, "error opening DICOM folder \"" + dirpath.string() + "\": " + strerror(errno));
  }
}

void Tree::read_file(const std::filesystem::path &filepath) {
  QuickScan reader;
  if (reader.read(filepath)) {
    INFO("error reading file \"" + filepath.string() + "\" - ignored");
    return;
  }

  if (!(reader.dim[0] && reader.dim[1] && reader.bits_alloc && reader.data)) {
    INFO("DICOM file \"" + filepath.string() + "\" does not seem to contain image data - ignored");
    return;
  }

  std::shared_ptr<Patient> patient = find(reader.patient, reader.patient_ID, reader.patient_DOB);
  std::shared_ptr<Study> study =
      patient->find(reader.study, reader.study_ID, reader.study_UID, reader.study_date, reader.study_time);
  for (const auto &image_type : reader.image_type) {
    std::shared_ptr<Series> series = study->find(reader.series,
                                                 reader.series_number,
                                                 image_type.first,
                                                 reader.series_ref_UID,
                                                 reader.modality,
                                                 reader.series_date,
                                                 reader.series_time);

    std::shared_ptr<Image> image(new Image);
    image->filepath = filepath;
    image->series = series.get();
    image->sequence_name = reader.sequence;
    image->image_type = image_type.first;
    image->transfer_syntax_supported = reader.transfer_syntax_supported;
    series->push_back(image);
  }
}

void Tree::read(const std::filesystem::path &path) {
  description = path.string();
  if (std::filesystem::is_directory(path)) {
    ProgressBar progress("scanning folder \"" + shorten(path.string()) + "\" for DICOM data", 0);
    read_dir(path, progress);
  } else {
    try {
      read_file(path);
    } catch (Exception) {
    }
  }

  if (empty())
    throw Exception("no DICOM images found in \"" + path.string() + "\"");
}

std::ostream &operator<<(std::ostream &stream, const Tree &item) {
  stream << "FileSet " << item.description << ":\n";
  for (size_t n = 0; n < item.size(); n++)
    stream << *item[n];
  return stream;
}

} // namespace MR::File::Dicom
