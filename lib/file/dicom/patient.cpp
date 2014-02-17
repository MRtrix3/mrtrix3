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


#include "file/dicom/patient.h"
#include "file/dicom/study.h"
#include "file/dicom/series.h"

namespace MR {
  namespace File {
    namespace Dicom {

      RefPtr<Study> Patient::find (const std::string& study_name, const std::string& study_ID, 
          const std::string& study_date, const std::string& study_time)
      {
        for (size_t n = 0; n < size(); n++) {
          bool match = true;
          if (study_name == (*this)[n]->name) {
            if (study_ID.size() && (*this)[n]->ID.size()) 
              if (study_ID != (*this)[n]->ID) 
                match = false;
            if (match) {
              if (study_date.size() && (*this)[n]->date.size())
                if (study_date != (*this)[n]->date) 
                  match = false;
            }
            if (match) {
              if (study_time.size() && (*this)[n]->time.size()) 
                if (study_time != (*this)[n]->time) 
                  match = false;
            }
            if (match) 
              return (*this)[n];
          }
        }

        push_back (RefPtr<Study> (new Study (this, study_name, study_ID, study_date, study_time)));
        return back();
      }






      std::ostream& operator<< (std::ostream& stream, const Patient& item)
      {
        stream << MR::printf ("  %-30s %-16s %10s\n", 
            item.name.c_str(), 
            format_ID (item.ID).c_str(), 
            format_date (item.DOB).c_str());

        for (size_t n = 0; n < item.size(); n++) 
          stream << *item[n];

        return stream;
      }

    }
  }
}


