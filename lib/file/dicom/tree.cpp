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
        bool match;

        for (size_t n = 0; n < size(); n++) {
          match = true;
          if (patient_name == (*this)[n]->name) {
            if (patient_ID.size() && (*this)[n]->ID.size()) 
              if (patient_ID != (*this)[n]->ID) match = false;
            if (match) {
              if (patient_DOB.size() && (*this)[n]->DOB.size())
                if (patient_DOB != (*this)[n]->DOB) match = false;
            }
            if (match) return ((*this)[n]);
          }
        }

        push_back (RefPtr<Patient> (new Patient (patient_name, patient_ID, patient_DOB)));
        return (back());
      }






      void Tree::read_dir (const std::string& filename)
      {
        try { 
          Path::Dir folder (filename); 
          std::string entry;
          while ((entry = folder.read_name()).size()) {
            std::string name (Path::join (filename, entry));
            if (Path::is_dir (name)) read_dir (name);
            else {
              try { read_file (name); }
              catch (Exception& E) { E.display (3); }
            }
            ProgressBar::inc();
          }
        }
        catch (Exception& E) { throw Exception (E, "error opening DICOM folder \"" + filename + "\": " + strerror (errno)); }
      }





      void Tree::read_file (const std::string& filename)
      {
        QuickScan reader;
        if (reader.read (filename)) {
          info ("error reading file \"" + filename + "\" - assuming not DICOM"); 
          return;
        }

        if (! (reader.dim[0] && reader.dim[1] && reader.bits_alloc && reader.data)) {
          info ("DICOM file \"" + filename + "\" does not seem to contain image data - ignored"); 
          return;
        }

        RefPtr<Patient> patient = find (reader.patient, reader.patient_ID, reader.patient_DOB);
        RefPtr<Study> study = patient->find (reader.study, reader.study_ID, reader.study_date, reader.study_time);
        RefPtr<Series> series = study->find (reader.series, reader.series_number, reader.modality, reader.series_date, reader.series_time);

        RefPtr<Image> image (new Image);
        image->filename = filename;
        image->series = series.get();
        image->sequence_name = reader.sequence;
        series->push_back (image);
      }





      void Tree::read (const std::string& filename)
      {
        ProgressBar::init (0, "scanning DICOM folder \"" + shorten (filename) + "\"");
        read_dir (filename);
        ProgressBar::done();

        if (size() > 0) return;

        throw Exception ("no DICOM images found in \"" + filename + "\"");
      }






      std::ostream& operator<< (std::ostream& stream, const Tree& item)
      { 
        stream << "FileSet " << item.description << ":\n";
        for (size_t n = 0; n < item.size(); n++) stream << (*item[n]); 
        return (stream);
      }


    }
  }
}


