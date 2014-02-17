#ifndef __file_dicom_tree_h__
#define __file_dicom_tree_h__

#include "ptr.h"
#include "file/dicom/patient.h"

namespace MR {
  namespace File {
    namespace Dicom {

      class Series; 
      class Patient;

      class Tree : public std::vector< RefPtr<Patient> > { 
        public:
          std::string description;
          void read (const std::string& filename);
          RefPtr<Patient> find (const std::string& patient_name, const std::string& patient_ID = "", 
              const std::string& patient_DOB = "");

          void sort() {
            for (size_t npatient = 0; npatient < size(); ++npatient) {
              Patient& patient (*((*this)[npatient]));
              for (size_t nstudy = 0; nstudy < patient.size(); ++nstudy) 
                std::sort (patient[nstudy]->begin(), patient[nstudy]->end(), PtrComp());
            }
          }

        protected:
          void read_dir (const std::string& filename, ProgressBar& progress);
          void read_file (const std::string& filename);
      }; 

      std::ostream& operator<< (std::ostream& stream, const Tree& item);

      extern std::vector< RefPtr<Series> > (*select_func) (const Tree& tree);

    }
  }
}

#endif



