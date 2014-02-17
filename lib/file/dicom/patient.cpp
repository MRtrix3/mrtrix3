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


