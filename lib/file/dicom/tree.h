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



