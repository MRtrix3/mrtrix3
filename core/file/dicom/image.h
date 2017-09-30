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


#ifndef __file_dicom_image_h__
#define __file_dicom_image_h__

#include <memory>

#include "datatype.h"
#include "file/dicom/element.h"

namespace MR {
  namespace File {
    namespace Dicom {

      class Series;
      class Element;

      class Frame { MEMALIGN(Frame)
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
            transfer_syntax_supported = true;
            pe_axis = 3;
            pe_sign = 0;
            pixel_bandwidth = bandwidth_per_pixel_phase_encode = echo_time = repetition_time = flip_angle = partial_fourier_fraction = NAN;
            echo_train_length = parallel_inplane_factor = 0;
            partial_fourier_axis = -1;
          }

          size_t acq_dim[2], dim[2], series_num, instance, acq, sequence;
          Eigen::Vector3 position_vector, orientation_x, orientation_y, orientation_z, G;
          default_type distance, pixel_size[2], slice_thickness, slice_spacing, scale_slope, scale_intercept, bvalue;
          size_t data, bits_alloc, data_size, frame_offset;
          std::string filename;
          bool DW_scheme_wrt_image, transfer_syntax_supported;
          size_t pe_axis;
          int pe_sign;
          default_type pixel_bandwidth, bandwidth_per_pixel_phase_encode, echo_time, repetition_time, flip_angle, partial_fourier_fraction;
          size_t echo_train_length, parallel_inplane_factor;
          int partial_fourier_axis;
          vector<uint32_t> index;

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
              orientation_z = orientation_x.cross (orientation_y);
            else {
              Eigen::Vector3 normal = orientation_x.cross (orientation_y);
              if (normal.dot (orientation_z) < 0.0)
                orientation_z = -normal;
              else
                orientation_z = normal;
            }

            orientation_z.normalize();
            distance = orientation_z.dot (position_vector);
          }

          static vector<size_t> count (const vector<Frame*>& frames);
          static default_type get_slice_separation (const vector<Frame*>& frames, size_t nslices);
          static std::string get_DW_scheme (const vector<Frame*>& frames, const size_t nslices, const transform_type& image_transform);
          static Eigen::MatrixXd get_PE_scheme (const vector<Frame*>& frames, const size_t nslices);

          friend std::ostream& operator<< (std::ostream& stream, const Frame& item);
      };











      class Image : public Frame { MEMALIGN(Image)

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

          vector<uint32_t> frame_dim;
          vector<std::shared_ptr<Frame>> frames;

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


