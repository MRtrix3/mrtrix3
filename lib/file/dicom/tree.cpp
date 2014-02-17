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


#include "file/path.h"
#include "file/dicom/element.h"
#include "file/dicom/quick_scan.h"
#include "file/dicom/image.h"
#include "file/dicom/series.h"
#include "file/dicom/study.h"
#include "file/dicom/patient.h"
#include "file/dicom/tree.h"

namespace MR {
  namespace File {
    namespace Dicom {

      RefPtr<Patient> Tree::find (const std::string& patient_name, const std::string& patient_ID, const std::string& patient_DOB)
      {
        for (size_t n = 0; n < size(); n++) {
          bool match = true;
          if (patient_name == (*this)[n]->name) {
            if (patient_ID.size() && (*this)[n]->ID.size()) 
              if (patient_ID != (*this)[n]->ID) 
                match = false;
            if (match) {
              if (patient_DOB.size() && (*this)[n]->DOB.size())
                if (patient_DOB != (*this)[n]->DOB) 
                  match = false;
            }
            if (match) 
              return (*this)[n];
          }
        }

        push_back (RefPtr<Patient> (new Patient (patient_name, patient_ID, patient_DOB)));
        return back();
      }






      void Tree::read_dir (const std::string& filename, ProgressBar& progress)
      {
        try { 
          Path::Dir folder (filename); 
          std::string entry;
          while ((entry = folder.read_name()).size()) {
            std::string name (Path::join (filename, entry));
            if (Path::is_dir (name))
              read_dir (name, progress);
            else {
              try { 
                read_file (name); 
              }
              catch (Exception& E) { 
                E.display (3);
              }
            }
            ++progress;
          }
        }
        catch (Exception& E) { 
          throw Exception (E, "error opening DICOM folder \"" + filename + "\": " + strerror (errno)); 
        }
      }





      void Tree::read_file (const std::string& filename)
      {
        QuickScan reader;
        if (reader.read (filename)) {
          INFO ("error reading file \"" + filename + "\" - assuming not DICOM"); 
          return;
        }

        if (! (reader.dim[0] && reader.dim[1] && reader.bits_alloc && reader.data)) {
          INFO ("DICOM file \"" + filename + "\" does not seem to contain image data - ignored"); 
          return;
        }

        RefPtr<Patient> patient = find (reader.patient, reader.patient_ID, reader.patient_DOB);
        RefPtr<Study> study = patient->find (reader.study, reader.study_ID, reader.study_date, reader.study_time);
        RefPtr<Series> series = study->find (reader.series, reader.series_number, reader.modality, reader.series_date, reader.series_time);

        RefPtr<Image> image (new Image);
        image->filename = filename;
        image->series = series;
        image->sequence_name = reader.sequence;
        series->push_back (image);
      }





      void Tree::read (const std::string& filename)
      {
        ProgressBar progress ("scanning DICOM folder \"" + shorten (filename) + "\"", 0);
        if (Path::is_dir (filename))
          read_dir (filename, progress);
        else {
          try {
            read_file (filename);
          }
          catch (Exception) { 
          }
        }

        if (size() > 0) 
          return;

        throw Exception ("no DICOM images found in \"" + filename + "\"");
      }






      std::ostream& operator<< (std::ostream& stream, const Tree& item)
      { 
        stream << "FileSet " << item.description << ":\n";
        for (size_t n = 0; n < item.size(); n++) 
          stream << *item[n]; 
        return stream;
      }


    }
  }
}


