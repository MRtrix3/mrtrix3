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
#include "file/config.h"
#include "get_set.h"
#include "file/dicom/mapper.h"
#include "file/dicom/image.h"
#include "file/dicom/series.h"
#include "file/dicom/study.h"
#include "file/dicom/patient.h"
#include "file/dicom/tree.h"
#include "image/format/list.h"
#include "image/header.h"

namespace MR {
  namespace Image {
    namespace Format {

      bool DICOM::read (Header& H) const
      {
        if (!Path::is_dir (H.name())) return (false);

        File::Dicom::Tree dicom;
        
        dicom.read (H.name());
        dicom.sort();

        std::vector< RefPtr<File::Dicom::Series> > series = File::Dicom::select_func (dicom);
        if (series.empty()) throw Exception ("no DICOM series selected");

        dicom_to_mapper (H, series);

        return (true);
      }


      bool DICOM::check (Header& H, size_t num_axes) const { return (false); }
      void DICOM::create (Header& H) const { assert (0); }


    }
  }
}
