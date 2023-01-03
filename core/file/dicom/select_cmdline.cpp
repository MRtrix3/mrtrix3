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

#include "file/dicom/image.h"
#include "file/dicom/series.h"
#include "file/dicom/study.h"
#include "file/dicom/patient.h"
#include "file/dicom/tree.h"

namespace MR {
  namespace File {
    namespace Dicom {

      vector<std::shared_ptr<Series>> select_cmdline (const Tree& tree)
      {
        vector<std::shared_ptr<Series>> series;

        if (tree.size() == 0)
          throw Exception ("DICOM tree its empty");

        //ENVVAR name: DICOM_PATIENT
        //ENVVAR when reading DICOM data, match the PatientName entry against
        //ENVVAR the string provided
        const char* patient_from_env = getenv ("DICOM_PATIENT");

        //ENVVAR name: DICOM_ID
        //ENVVAR when reading DICOM data, match the PatientID entry against
        //ENVVAR the string provided
        const char* patid_from_env = getenv ("DICOM_ID");

        //ENVVAR name: DICOM_STUDY
        //ENVVAR when reading DICOM data, match the StudyName entry against
        //ENVVAR the string provided
        const char* study_from_env = getenv ("DICOM_STUDY");

        //ENVVAR name: DICOM_SERIES
        //ENVVAR when reading DICOM data, match the SeriesName entry against
        //ENVVAR the string provided
        const char* series_from_env = getenv ("DICOM_SERIES");

        if (patient_from_env || patid_from_env || study_from_env || series_from_env) {

          // select using environment variables:

          vector<std::shared_ptr<Patient>> patient;
          for (size_t i = 0; i < tree.size(); i++) {
            if ( (!patient_from_env || match (patient_from_env, tree[i]->name, true)) &&
                 (!patid_from_env || match (patid_from_env, tree[i]->ID, true)) )
              patient.push_back (tree[i]);
          }
          if (patient.empty())
            throw Exception ("no matching patients in DICOM dataset \"" + tree.description + "\"");
          if (patient.size() > 1)
            throw Exception ("too many matching patients in DICOM dataset \"" + tree.description + "\"");

          vector<std::shared_ptr<Study>> study;
          for (size_t i = 0; i < patient[0]->size(); i++) {
            if (!study_from_env || match (study_from_env, (*patient[0])[i]->name, true))
              study.push_back ((*patient[0])[i]);
          }
          if (study.empty())
            throw Exception ("no matching studies in DICOM dataset \"" + tree.description + "\"");
          if (study.size() > 1)
            throw Exception ("too many matching studies in DICOM dataset \"" + tree.description + "\"");

          for (size_t i = 0; i < study[0]->size(); i++) {
            if (!series_from_env || match (series_from_env, (*study[0])[i]->name, true))
              series.push_back ((*study[0])[i]);
          }
          if (series.empty())
            throw Exception ("no matching studies in DICOM dataset \"" + tree.description + "\"");

          return series;
        }




        // select interactively:

        std::string buf;
        const Patient* patient_p = nullptr;
        if (tree.size() > 1) {
          while (patient_p == nullptr) {
            fprintf(stderr, "Select patient (q to abort):\n");
            for (size_t i = 0; i < tree.size(); i++) {
              fprintf (stderr, "  %2" PRI_SIZET " - %s %s %s\n",
                  i+1,
                  tree[i]->name.c_str(),
                  format_ID(tree[i]->ID).c_str(),
                  format_date (tree[i]->DOB).c_str() );
            }
            std::cerr << "? ";
            std::cin >> buf;
            if (buf[0] == 'q' || buf[0] == 'Q')
              throw CancelException();
            if (isdigit (buf[0])) {
              int n = to<int>(buf) - 1;
              if (n <= (int) tree.size())
                patient_p = tree[n].get();
            }
            if (!patient_p)
              fprintf (stderr, "invalid selection - try again\n");
          }
        }
        else
          patient_p = tree[0].get();



        const Patient& patient (*patient_p);

        if (tree.size() > 1) {
          fprintf (stderr, "patient: %s %s %s\n",
              patient.name.c_str(),
              format_ID (patient.ID).c_str(),
              format_date (patient.DOB).c_str() );
        }


        const Study* study_p = nullptr;
        if (patient.size() > 1) {
          while (study_p == nullptr) {
            fprintf (stderr, "Select study (q to abort):\n");
            for (size_t i = 0; i < patient.size(); i++) {
              fprintf (stderr, "  %4" PRI_SIZET " - %s %s %s %s\n",
                  i+1,
                  ( patient[i]->name.size() ? patient[i]->name.c_str() : "unnamed" ),
                  format_ID (patient[i]->ID).c_str(),
                  format_date (patient[i]->date).c_str(),
                  format_time (patient[i]->time).c_str() );
            }
            std::cerr << "? ";
            std::cin >> buf;
            if (buf[0] == 'q' || buf[0] == 'Q')
              throw CancelException();
            if (isdigit (buf[0])) {
              int n = to<int>(buf) - 1;
              if (n <= (int) patient.size())
                study_p = patient[n].get();
            }
            if (!study_p)
              fprintf (stderr, "invalid selection - try again\n");
          }
        }
        else
          study_p = patient[0].get();



        const Study& study(*study_p);

        if (patient.size() > 1) {
          fprintf (stderr, "study: %s %s %s %s\n",
              ( study.name.size() ? study.name.c_str() : "unnamed" ),
              format_ID (study.ID).c_str(),
              format_date (study.date).c_str(),
              format_time (study.time).c_str() );
        }



        if (study.size() > 1) {
          while (series.size() == 0) {
            fprintf (stderr, "Select series ('q' to abort):\n");
            for (size_t i = 0; i < study.size(); i++) {
              fprintf (stderr, "  %2" PRI_SIZET " - %4" PRI_SIZET " %s images %8s %s (%s) [%" PRI_SIZET "] %s\n",
                  i,
                  study[i]->size(),
                  ( study[i]->modality.size() ? study[i]->modality.c_str() : "" ),
                  format_time (study[i]->time).c_str(),
                  ( study[i]->name.size() ? study[i]->name.c_str() : "unnamed" ),
                  ( (*study[i])[0]->sequence_name.size() ? (*study[i])[0]->sequence_name.c_str() : "?" ),
                  study[i]->number, study[i]->image_type.c_str());
            }
            std::cerr << "? ";
            std::cin >> buf;
            if (buf[0] == 'q' || buf[0] == 'Q')
              throw CancelException();
            if (isdigit (buf[0])) {
              vector<uint32_t> seq;
              try {
                seq = parse_ints<uint32_t> (buf);
                for (size_t i = 0; i < seq.size(); i++) {
                  if (seq[i] < 0 || seq[i] >= (uint32_t) study.size()) {
                    series.clear();
                    break;
                  }
                  series.push_back (study[seq[i]]);
                }
              }
              catch (Exception) {
                seq.clear();
              }
            }
            if (series.size() == 0)
              fprintf (stderr, "Invalid selection - please try again\n");
          }
        }
        else series.push_back (study[0]);

        return series;
      }



      vector<std::shared_ptr<Series>> (*select_func) (const Tree& tree) = select_cmdline;



    }
  }
}

