/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#ifndef __file_dicom_patient_h__
#define __file_dicom_patient_h__

#include "memory.h"
#include "file/dicom/study.h"

namespace MR {
  namespace File {
    namespace Dicom {

      class Study;

      class Patient : public vector<std::shared_ptr<Study>> { NOMEMALIGN
        public:
          Patient (const std::string& patient_name, const std::string& patient_ID = "", 
              const std::string& patient_DOB = "") :
            name (patient_name), ID (patient_ID), DOB (patient_DOB) { }
          std::string name, ID, DOB;

          std::shared_ptr<Study> find (const std::string& study_name, const std::string& study_ID = "", 
              const std::string& study_date = "", const std::string& study_time = "");

      };


      std::ostream& operator<< (std::ostream& stream, const Patient& item);


    }
  }
}


#endif



