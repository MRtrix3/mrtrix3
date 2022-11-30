/* Copyright (c) 2008-2022 the MRtrix3 contributors.
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

      std::shared_ptr<Patient> Tree::find (const std::string& patient_name, const std::string& patient_ID, const std::string& patient_DOB)
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

        push_back (std::shared_ptr<Patient> (new Patient (patient_name, patient_ID, patient_DOB)));
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
          INFO ("error reading file \"" + filename + "\" - ignored");
          return;
        }

        if (! (reader.dim[0] && reader.dim[1] && reader.bits_alloc && reader.data)) {
          INFO ("DICOM file \"" + filename + "\" does not seem to contain image data - ignored");
          return;
        }

        std::shared_ptr<Patient> patient = find (reader.patient, reader.patient_ID, reader.patient_DOB);
        std::shared_ptr<Study> study = patient->find (reader.study, reader.study_ID, reader.study_UID, reader.study_date, reader.study_time);
        for (const auto& image_type : reader.image_type) {
          std::shared_ptr<Series> series = study->find (reader.series, reader.series_number, image_type.first,
              reader.series_ref_UID,  reader.modality, reader.series_date, reader.series_time);

          std::shared_ptr<Image> image (new Image);
          image->filename = filename;
          image->series = series.get();
          image->sequence_name = reader.sequence;
          image->image_type = image_type.first;
          image->transfer_syntax_supported = reader.transfer_syntax_supported;
          series->push_back (image);
        }
      }





      void Tree::read (const std::string& filename)
      {
        description = filename;
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


