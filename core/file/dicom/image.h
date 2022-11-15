/* Copyright (c) 2008-2022 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#ifndef __file_dicom_image_h__
#define __file_dicom_image_h__

#include <memory>

#include "datatype.h"
#include "types.h"
#include "file/dicom/element.h"

namespace MR {
  namespace File {
    namespace Dicom {

      class Series;
      class Element;

      class Frame { MEMALIGN(Frame)
        public:
          Frame () {
            acq_dim[0] = acq_dim[1] = dim[0] = dim[1] = instance =
                series_num = acq = sequence = echo_index = grad_number = UINT_MAX;
            samples_per_pixel = 1;
            position_vector[0] = position_vector[1] = position_vector[2] = NaN;
            orientation_x[0] = orientation_x[1] = orientation_x[2] = NaN;
            orientation_y[0] = orientation_y[1] = orientation_y[2] = NaN;
            orientation_z[0] = orientation_z[1] = orientation_z[2] = NaN;
            distance = NaN;
            pixel_size[0] = pixel_size[1] = slice_thickness = slice_spacing = NaN;
            scale_intercept = 0.0;
            scale_slope = 1.0;
            bvalue = G[0] = G[1] = G[2] = NaN;
            data = bits_alloc = data_size = frame_offset = 0;
            DW_scheme_wrt_image = false;
            transfer_syntax_supported = true;
            ignore_series_num = false;
            pe_axis = 3;
            pe_sign = 0;
            philips_orientation = '\0';
            pixel_bandwidth = bandwidth_per_pixel_phase_encode = echo_time = inversion_time = repetition_time = flip_angle = partial_fourier = time_after_start = NaN;
            echo_train_length = 0;
            bipolar_flag = readoutmode_flag = 0;
          }

          size_t acq_dim[2], dim[2], series_num, instance, acq, sequence, echo_index, grad_number, samples_per_pixel;
          Eigen::Vector3d position_vector, orientation_x, orientation_y, orientation_z, G;
          default_type distance, pixel_size[2], slice_thickness, slice_spacing, scale_slope, scale_intercept, bvalue;
          size_t data, bits_alloc, data_size, frame_offset;
          std::string filename, image_type;
          bool DW_scheme_wrt_image, transfer_syntax_supported, ignore_series_num;
          size_t pe_axis;
          int pe_sign;
          char philips_orientation;
          Time acquisition_time;
          default_type pixel_bandwidth, bandwidth_per_pixel_phase_encode, echo_time, inversion_time, repetition_time, flip_angle, partial_fourier, time_after_start;
          size_t echo_train_length;
          size_t bipolar_flag, readoutmode_flag;
          vector<uint32_t> index;
          vector<default_type> flip_angles;

          bool operator< (const Frame& frame) const {
            if (!ignore_series_num && series_num != frame.series_num)
              return series_num < frame.series_num;
            if (image_type != frame.image_type)
              return image_type < frame.image_type;
            if (acq != frame.acq)
              return acq < frame.acq;
            if (std::isfinite (distance) && std::isfinite (frame.distance) && distance != frame.distance)
              return distance < frame.distance;
            for (size_t n = index.size(); n--;)
              if (index[n] != frame.index[n])
                return index[n] < frame.index[n];
            if (echo_index != frame.echo_index)
              return echo_index < frame.echo_index;
            if (std::isfinite (echo_time) && echo_time != frame.echo_time)
              return echo_time < frame.echo_time;
            if (grad_number != frame.grad_number)
              return grad_number < frame.grad_number;
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
              if (!orientation_x.allFinite() || !orientation_y.allFinite())
                throw Exception ("slice orientation information missing from DICOM header!");
              Eigen::Vector3d normal = orientation_x.cross (orientation_y);
              if (normal.dot (orientation_z) < 0.0)
                orientation_z = -normal;
              else
                orientation_z = normal;
            }

            if (!position_vector.allFinite())
              throw Exception ("slice position information missing from DICOM header!");

            orientation_z.normalize();
            distance = orientation_z.dot (position_vector);
          }

          bool is_philips_iso () const {
            if (philips_orientation == '\0')
              return false;
            return (philips_orientation == 'I' && bvalue > 0.0);
          }

          static vector<size_t> count (const vector<Frame*>& frames);
          static default_type get_slice_separation (const vector<Frame*>& frames, size_t nslices);
          static std::string get_DW_scheme (const vector<Frame*>& frames, const size_t nslices, const transform_type& image_transform);
          static Eigen::MatrixXd get_PE_scheme (const vector<Frame*>& frames, const size_t nslices);

          friend std::ostream& operator<< (std::ostream& stream, const Frame& item);
      };











      class Image : public Frame { MEMALIGN(Image)

        public:
          Image (Series* parent = nullptr) :
              series (parent),
              images_in_mosaic (0),
              is_BE (false),
              in_frames (false) { }

          Series* series;
          size_t images_in_mosaic;
          std::string sequence_name, manufacturer;
          bool is_BE, in_frames;
          vector<float> mosaic_slices_timing;

          vector<uint32_t> frame_dim;
          vector<std::shared_ptr<Frame>> frames;

          void read ();
          void parse_item (Element& item, const std::string& dirname = "");
          void decode_csa (const uint8_t* start, const uint8_t* end);
          KeyValues read_csa_ascii (const vector<std::string>& data);

          bool operator< (const Image& ima) const {
            return Frame::operator< (ima);
          }

          friend std::ostream& operator<< (std::ostream& stream, const Image& item);

      };





    }
  }
}


#endif


