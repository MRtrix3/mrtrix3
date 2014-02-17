#ifndef __file_dicom_patient_h__
#define __file_dicom_patient_h__

#include "ptr.h"
#include "file/dicom/study.h"

namespace MR {
  namespace File {
    namespace Dicom {

      class Study;

      class Patient : public std::vector< RefPtr<Study> > {
        public:
          Patient (const std::string& patient_name, const std::string& patient_ID = "", 
              const std::string& patient_DOB = "") :
            name (patient_name), ID (patient_ID), DOB (patient_DOB) { }
          std::string name, ID, DOB;

          RefPtr<Study> find (const std::string& study_name, const std::string& study_ID = "", 
              const std::string& study_date = "", const std::string& study_time = "");

      };


      std::ostream& operator<< (std::ostream& stream, const Patient& item);


    }
  }
}


#endif



