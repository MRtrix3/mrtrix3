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

#include "file/dicom/series.h"
#include "file/dicom/study.h"
#include "file/dicom/patient.h"

namespace MR {
  namespace File {
    namespace Dicom {


      bool Series::operator< (const Series& s) const   { return (number < s.number); }



      std::vector<int>  Series::count () const
      {
        std::vector<int> dim (3);
        std::vector<int> current_dim(2);
        dim[0] = dim[1] = dim[2] = 0;
        current_dim[0] = current_dim[1] = 1;

        if (size() == 0) return (dim);

        const Image* first[] = { (*this)[0].get(), (*this)[0].get() };


        for (uint current_entry = 1; current_entry < size(); current_entry++) {

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

        return (dim);
      }






      std::ostream& operator<< (std::ostream& stream, const Series& item)
      {
        stream << MR::printf ("      %4u - %4u %4s images %10s %8s %s\n", 
              item.number, 
              item.size(), 
              ( item.modality.size() ? item.modality.c_str() : "(?)" ),
              format_date(item.date).c_str(),
              format_time(item.time).c_str(),
              item.name.c_str() );

        for (uint n = 0; n < item.size(); n++) stream << (*item[n]);
        return (stream);
      }



    }
  }
}


