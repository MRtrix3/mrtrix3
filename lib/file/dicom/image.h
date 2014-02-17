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


#ifndef __file_dicom_image_h__
#define __file_dicom_image_h__

#include "ptr.h"
#include "datatype.h"
#include "math/vector.h"
#include "math/matrix.h"
#include "file/dicom/element.h"

namespace MR {
  namespace File {
    namespace Dicom {

      class Series;
      class Element;

      class Frame { 
        public:
          Frame () { 
            acq_dim[0] = acq_dim[1] = dim[0] = dim[1] = instance = series_num = acq = sequence = UINT_MAX;
            position_vector[0] = position_vector[1] = position_vector[2] = NAN;
            orientation_x[0] = orientation_x[1] = orientation_x[2] = NAN;
            orientation_y[0] = orientation_y[1] = orientation_y[2] = NAN;
            orientation_z[0] = orientation_z[1] = orientation_z[2] = NAN;
            distance = NAN;
            pixel_size[0] = pixel_size[1] = slice_thickness = slice_spacing = NAN; 
            scale_intercept = 0.0;
            scale_slope = 1.0;
            bvalue = G[0] = G[1] = G[2] = NAN;
            data = bits_alloc = data_size = frame_offset = 0;
            DW_scheme_wrt_image = false;
          }

          size_t acq_dim[2], dim[2], series_num, instance, acq, sequence;
          float position_vector[3], orientation_x[3], orientation_y[3], orientation_z[3], distance;
          float pixel_size[2], slice_thickness, slice_spacing, scale_slope, scale_intercept;
          float bvalue, G[3];
          size_t data, bits_alloc, data_size, frame_offset;
          std::string filename;
          bool DW_scheme_wrt_image;
          std::vector<uint32_t> index;

          bool operator< (const Frame& frame) const {
            if (series_num != frame.series_num) 
              return series_num < frame.series_num;
            if (acq != frame.acq) 
              return acq < frame.acq;
            assert (std::isfinite (distance));
            assert (std::isfinite (frame.distance));
            if (distance != frame.distance) 
              return distance < frame.distance;
            for (size_t n = index.size(); n--;)
              if (index[n] != frame.index[n])
                return index[n] < frame.index[n];
            if (sequence != frame.sequence) 
              return sequence < frame.sequence;
            if (instance != frame.instance) 
              return instance < frame.instance;
            return false;
          }


          void calc_distance ()
          {
            if (!std::isfinite (orientation_z[0])) 
              Math::cross (orientation_z, orientation_x, orientation_y);
            else {
              float normal[3];
              Math::cross (normal, orientation_x, orientation_y);
              if (Math::dot (normal, orientation_z) < 0.0) {
                orientation_z[0] = -normal[0];
                orientation_z[1] = -normal[1];
                orientation_z[2] = -normal[2];
              }
              else {
                orientation_z[0] = normal[0];
                orientation_z[1] = normal[1];
                orientation_z[2] = normal[2];
              }
            }

            Math::normalise (orientation_z);
            distance = Math::dot (orientation_z, position_vector);
          }

          static std::vector<size_t> count (const std::vector<Frame*>& frames);
          static float get_slice_separation (const std::vector<Frame*>& frames, size_t nslices);
          static Math::Matrix<float> get_DW_scheme (const std::vector<Frame*>& frames, size_t nslices, const Math::Matrix<float>& image_transform);

          friend std::ostream& operator<< (std::ostream& stream, const Frame& item);
      };











      class Image : public Frame {

        public:
          Image (Series* parent = NULL) : 
            series (parent), 
            images_in_mosaic (0),
            is_BE (false),
            in_frames (false) { }

          Series* series;
          size_t images_in_mosaic;
          std::string  sequence_name, manufacturer;
          bool is_BE, in_frames;

          std::vector<uint32_t> frame_dim;
          std::vector< RefPtr<Frame> > frames;

          void read ();
          void parse_item (Element& item, const std::string& dirname = "");
          void decode_csa (const uint8_t* start, const uint8_t* end);

          bool operator< (const Image& ima) const {
            return Frame::operator< (ima);
          }

          friend std::ostream& operator<< (std::ostream& stream, const Image& item);
      };





    }
  }
}


#endif


