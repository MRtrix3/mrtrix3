/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


#include "dwi/tractography/mapping/mapping.h"



namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Mapping {



        void generate_header (Image::Header& header, const std::string& tck_file_path, const std::vector<float>& voxel_size)
        {

          Tractography::Properties properties;
          Tractography::Reader<float> file (tck_file_path, properties);

          Streamline<float> tck;
          size_t track_counter = 0;

          Point<float> min_values ( INFINITY,  INFINITY,  INFINITY);
          Point<float> max_values (-INFINITY, -INFINITY, -INFINITY);

          {
            ProgressBar progress ("creating new template image...", 0);
            while (file (tck) && track_counter++ < MAX_TRACKS_READ_FOR_HEADER) {
              for (std::vector<Point<float> >::const_iterator i = tck.begin(); i != tck.end(); ++i) {
                min_values[0] = std::min (min_values[0], (*i)[0]);
                max_values[0] = std::max (max_values[0], (*i)[0]);
                min_values[1] = std::min (min_values[1], (*i)[1]);
                max_values[1] = std::max (max_values[1], (*i)[1]);
                min_values[2] = std::min (min_values[2], (*i)[2]);
                max_values[2] = std::max (max_values[2], (*i)[2]);
              }
              ++progress;
            }
          }

          min_values -= Point<float> (3.0*voxel_size[0], 3.0*voxel_size[1], 3.0*voxel_size[2]);
          max_values += Point<float> (3.0*voxel_size[0], 3.0*voxel_size[1], 3.0*voxel_size[2]);

          header.name() = "tckmap image header";
          header.set_ndim (3);

          for (size_t i = 0; i != 3; ++i) {
            header.dim(i) = Math::ceil((max_values[i] - min_values[i]) / voxel_size[i]);
            header.vox(i) = voxel_size[i];
            header.stride(i) = i+1;
            //header.set_units (i, Image::Axis::millimeters);
          }

          //header.set_description (0, Image::Axis::left_to_right);
          //header.set_description (1, Image::Axis::posterior_to_anterior);
          //header.set_description (2, Image::Axis::inferior_to_superior);

          Math::Matrix<float>& M (header.transform());
          M.allocate (4,4);
          M.identity();
          M(0,3) = min_values[0];
          M(1,3) = min_values[1];
          M(2,3) = min_values[2];
          file.close();
        }





        void oversample_header (Image::Header& header, const std::vector<float>& voxel_size)
        {
          INFO ("oversampling header...");

          Math::Matrix<float> transform (header.transform());
          for (size_t j = 0; j != 3; ++j) {
            for (size_t i = 0; i < 3; ++i)
              header.transform()(i,3) += 0.5 * (voxel_size[j] - header.vox(j)) * transform(i,j);
            header.dim(j) = Math::ceil(header.dim(j) * header.vox(j) / voxel_size[j]);
            header.vox(j) = voxel_size[j];
          }
        }





      }
    }
  }
}



