/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef __file_dicom_tree_h__
#define __file_dicom_tree_h__

#include "memory.h"
#include "file/dicom/patient.h"

namespace MR {
  namespace File {
    namespace Dicom {

      class Series; 
      class Patient;

      class Tree : public std::vector<std::shared_ptr<Patient>> { 
        public:
          std::string description;
          void read (const std::string& filename);
          std::shared_ptr<Patient> find (const std::string& patient_name, const std::string& patient_ID = "", 
              const std::string& patient_DOB = "");

          void sort() {
            for (size_t npatient = 0; npatient < size(); ++npatient) {
              Patient& patient (*((*this)[npatient]));
              for (size_t nstudy = 0; nstudy < patient.size(); ++nstudy) 
                std::sort (patient[nstudy]->begin(), patient[nstudy]->end(), 
                    [](decltype(*patient[nstudy]->begin())& a, decltype(*patient[nstudy]->begin())& b) { return *a < *b; });
            }
          }

        protected:
          void read_dir (const std::string& filename, ProgressBar& progress);
          void read_file (const std::string& filename);
      }; 

      std::ostream& operator<< (std::ostream& stream, const Tree& item);

      extern std::vector<std::shared_ptr<Series>> (*select_func) (const Tree& tree);

    }
  }
}

#endif



