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


    15-10-2008 J-Donald Tournier <d.tournier@brain.org.au>
    * remove MR::DICOM_DW_gradients_PRS flag

*/

#ifndef __file_dicom_mapper_h__
#define __file_dicom_mapper_h__

#include "ptr.h"

namespace MR {

  namespace Image { 
    class Mapper; 
    class Header; 
  }

  namespace File {
    namespace Dicom {

      class Series;

      void dicom_to_mapper (
          MR::Image::Mapper& dmap, 
          MR::Image::Header& H,
          std::vector< RefPtr<Series> >& series);
      
    }
  }
}

#endif




