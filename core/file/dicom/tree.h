/* Copyright (c) 2008-2021 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
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

      class Tree : public vector<std::shared_ptr<Patient>> { NOMEMALIGN
        public:
          std::string description;
          void read (const std::string& filename);
          std::shared_ptr<Patient> find (const std::string& patient_name, const std::string& patient_ID = "", 
              const std::string& patient_DOB = "");

          void sort() {
            for (size_t npatient = 0; npatient < size(); ++npatient) {
              Patient& patient (*((*this)[npatient]));
              for (size_t nstudy = 0; nstudy < patient.size(); ++nstudy)
                std::sort (patient[nstudy]->begin(), patient[nstudy]->end(), compare_ptr_contents());
            }
          }

        protected:
          void read_dir (const std::string& filename, ProgressBar& progress);
          void read_file (const std::string& filename);
      }; 

      std::ostream& operator<< (std::ostream& stream, const Tree& item);

      extern vector<std::shared_ptr<Series>> (*select_func) (const Tree& tree);

    }
  }
}

#endif



