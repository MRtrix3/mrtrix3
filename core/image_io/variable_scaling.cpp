/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#include <limits>

#include "app.h"
#include "progressbar.h"
#include "header.h"
#include "image_io/variable_scaling.h"

namespace MR
{
  namespace ImageIO
  {


    void VariableScaling::load (const Header& header, size_t)
    {
      if (files.empty())
        throw Exception ("no files specified in header for image \"" + header.name() + "\"");

      assert (header.datatype().is_floating_point() && header.datatype().bits() == 32 && header.datatype().is_byte_order_native());
      assert (files.size() == scale_factors.size());
      assert (header.intensity_offset() == 0 && header.intensity_scale() == 1.0);

      size_t voxels_per_segment = segsize / files.size();

      DEBUG ("loading variable-scaling DICOM image \"" + header.name() + "\"...");
      addresses.resize (1);
      addresses[0].reset (new uint8_t [segsize * sizeof(float32)]);
      if (!addresses[0])
        throw Exception ("failed to allocate memory for image \"" + header.name() + "\"");

      ProgressBar progress ("rescaling DICOM images", files.size());
      float32* data = reinterpret_cast<float32*> (addresses[0].get());
      for (size_t n = 0; n < files.size(); n++) {
        const float offset = scale_factors[n].offset;
        const float scale = scale_factors[n].scale;
        File::MMap file (files[n], false, false, sizeof(uint16_t) * voxels_per_segment);
        const uint16_t* from = reinterpret_cast<uint16_t*> (file.address());

        for (size_t i = 0; i < voxels_per_segment; i++)
          data[i] = offset + scale * from[i];

        data += voxels_per_segment;
        ++progress;
      }

    }

    void VariableScaling::unload (const Header& header) { }

  }
}


