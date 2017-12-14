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


#include "file/dicom/series.h"
#include "file/dicom/study.h"
#include "file/dicom/patient.h"

namespace MR {
  namespace File {
    namespace Dicom {

      vector<int> Series::count () const
      {
        vector<int> dim (3);
        vector<int> current_dim(2);
        dim[0] = dim[1] = dim[2] = 0;
        current_dim[0] = current_dim[1] = 1;

        if (size() == 0) 
          return dim;

        const Image* first[] = { (*this)[0].get(), (*this)[0].get() };


        for (size_t current_entry = 1; current_entry < size(); current_entry++) {

          if ((*this)[current_entry]->acq != first[1]->acq) {
            if (dim[1] && dim[1] != current_dim[1]) 
              throw Exception ("mismatch between number of images along slice dimension");

            if (dim[0] && dim[0] != current_dim[0]) 
              throw Exception ("mismatch between number of images along sequence dimension");

            first[0] = first[1] = (*this)[current_entry].get();
            dim[0] = current_dim[0];
            dim[1] = current_dim[1];
            current_dim[0] = current_dim[1] = 1;
            dim[2]++;
          }
          else if ((*this)[current_entry]->distance != first[0]->distance) {
            if (dim[0] && dim[0] != current_dim[0]) 
              throw Exception ("mismatch between number of images along sequence dimension");

            first[0] = (*this)[current_entry].get();
            dim[0] = current_dim[0];
            current_dim[0] = 1;
            current_dim[1]++;
          }
          else current_dim[0]++;
        }

        if (dim[1] && dim[1] != current_dim[1]) 
          throw Exception ("mismatch between number of images along slice dimension");

        if (dim[0] && dim[0] != current_dim[0]) 
          throw Exception ("mismatch between number of images along sequence dimension");

        dim[0] = current_dim[0];
        dim[1] = current_dim[1];
        dim[2]++;

        return dim;
      }






      std::ostream& operator<< (std::ostream& stream, const Series& item)
      {
        stream << MR::printf ("      %4u - %4u %4s images %10s %8s %s [ %s ]\n", 
              item.number, 
              item.size(), 
              ( item.modality.size() ? item.modality.c_str() : "(?)" ),
              format_date(item.date).c_str(),
              format_time(item.time).c_str(),
              item.name.c_str(),
              item.image_type.c_str() );

        for (size_t n = 0; n < item.size(); n++) 
          stream << *item[n];

        return stream;
      }



    }
  }
}


