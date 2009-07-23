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

#include "ptr.h"
#include "file/dicom/patient.h"

namespace MR {
  namespace File {
    namespace Dicom {

      class Series; 
      class Patient;

      class Tree : public std::vector< RefPtr<Patient> > { 
        protected:
          void    read_dir (const std::string& filename);
          void    read_file (const std::string& filename);
         
        public:
          std::string    description;
          void      read (const std::string& filename);
          RefPtr<Patient>   find (
              const std::string& patient_name, 
              const std::string& patient_ID = "", 
              const std::string& patient_DOB = "");

          void       sort();
      }; 

      std::ostream& operator<< (std::ostream& stream, const Tree& item);

      extern std::vector< RefPtr<Series> > (*select_func) (const Tree& tree);










      inline void Tree::sort() {
        for (uint patient = 0; patient < size(); patient++) {
          Patient& pat (*((*this)[patient]));
          for (uint study = 0; study < pat.size(); study++) 
            std::sort (pat[study]->begin(), pat[study]->end());
        }
      }


    }
  }
}

#endif



