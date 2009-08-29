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
#include "file/misc.h"
#include "file/nifti1_utils.h"
#include "image/misc.h"
#include "image/header.h"
#include "image/format/list.h"

namespace MR {
  namespace Image {
    namespace Format {

      bool NIfTI::read (Header& H) const
      {
        if (!Path::has_suffix (H.name(), ".nii")) return (false);

        File::MMap fmap (H.name());
        size_t data_offset = File::NIfTI::read (H, *((const nifti_1_header*) fmap.address()));

        H.files.push_back (File::Entry (H.name(), data_offset));

        return (true);
      }





      bool NIfTI::check (Header& H, int num_axes) const
      {
        if (!Path::has_suffix (H.name(), ".nii")) return (false);
        if (num_axes < 3) throw Exception ("cannot create NIfTI-1.1 image with less than 3 dimensions");
        if (num_axes > 8) throw Exception ("cannot create NIfTI-1.1 image with more than 8 dimensions");

        H.axes.ndim() = num_axes;
        for (size_t i = 0; i < H.ndim(); i++) {
          if (H.axes.dim(i) < 1) H.axes.dim(i) = 1;
          H.axes.order(i) = i;
          H.axes.forward(i) = true;
        }

        H.axes.description(0) = Axes::left_to_right;
        H.axes.units(0) = Axes::millimeters;

        H.axes.description(1) = Axes::posterior_to_anterior;
        H.axes.units(1) = Axes::millimeters;

        H.axes.description(1) = Axes::inferior_to_superior;
        H.axes.units(1) = Axes::millimeters;

        return (true);
      }





      void NIfTI::create (Header& H) const
      {
        if (H.ndim() > 7) 
          throw Exception ("NIfTI-1.1 format cannot support more than 7 dimensions for image \"" + H.name() + "\"");

        nifti_1_header NH;
        File::NIfTI::write (NH, H);

        File::create (H.name(), 352 + memory_footprint (H));

        std::ofstream out (H.name().c_str());
        if (!out) throw Exception ("error opening file \"" + H.name() + "\" for writing: " + strerror (errno));
        out.write ((char*) &NH, 352);
        out.close();

        H.files.push_back (File::Entry (H.name(), 352));
      }

    }
  }
}

