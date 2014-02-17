#include "file/dicom/series.h"
#include "file/dicom/study.h"
#include "file/dicom/patient.h"

namespace MR {
  namespace File {
    namespace Dicom {

      std::vector<int> Series::count () const
      {
        std::vector<int> dim (3);
        std::vector<int> current_dim(2);
        dim[0] = dim[1] = dim[2] = 0;
        current_dim[0] = current_dim[1] = 1;

        if (size() == 0) 
          return dim;

        const Image* first[] = { (*this)[0], (*this)[0] };


        for (size_t current_entry = 1; current_entry < size(); current_entry++) {

          if ((*this)[current_entry]->acq != first[1]->acq) {
            if (dim[1] && dim[1] != current_dim[1]) 
              throw Exception ("mismatch between number of images along slice dimension");

            if (dim[0] && dim[0] != current_dim[0]) 
              throw Exception ("mismatch between number of images along sequence dimension");

            first[0] = first[1] = (*this)[current_entry];
            dim[0] = current_dim[0];
            dim[1] = current_dim[1];
            current_dim[0] = current_dim[1] = 1;
            dim[2]++;
          }
          else if ((*this)[current_entry]->distance != first[0]->distance) {
            if (dim[0] && dim[0] != current_dim[0]) 
              throw Exception ("mismatch between number of images along sequence dimension");

            first[0] = (*this)[current_entry];
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
        stream << MR::printf ("      %4u - %4u %4s images %10s %8s %s\n", 
              item.number, 
              item.size(), 
              ( item.modality.size() ? item.modality.c_str() : "(?)" ),
              format_date(item.date).c_str(),
              format_time(item.time).c_str(),
              item.name.c_str() );

        for (size_t n = 0; n < item.size(); n++) 
          stream << *item[n];

        return stream;
      }



    }
  }
}


