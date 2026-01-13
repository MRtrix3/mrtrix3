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

#include "file/dicom/image.h"
#include "file/dicom/patient.h"
#include "file/dicom/series.h"
#include "file/dicom/study.h"
#include "file/dicom/tree.h"

namespace MR::File::Dicom {

std::vector<std::shared_ptr<Series>> select_cmdline(const Tree &tree) {
  std::vector<std::shared_ptr<Series>> series;

  if (tree.empty())
    throw Exception("DICOM tree is empty");

  // ENVVAR name: DICOM_PATIENT
  // ENVVAR when reading DICOM data, match the PatientName entry against
  // ENVVAR the string provided
  const char *patient_env = getenv("DICOM_PATIENT"); // check_syntax off
  const std::string patient_from_env(patient_env == nullptr ? "" : std::string(patient_env));

  // ENVVAR name: DICOM_ID
  // ENVVAR when reading DICOM data, match the PatientID entry against
  // ENVVAR the string provided
  const char *patid_env = getenv("DICOM_ID"); // check_syntax off
  const std::string patid_from_env(patid_env == nullptr ? "" : std::string(patid_env));

  // ENVVAR name: DICOM_STUDY
  // ENVVAR when reading DICOM data, match the StudyName entry against
  // ENVVAR the string provided
  const char *study_env = getenv("DICOM_STUDY"); // check_syntax off
  const std::string study_from_env(study_env == nullptr ? "" : std::string(study_env));

  // ENVVAR name: DICOM_SERIES
  // ENVVAR when reading DICOM data, match the SeriesName entry against
  // ENVVAR the string provided
  const char *series_env = getenv("DICOM_SERIES"); // check_syntax off
  const std::string series_from_env(series_env == nullptr ? "" : std::string(series_env));

  if (!patient_from_env.empty() || !patid_from_env.empty() || !study_from_env.empty() || !series_from_env.empty()) {

    // select using environment variables:

    std::vector<std::shared_ptr<Patient>> patient;
    for (size_t i = 0; i < tree.size(); i++) {
      if ((patient_from_env.empty() || match(patient_from_env, tree[i]->name, true)) &&
          (patid_from_env.empty() || match(patid_from_env, tree[i]->ID, true)))
        patient.push_back(tree[i]);
    }
    if (patient.empty())
      throw Exception("no matching patients in DICOM dataset \"" + tree.description + "\"");
    if (patient.size() > 1)
      throw Exception("too many matching patients in DICOM dataset \"" + tree.description + "\"");

    std::vector<std::shared_ptr<Study>> study;
    for (size_t i = 0; i < patient[0]->size(); i++) {
      if (study_from_env.empty() || match(study_from_env, (*patient[0])[i]->name, true))
        study.push_back((*patient[0])[i]);
    }
    if (study.empty())
      throw Exception("no matching studies in DICOM dataset \"" + tree.description + "\"");
    if (study.size() > 1)
      throw Exception("too many matching studies in DICOM dataset \"" + tree.description + "\"");

    for (size_t i = 0; i < study[0]->size(); i++) {
      if (series_from_env.empty() || match(series_from_env, (*study[0])[i]->name, true))
        series.push_back((*study[0])[i]);
    }
    if (series.empty())
      throw Exception("no matching studies in DICOM dataset \"" + tree.description + "\"");

    return series;
  }

  // select interactively:

  std::string buf;
  const Patient *patient_p = nullptr;
  if (tree.size() > 1) {
    while (patient_p == nullptr) {
      fprintf(stderr, "Select patient (q to abort):\n");
      for (size_t i = 0; i < tree.size(); i++) {
        fprintf(stderr,
                "  %2" PRI_SIZET " - %s %s %s\n",
                i + 1,
                tree[i]->name.c_str(),
                format_ID(tree[i]->ID).c_str(),
                format_date(tree[i]->DOB).c_str());
      }
      std::cerr << "? ";
      std::cin >> buf;
      if (!std::cin || buf[0] == 'q' || buf[0] == 'Q')
        throw CancelException();
      if (isdigit(buf[0])) {
        const int n = to<int>(buf) - 1;
        if (n <= static_cast<int>(tree.size()))
          patient_p = tree[n].get();
      }
      if (!patient_p)
        fprintf(stderr, "invalid selection - try again\n");
    }
  } else
    patient_p = tree[0].get();

  const Patient &patient(*patient_p);

  if (tree.size() > 1) {
    fprintf(stderr,
            "patient: %s %s %s\n",
            patient.name.c_str(),
            format_ID(patient.ID).c_str(),
            format_date(patient.DOB).c_str());
  }

  const Study *study_p = nullptr;
  if (patient.size() > 1) {
    while (study_p == nullptr) {
      fprintf(stderr, "Select study (q to abort):\n");
      for (size_t i = 0; i < patient.size(); i++) {
        fprintf(stderr,
                "  %4" PRI_SIZET " - %s %s %s %s\n",
                i + 1,
                (!patient[i]->name.empty() ? patient[i]->name.c_str() : "unnamed"),
                format_ID(patient[i]->ID).c_str(),
                format_date(patient[i]->date).c_str(),
                format_time(patient[i]->time).c_str());
      }
      std::cerr << "? ";
      std::cin >> buf;
      if (!std::cin || buf[0] == 'q' || buf[0] == 'Q')
        throw CancelException();
      if (isdigit(buf[0])) {
        const int n = to<int>(buf) - 1;
        if (n <= static_cast<int>(patient.size()))
          study_p = patient[n].get();
      }
      if (!study_p)
        fprintf(stderr, "invalid selection - try again\n");
    }
  } else
    study_p = patient[0].get();

  const Study &study(*study_p);

  if (patient.size() > 1) {
    fprintf(stderr,
            "study: %s %s %s %s\n",
            (!study.name.empty() ? study.name.c_str() : "unnamed"),
            format_ID(study.ID).c_str(),
            format_date(study.date).c_str(),
            format_time(study.time).c_str());
  }

  if (study.size() > 1) {
    while (series.empty()) {
      fprintf(stderr, "Select series ('q' to abort):\n");
      for (size_t i = 0; i < study.size(); i++) {
        fprintf(stderr,
                "  %2" PRI_SIZET " - %4" PRI_SIZET " %s images %8s %s (%s) [%" PRI_SIZET "] %s\n",
                i,
                study[i]->size(),
                (!study[i]->modality.empty() ? study[i]->modality.c_str() : ""),
                format_time(study[i]->time).c_str(),
                (!study[i]->name.empty() ? study[i]->name.c_str() : "unnamed"),
                (!(*study[i])[0]->sequence_name.empty() ? (*study[i])[0]->sequence_name.c_str() : "?"),
                study[i]->number,
                study[i]->image_type.c_str());
      }
      std::cerr << "? ";
      std::cin >> buf;
      if (!std::cin || buf[0] == 'q' || buf[0] == 'Q')
        throw CancelException();
      if (isdigit(buf[0])) {
        std::vector<uint32_t> seq;
        try {
          seq = parse_ints<uint32_t>(buf);
          for (size_t i = 0; i < seq.size(); i++) {
            if (seq[i] < 0 || seq[i] >= static_cast<uint32_t>(study.size())) {
              series.clear();
              break;
            }
            series.push_back(study[seq[i]]);
          }
        } catch (Exception) {
          seq.clear();
        }
      }
      if (series.empty())
        fprintf(stderr, "Invalid selection - please try again\n");
    }
  } else
    series.push_back(study[0]);

  return series;
}

std::vector<std::shared_ptr<Series>> (*select_func)(const Tree &tree) = select_cmdline;

} // namespace MR::File::Dicom
