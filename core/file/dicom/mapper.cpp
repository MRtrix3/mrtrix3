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


#include <algorithm>

#include "header.h"
#include "phase_encoding.h"
#include "image_io/default.h"
#include "image_io/mosaic.h"
#include "image_io/variable_scaling.h"
#include "file/dicom/mapper.h"
#include "file/dicom/image.h"
#include "file/dicom/series.h"
#include "file/dicom/study.h"
#include "file/dicom/patient.h"
#include "file/dicom/tree.h"

namespace MR {
  namespace File {
    namespace Dicom {


      std::unique_ptr<MR::ImageIO::Base> dicom_to_mapper (MR::Header& H, vector<std::shared_ptr<Series>>& series)
      {
        assert (series.size() > 0);
        std::unique_ptr<MR::ImageIO::Base> io_handler;

        Patient* patient (series[0]->study->patient);
        std::string sbuf = ( patient->name.size() ? patient->name : "unnamed" );
        sbuf += " " + format_ID (patient->ID);
        if (series[0]->modality.size())
          sbuf += std::string (" [") + series[0]->modality + "]";
        if (series[0]->name.size())
          sbuf += std::string (" ") + series[0]->name;
        add_line (H.keyval()["comments"], sbuf);
        H.name() = sbuf;

        // build up sorted list of frames:
        vector<Frame*> frames;

        // loop over series list:
        for (const auto series_it : series) {
          try {
            series_it->read();
          }
          catch (Exception& E) {
            E.display();
            throw Exception ("error reading series " + str (series_it->number) + " of DICOM image \"" + H.name() + "\"");
          }

          std::sort (series_it->begin(), series_it->end(), compare_ptr_contents());

          // loop over images in each series:
          for (auto image_it : *series_it) {
            if (!image_it->transfer_syntax_supported) {
              Exception E ("unsupported transfer syntax found in DICOM data");
              E.push_back ("consider using third-party tools to convert your data to standard uncompressed encoding");
              E.push_back ("See the MRtrix3 documentation on DICOM handling for details:");
              E.push_back ("   http://mrtrix.readthedocs.io/en/latest/tips_and_tricks/dicom_handling.html#error-unsupported-transfer-syntax");
              throw E;
            }
            // if multi-frame, loop over frames in image:
            if (image_it->frames.size()) {
              std::sort (image_it->frames.begin(), image_it->frames.end(), compare_ptr_contents());
              for (auto frame_it : image_it->frames)
                if (frame_it->image_type == series_it->image_type)
                  frames.push_back (frame_it.get());
            }
            // otherwise add image frame:
            else
              frames.push_back (image_it.get());
          }
        }

        auto dim = Frame::count (frames);

        if (dim[0]*dim[1]*dim[2] < frames.size())
          throw Exception ("dimensions mismatch in DICOM series");

        if (dim[0]*dim[1]*dim[2] > frames.size())
          throw Exception ("missing image frames for DICOM image \"" + H.name() + "\"");

        if (dim[0] > 1) { // switch axes so slice dim is inner-most:
          vector<Frame*> list (frames);
          vector<Frame*>::iterator it = frames.begin();
          for (size_t k = 0; k < dim[2]; ++k)
            for (size_t i = 0; i < dim[0]; ++i)
              for (size_t j = 0; j < dim[1]; ++j)
                *(it++) = list[i+dim[0]*(j+dim[1]*k)];
        }

        default_type slice_separation = Frame::get_slice_separation (frames, dim[1]);

        if (series[0]->study->name.size())
          add_line (H.keyval()["comments"], std::string ("study: " + series[0]->study->name + " [ " + series[0]->image_type + " ]"));

        if (patient->DOB.size())
          add_line (H.keyval()["comments"], std::string ("DOB: " + format_date (patient->DOB)));

        if (series[0]->date.size()) {
          sbuf = "DOS: " + format_date (series[0]->date);
          if (series[0]->time.size())
            sbuf += " " + format_time (series[0]->time);
          add_line (H.keyval()["comments"], sbuf);
        }

        const Image& image (*(*series[0])[0]);
        const Frame& frame (*frames[0]);

        if (std::isfinite (image.echo_time))
          H.keyval()["EchoTime"] = str (0.001 * image.echo_time, 6);
        if (std::isfinite (image.flip_angle))
          H.keyval()["FlipAngle"] = str (image.flip_angle, 6);
        if (std::isfinite (image.repetition_time))
          H.keyval()["RepetitionTime"] = str (0.001 * image.repetition_time, 6);

        size_t nchannels = image.frames.size() ? 1 : image.data_size / (frame.dim[0] * frame.dim[1] * (frame.bits_alloc/8));
        if (nchannels > 1)
          INFO ("data segment is larger than expected from image dimensions - interpreting as multi-channel data");

        H.ndim() = 3 + (dim[0]*dim[2]>1) + (nchannels>1);

        size_t current_axis = 0;

        if (nchannels > 1) {
          H.stride(3) = 1;
          H.size(3) = nchannels;
          ++current_axis;
        }

        H.stride(0) = ++current_axis;
        H.size(0) = frame.dim[0];
        H.spacing(0) = frame.pixel_size[0];

        H.stride(1) = ++current_axis;
        H.size(1) = frame.dim[1];
        H.spacing(1) = frame.pixel_size[1];

        H.stride(2) = ++current_axis;
        H.size(2) = dim[1];
        H.spacing(2) = slice_separation;

        if (dim[0]*dim[2] > 1) {
          H.stride(current_axis) = current_axis+1;
          H.size(current_axis) = dim[0]*dim[2];
          ++current_axis;
        }


        if (frame.bits_alloc == 8)
          H.datatype() = DataType::UInt8;
        else if (frame.bits_alloc == 16) {
          H.datatype() = DataType::UInt16;
          if (image.is_BE)
            H.datatype() = DataType::UInt16 | DataType::BigEndian;
          else
            H.datatype() = DataType::UInt16 | DataType::LittleEndian;
        }
        else throw Exception ("unexpected number of allocated bits per pixel (" + str (frame.bits_alloc)
            + ") in file \"" + H.name() + "\"");

        H.set_intensity_scaling (frame.scale_slope, frame.scale_intercept);


        // If multi-frame, take the transform information from the sorted frames; the first entry in the
        // vector should be the first slice of the first volume
        {
          transform_type M;

          M(0,0) = -frame.orientation_x[0];
          M(1,0) = -frame.orientation_x[1];
          M(2,0) = +frame.orientation_x[2];

          M(0,1) = -frame.orientation_y[0];
          M(1,1) = -frame.orientation_y[1];
          M(2,1) = +frame.orientation_y[2];

          M(0,2) = -frame.orientation_z[0];
          M(1,2) = -frame.orientation_z[1];
          M(2,2) = +frame.orientation_z[2];

          M(0,3) = -frame.position_vector[0];
          M(1,3) = -frame.position_vector[1];
          M(2,3) = +frame.position_vector[2];

          H.transform() = M;
          std::string dw_scheme = Frame::get_DW_scheme (frames, dim[1], M);
          if (dw_scheme.size())
            H.keyval()["dw_scheme"] = dw_scheme;
        }

        PhaseEncoding::set_scheme (H, Frame::get_PE_scheme (frames, dim[1]));

        bool inconsistent_scaling = false;
        for (size_t n = 1; n < frames.size(); ++n) { // check consistency of data scaling:
          if (frames[n]->scale_intercept != frames[n-1]->scale_intercept ||
              frames[n]->scale_slope != frames[n-1]->scale_slope) {
            if (image.images_in_mosaic)
              throw Exception ("unable to load series due to inconsistent data scaling between DICOM mosaic frames");
            inconsistent_scaling = true;
            INFO ("DICOM images contain inconsistency scaling - data will be rescaled and stored in 32-bit floating-point format");
            break;
          }
        }

        // Slice timing may come from a few different potential sources
        vector<float> slices_timing;
        if (image.images_in_mosaic) {
          if (image.mosaic_slices_timing.size() != image.images_in_mosaic) {
            WARN ("Number of entries in mosaic slice timing (" + str(image.mosaic_slices_timing.size()) + ") does not match number of images in mosaic (" + str(image.images_in_mosaic) + "); omitting");
          } else {
            DEBUG ("Taking slice timing information from CSA mosaic info");
            // CSA mosaic defines these in ms; we want them in s
            for (auto f : image.mosaic_slices_timing)
              slices_timing.push_back (0.001 * f);
          }
        } else if (std::isfinite (frame.time_after_start)) {
          DEBUG ("Taking slice timing information from CSA TimeAfterStart field");
          default_type min_time_after_start = std::numeric_limits<default_type>::infinity();
          for (size_t n = 0; n != dim[1]; ++n)
            min_time_after_start = std::min (min_time_after_start, frames[n]->time_after_start);
          for (size_t n = 0; n != dim[1]; ++n)
            slices_timing.push_back (frames[n]->time_after_start - min_time_after_start);
          H.keyval()["SliceTiming"] = join (slices_timing, ",");
        } else if (std::isfinite (static_cast<default_type>(frame.acquisition_time))) {
          DEBUG ("Estimating slice timing from DICOM AcquisitionTime field");
          default_type min_acquisition_time = std::numeric_limits<default_type>::infinity();
          for (size_t n = 0; n != dim[1]; ++n)
            min_acquisition_time = std::min (min_acquisition_time, default_type(frames[n]->acquisition_time));
          for (size_t n = 0; n != dim[1]; ++n)
            slices_timing.push_back (default_type(frames[n]->acquisition_time) - min_acquisition_time);
        }
        if (slices_timing.size()) {
          const size_t slices_acquired_at_zero = std::count (slices_timing.begin(), slices_timing.end(), 0.0f);
          if (slices_acquired_at_zero < (image.images_in_mosaic ? image.images_in_mosaic : dim[1])) {
            H.keyval()["SliceTiming"] = join (slices_timing, ",");
            H.keyval()["MultibandAccelerationFactor"] = str (slices_acquired_at_zero);
            H.keyval()["SliceEncodingDirection"] = "k";
          } else {
            DEBUG ("All slices acquired at same time; not writing slice encoding information");
          }
        } else {
          DEBUG ("No slice timing information obtained");
        }


        if (image.images_in_mosaic) {

          INFO ("DICOM image \"" + H.name() + "\" is in mosaic format");
          if (H.size (2) != 1)
            throw Exception ("DICOM mosaic contains multiple slices in image \"" + H.name() + "\"");

          size_t mosaic_size = std::ceil (std::sqrt (image.images_in_mosaic));
          H.size(0) = std::floor (frame.dim[0] / mosaic_size);
          H.size(1) = std::floor (frame.dim[1] / mosaic_size);
          H.size(2) = image.images_in_mosaic;

          if (frame.acq_dim[0] > size_t(H.size(0)) || frame.acq_dim[1] > size_t(H.size(1))) {
            WARN ("acquisition matrix [ " + str (frame.acq_dim[0]) + " " + str (frame.acq_dim[1])
                + " ] is smaller than expected [ " + str(H.size(0)) + " " + str(H.size(1)) + " ] in DICOM mosaic");
            WARN ("  image may be incorrectly reformatted");
          }

          if (H.size(0)*mosaic_size != frame.dim[0] || H.size(1)*mosaic_size != frame.dim[1]) {
            WARN ("dimensions of DICOM mosaic [ " + str(frame.dim[0]) + " " + str(frame.dim[1])
                + " ] do not match expected size [ " + str(H.size(0)*mosaic_size) + " " + str(H.size(0)*mosaic_size) + " ]");
            WARN ("  assuming data are stored as " + str(mosaic_size)+"x"+str(mosaic_size) + " mosaic of " + str(H.size(0))+"x"+ str(H.size(1)) + " slices.");
            WARN ("  image may be incorrectly reformatted");
          }

          if (frame.acq_dim[0] != size_t(H.size(0)) || frame.acq_dim[1] != size_t(H.size(1)))
            INFO ("note: acquisition matrix [ " + str (frame.acq_dim[0]) + " " + str (frame.acq_dim[1])
                + " ] differs from reconstructed matrix [ " + str(H.size(0)) + " " + str(H.size(1)) + " ]");

          float xinc = H.spacing(0) * (frame.dim[0] - H.size(0)) / 2.0;
          float yinc = H.spacing(1) * (frame.dim[1] - H.size(1)) / 2.0;
          for (size_t i = 0; i < 3; i++)
            H.transform()(i,3) += xinc * H.transform()(i,0) + yinc * H.transform()(i,1);

          io_handler.reset (new MR::ImageIO::Mosaic (H, frame.dim[0], frame.dim[1], H.size (0), H.size (1), H.size (2)));

        }
        else if (inconsistent_scaling) {

          H.reset_intensity_scaling();
          H.datatype() = DataType::Float32;
          H.datatype().set_byte_order_native();

          MR::ImageIO::VariableScaling* handler = new MR::ImageIO::VariableScaling (H);

          for (size_t n = 0; n < frames.size(); ++n)
            handler->scale_factors.push_back ({ frames[n]->scale_intercept, frames[n]->scale_slope });

          io_handler.reset (handler);
        }
        else {

          io_handler.reset (new MR::ImageIO::Default (H));

        }

        for (size_t n = 0; n < frames.size(); ++n)
          io_handler->files.push_back (File::Entry (frames[n]->filename, frames[n]->data));

        return io_handler;
      }



    }
  }
}

