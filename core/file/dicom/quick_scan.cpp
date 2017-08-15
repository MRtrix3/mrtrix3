/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
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

      bool QuickScan::read (const std::string& file_name, bool print_DICOM_fields, bool print_CSA_fields, bool force_read)
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
        image_type.clear();
        series_date.clear();
        series_time.clear();
        sequence.clear();
        series_number = bits_alloc = dim[0] = dim[1] = data = 0;
        transfer_syntax_supported = true;

        Element item;
        try {
          item.set (filename, force_read); 
          std::string current_image_type; 
          bool in_frames = false;

          while (item.read()) {
            if      (item.is (0x0008U, 0x0008U)) current_image_type = join (item.get_string(), " ");
            else if (item.is (0x0008U, 0x0020U)) study_date = item.get_string()[0];
            else if (item.is (0x0008U, 0x0021U)) series_date = item.get_string()[0];
            else if (item.is (0x0008U, 0x0030U)) study_time = item.get_string()[0];
            else if (item.is (0x0008U, 0x0031U)) series_time = item.get_string()[0];
            else if (item.is (0x0008U, 0x0060U)) modality = item.get_string()[0];
            else if (item.is (0x0008U, 0x1030U)) study = item.get_string()[0];
            else if (item.is (0x0008U, 0x103EU)) series = item.get_string()[0];
            else if (item.is (0x0010U, 0x0010U)) patient = item.get_string()[0];
            else if (item.is (0x0010U, 0x0020U)) patient_ID = item.get_string()[0];
            else if (item.is (0x0010U, 0x0030U)) patient_DOB = item.get_string()[0];
            else if (item.is (0x0018U, 0x0024U)) sequence = item.get_string()[0];
            else if (item.is (0x0020U, 0x0010U)) study_ID = item.get_string()[0];
            else if (item.is (0x0020U, 0x0011U)) series_number = item.get_uint()[0];
            else if (item.is (0x0028U, 0x0010U)) dim[1] = item.get_uint()[0];
            else if (item.is (0x0028U, 0x0011U)) dim[0] = item.get_uint()[0];
            else if (item.is (0x0028U, 0x0100U)) bits_alloc = item.get_uint()[0];
            else if (item.is (0x7FE0U, 0x0010U)) data = item.offset (item.data);
            else if (item.is (0xFFFEU, 0xE000U)) {
              if (item.parents.size() &&
                  item.parents.back().group ==  0x5200U &&
                  item.parents.back().element == 0x9230U) { // multi-frame item
                if (in_frames) ++image_type[current_image_type];
                else in_frames = true;
              }
            }

            if (print_DICOM_fields) 
              print (str(item));

            if (print_CSA_fields && item.group == 0x0029U) {
              if (item.element == 0x1010U || item.element == 0x1020U) {
                CSAEntry entry (item.data, item.data + item.size);
                while (entry.parse()) 
                  print (str (entry));
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

