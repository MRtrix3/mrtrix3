#ifndef __file_dicom_study_h__
#define __file_dicom_study_h__

#include "ptr.h"
#include "file/dicom/series.h"

namespace MR {
  namespace File {
    namespace Dicom {

      class Patient;
      class Series;

      class Study : public std::vector< RefPtr<Series> > {
        public:
          Study (Patient* parent, const std::string& study_name, const std::string& study_ID = "", 
              const std::string& study_date = "", const std::string& study_time = "") :
        patient (parent), name (study_name), ID (study_ID),
        date (study_date), time (study_time) { }

          Patient* patient;
          std::string name, ID, date, time;

          RefPtr<Series> find (const std::string& series_name, size_t series_number,
              const std::string& series_modality = "", const std::string& series_date = "", const std::string& series_time = "");
      };


      std::ostream& operator<< (std::ostream& stream, const Study& item);


    }
  }
}


#endif



