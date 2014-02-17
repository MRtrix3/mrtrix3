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


#ifndef __file_dicom_series_h__
#define __file_dicom_series_h__

#include "ptr.h"
#include "progressbar.h"
#include "file/dicom/image.h"

namespace MR {
  namespace File {
    namespace Dicom {

      class Study;
      class Image;

      class Series : public std::vector< RefPtr <Image> > {
        public:
          Series (Study* parent, const std::string& series_name, size_t series_number,
              const std::string& series_modality = "", const std::string& series_date = "", const std::string& series_time = "") :
            study (parent), name (series_name), modality (series_modality),
            date (series_date), time (series_time) { 
              number = series_number; 
            }

          Study* study;
          std::string name;
          size_t number;
          std::string modality;
          std::string date;
          std::string time;

          void read () {
            ProgressBar progress ("reading DICOM series \"" + name + "\"...", size());
            for (size_t i = 0; i < size(); i++) {
              (*this)[i]->read(); 
              ++progress;
            }
          }

          std::vector<int> count () const;
          bool operator< (const Series& s) const {
            return number < s.number;
          }

          friend std::ostream& operator<< (std::ostream& stream, const Series& item);
      };


    }
  }
}


#endif



