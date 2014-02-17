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


#ifndef __file_dicom_quick_scan_h__
#define __file_dicom_quick_scan_h__

#include "mrtrix.h"

namespace MR {
  namespace File {
    namespace Dicom {

      class QuickScan {

        public:
          bool read (const std::string& file_name, bool print_DICOM_fields = false, bool print_CSA_fields = false, bool force_read = false);

          std::string filename, modality;
          std::string patient, patient_ID, patient_DOB;
          std::string study, study_ID, study_date, study_time;
          std::string series, series_date, series_time, sequence;
          size_t series_number, bits_alloc, dim[2], data;
      };

      std::ostream& operator<< (std::ostream& stream, const QuickScan& file);

    }
  }
}

#endif

