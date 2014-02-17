/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


#include "file/dicom/quick_scan.h"
#include "file/dicom/definitions.h"
#include "file/dicom/element.h"
#include "file/dicom/csa_entry.h"

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
        series_date.clear();
        series_time.clear();
        sequence.clear();
        series_number = bits_alloc = dim[0] = dim[1] = data = 0;

        Element item;
        try {
          item.set (filename, force_read); 

          while (item.read()) {
            if      (item.is (0x0008U, 0x0020U)) study_date = item.get_string()[0];
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
            else if (item.is (0x0008U, 0x0008U)) {
              // exclude Siemens MPR info image:
              // TODO: could handle this by splitting on basis on this entry
              std::vector<std::string> V (item.get_string());
              for (size_t n = 0; n < V.size(); n++) {
                if (uppercase (V[n]) == "CSAPARALLEL") 
                  return true;
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
          << format_time (file.series_time) << "\n    sequence: " 
          << ( file.sequence.size() ? file.sequence : "[unspecified]" ) << "\n";

        return stream;
      }

    }
  }
}

