/* Copyright (c) 2008-2022 the MRtrix3 contributors.
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

#include "file/dicom/quick_scan.h"
#include "file/dicom/definitions.h"
#include "file/dicom/element.h"
#include "file/dicom/csa_entry.h"
#include "debug.h"

namespace MR {
  namespace File {
    namespace Dicom {



      bool QuickScan::read (const std::string& file_name, bool print_DICOM_fields, bool print_CSA_fields, bool print_Phoenix, bool force_read)
      {
        filename = file_name;
        modality.clear();
        patient.clear();
        patient_ID.clear();
        patient_DOB.clear();
        study.clear();
        study_date.clear();
        study_ID.clear();
        study_time.clear();
        series.clear();
        series_ref_UID.clear();
        image_type.clear();
        series_date.clear();
        series_time.clear();
        sequence.clear();
        series_number = bits_alloc = dim[0] = dim[1] = data = 0;
        transfer_syntax_supported = true;

        {
          Element item;
          try {
            item.set (filename, force_read);
            std::string current_image_type;
            bool in_frames = false;

            while (item.read()) {
              if (!item.ignore_when_parsing()) {

                if      (item.is (0x0008U, 0x0008U)) current_image_type = join (item.get_string(), " ");
                else if (item.is (0x0008U, 0x0020U)) study_date = item.get_string (0);
                else if (item.is (0x0008U, 0x0021U)) series_date = item.get_string (0);
                else if (item.is (0x0008U, 0x0030U)) study_time = item.get_string (0);
                else if (item.is (0x0008U, 0x0031U)) series_time = item.get_string (0);
                else if (item.is (0x0008U, 0x0060U)) modality = item.get_string (0);
                else if (item.is (0x0008U, 0x1030U)) study = item.get_string (0);
                else if (item.is (0x0008U, 0x103EU)) series = item.get_string (0);
                else if (item.is (0x0010U, 0x0010U)) patient = item.get_string (0);
                else if (item.is (0x0010U, 0x0020U)) patient_ID = item.get_string (0);
                else if (item.is (0x0010U, 0x0030U)) patient_DOB = item.get_string (0);
                else if (item.is (0x0018U, 0x0024U)) sequence = item.get_string (0);
                else if (item.is (0x0020U, 0x000DU)) study_UID = item.get_string (0);
                else if (item.is (0x0020U, 0x000EU)) { if (item.is_in_series_ref_sequence()) series_ref_UID = item.get_string(0); }
                else if (item.is (0x0020U, 0x0010U)) study_ID = item.get_string (0);
                else if (item.is (0x0020U, 0x0011U)) series_number = item.get_uint (0);
                else if (item.is (0x0028U, 0x0010U)) dim[1] = item.get_uint (0);
                else if (item.is (0x0028U, 0x0011U)) dim[0] = item.get_uint (0);
                else if (item.is (0x0028U, 0x0100U)) bits_alloc = item.get_uint (0);
                else if (item.is (0x7FE0U, 0x0010U)) data = item.offset (item.data);
                else if (item.is (0xFFFEU, 0xE000U)) {
                  if (item.parents.size() &&
                      item.parents.back().group ==  0x5200U &&
                      item.parents.back().element == 0x9230U) { // multi-frame item
                    if (in_frames) ++image_type[current_image_type];
                    else in_frames = true;
                  }
                }
              }

              if (print_DICOM_fields)
                print (str(item));

              if ((print_CSA_fields || print_Phoenix) && item.group == 0x0029U) {
                if (item.element == 0x1010U ||
                    item.element == 0x1020U ||
                    item.element == 0x1110U ||
                    item.element == 0x1120U ||
                    item.element == 0x1210U ||
                    item.element == 0x1220U) {
                  CSAEntry entry (item.data, item.data + item.size);
                  while (entry.parse()) {
                    const bool is_phoenix = (strcmp ("MrPhoenixProtocol", entry.key()) == 0);
                    if ((print_Phoenix && is_phoenix) || (print_CSA_fields && !is_phoenix)) {
                      if (print_CSA_fields) {
                        print (str (entry));
                      } else {
                        const auto data = entry.get_string();
                        for (const auto& entry : data)
                          print (entry);
                      }
                    } else if (print_CSA_fields && is_phoenix) {
                      print (std::string("[CSA] ") + entry.key() + " (" + str(entry.num_items()) + " items): <");
                      const auto data = entry.get_string();
                      size_t line_count = 0;
                      for (const auto& entry : data) {
                        if (entry.size())
                          line_count += 1;
                        line_count += std::count (entry.begin(), entry.end(), '\n');
                       }
                       print (str(line_count) + " text lines>\n");
                    }
                  }
                }
              }
            }

            ++image_type[current_image_type];

            transfer_syntax_supported = item.transfer_syntax_supported;

          }
          catch (Exception& E) {
            E.display (3);
            return true;
          }
        }
        check_app_exit_code();

        return false;
      }






      std::ostream& operator<< (std::ostream& stream, const QuickScan& file)
      {
        stream << "file: \""
          << file.filename << "\" ["
          << file.modality << "]:\n    patient: "
          << file.patient << " "
          << format_ID(file.patient_ID) << " - "
          << format_date (file.patient_DOB) << "\n    study: "
          << ( file.study.size() ? file.study : "[unspecified]" ) << " "
          << format_ID(file.study_ID) << " - "
          << format_date (file.study_date) << " "
          << format_time (file.study_time) << "\n    series: ["
          << file.series_number << "] "
          << ( file.series.size() ? file.series : "[unspecified]" ) << " - "
          << format_date (file.series_date) << " "
          << format_time (file.series_time) << "\n";
        for (const auto& type : file.image_type)
          stream << "      image type: " << type.first << " [ " << type.second << " frames ]\n";
        stream << "    sequence: " << ( file.sequence.size() ? file.sequence : "[unspecified]" ) << "\n";

          return stream;
      }

    }
  }
}

