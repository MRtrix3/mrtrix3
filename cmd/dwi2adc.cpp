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

#include "command.h"
#include "progressbar.h"
#include "image/buffer_preload.h"
#include "image/voxel.h"
#include "image/threaded_copy.h"
#include "math/matrix.h"
#include "math/least_squares.h"
#include "dwi/gradient.h"


using namespace MR;
using namespace App;
using namespace std;


void usage ()
{
  DESCRIPTION 
    + "convert mean dwi (trace-weighted) images to mean adc maps";

  ARGUMENTS 
    + Argument ("input", "the input image.").type_image_in ()
    + Argument ("output", "the output image.").type_image_out ();

  OPTIONS 
    + DWI::GradOption;
}



typedef float value_type;
typedef Image::BufferPreload<value_type> InputBufferType;
typedef Image::Buffer<value_type> OutputBufferType;



class DWI2ADC {
  public:
    DWI2ADC (InputBufferType::voxel_type& dwi_vox, OutputBufferType::voxel_type& adc_vox, const Math::Matrix<value_type>& binv, size_t dwi_axis) : 
      dwi_vox (dwi_vox), adc_vox (adc_vox), dwi (dwi_vox.dim(dwi_axis)), adc (2), binv (binv), dwi_axis (dwi_axis) { }

    void operator() (const Image::Iterator& pos) {
      Image::voxel_assign (dwi_vox, pos);
      Image::voxel_assign (adc_vox, pos);
      for (dwi_vox[dwi_axis] = 0; dwi_vox[dwi_axis] < dwi_vox.dim(dwi_axis); ++dwi_vox[dwi_axis]) {
        value_type val = dwi_vox.value();
        dwi[dwi_vox[dwi_axis]] = val ? std::log (val) : 1.0e-12;
      }

      Math::mult (adc, binv, dwi);

      adc_vox[3] = 0;
      adc_vox.value() = std::exp (adc[0]);
      adc_vox[3] = 1;
      adc_vox.value() = adc[1];
    }

  protected:
    InputBufferType::voxel_type dwi_vox;
    OutputBufferType::voxel_type adc_vox;
    Math::Vector<value_type> dwi, adc;
    const Math::Matrix<value_type>& binv;
    const size_t dwi_axis;
};




void run () {
  InputBufferType dwi_buffer (argument[0]);
  Math::Matrix<value_type> grad = DWI::get_valid_DW_scheme<value_type> (dwi_buffer);

  size_t dwi_axis = 3;
  while (dwi_buffer.dim (dwi_axis) < 2) ++dwi_axis;
  INFO ("assuming DW images are stored along axis " + str (dwi_axis));

  Math::Matrix<value_type> b (grad.rows(), 2);
  for (size_t i = 0; i < b.rows(); ++i) {
    b(i,0) = 1.0;
    b(i,1) = -grad (i,3);
  }

  Math::Matrix<float> binv = Math::pinv (b);

  Image::Header header (dwi_buffer);
  header.datatype() = DataType::Float32;
  header.set_ndim (4);
  header.dim(3) = 2;

  OutputBufferType adc_buffer (argument[1], header);

  auto dwi_vox = dwi_buffer.voxel();
  auto adc_vox = adc_buffer.voxel();

  Image::ThreadedLoop ("computing ADC values...", dwi_vox, 1, 0, 3)
    .run (DWI2ADC (dwi_vox, adc_vox, binv, dwi_axis));
}


