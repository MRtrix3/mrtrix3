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

#include "file/dicom/image.h"
#include "file/dicom/series.h"
#include "file/dicom/study.h"
#include "file/dicom/patient.h"
#include "file/dicom/tree.h"

namespace MR {
  namespace File {
    namespace Dicom {

      std::vector< RefPtr<Series> > select_cmdline (const Tree& tree)
      {
        std::vector< RefPtr<Series> > series;

        if (tree.size() == 0) throw Exception ("DICOM tree its empty");

        std::string buf;
        const Patient* patient_p = NULL;
        if (tree.size() > 1) {
          while (patient_p == NULL) {
            fprintf(stderr, "Select patient (q to abort):\n");
            for (size_t i = 0; i < tree.size(); i++) {
              fprintf (stderr, "  %2zu - %s %s %s\n", 
                  i+1, 
                  tree[i]->name.c_str(),  
                  format_ID(tree[i]->ID).c_str(),
                  format_date (tree[i]->DOB).c_str() );
            }
            std::cerr << "? ";
            std::cin >> buf;
            if (!isdigit (buf[0])) { series.clear(); return (series); }
            int n = to<int>(buf) - 1;
            if (n > (int) tree.size()) fprintf (stderr, "invalid selection - try again\n");
            else patient_p = tree[n].get();
          }
        }
        else patient_p = tree[0].get();



        const Patient& patient (*patient_p);

        if (tree.size() > 1) {
          fprintf (stderr, "patient: %s %s %s\n", 
              patient.name.c_str(), 
              format_ID (patient.ID).c_str(),
              format_date (patient.DOB).c_str() );
        }


        const Study* study_p = NULL;
        if (patient.size() > 1) {
          while (study_p == NULL) {
            fprintf (stderr, "Select study (q to abort):\n");
            for (size_t i = 0; i < patient.size(); i++) {
              fprintf (stderr, "  %4zu - %s %s %s %s\n", 
                  i+1, 
                  ( patient[i]->name.size() ? patient[i]->name.c_str() : "unnamed" ),
                  format_ID (patient[i]->ID).c_str(), 
                  format_date (patient[i]->date).c_str(), 
                  format_time (patient[i]->time).c_str() );
            }
            std::cerr << "? ";
            std::cin >> buf;
            if (!isdigit (buf[0])) { series.clear(); return (series); }
            int n = to<int>(buf) - 1;
            if (n > (int) patient.size()) fprintf (stderr, "invalid selection - try again\n");
            else study_p = patient[n].get();
          }
        }
        else study_p = patient[0].get();



        const Study& study(*study_p);

        if (patient.size() > 1) {
          fprintf (stderr, "study: %s %s %s %s\n", 
              ( study.name.size() ? study.name.c_str() : "unnamed" ),
              format_ID (study.ID).c_str(),
              format_date (study.date).c_str(),
              format_time (study.time).c_str() );
        }



        if (study.size() > 1) {
          while (series.size() == 0) {
            fprintf (stderr, "Select series ('q' to abort):\n");
            for (size_t i = 0; i < study.size(); i++) {
              fprintf (stderr, "  %2zu - %4zu %s images %8s %s (%s) [%zu]\n", 
                  i,
                  study[i]->size(), 
                  ( study[i]->modality.size() ? study[i]->modality.c_str() : "" ), 
                  format_time (study[i]->time).c_str(), 
                  ( study[i]->name.size() ? study[i]->name.c_str() : "unnamed" ),
                  ( (*study[i])[0]->sequence_name.size() ? (*study[i])[0]->sequence_name.c_str() : "?" ),
                  study[i]->number);
            }
            std::cerr << "? ";
            std::cin >> buf;
            if (!isdigit (buf[0])) { series.clear(); return (series); }
            std::vector<int> seq;
            try { seq = parse_ints (buf); }
            catch (Exception&) { fprintf (stderr, "Invalid number sequence - please try again\n"); seq.clear(); }
            if (seq.size()) {
              for (size_t i = 0; i < seq.size(); i++) {
                if (seq[i] < 0 || seq[i] >= (int) study.size()) {
                  fprintf (stderr, "invalid selection - try again\n");
                  series.clear();
                  break;
                }
                series.push_back (study[seq[i]]);
              }
            }
          }
        }
        else series.push_back (study[0]);

        return (series);
      }



      std::vector< RefPtr<Series> > (*select_func) (const Tree& tree) = select_cmdline;



    }
  }
}

