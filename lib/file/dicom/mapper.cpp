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

#include "image/header.h"
#include "image/handler/base.h"
#include "image/handler/default.h"
#include "image/handler/mosaic.h"
#include "file/dicom/mapper.h"
#include "file/dicom/image.h"
#include "file/dicom/series.h"
#include "file/dicom/study.h"
#include "file/dicom/patient.h"
#include "file/dicom/tree.h"

namespace MR {
  namespace File {
    namespace Dicom {


      RefPtr<MR::Image::Handler::Base> dicom_to_mapper (MR::Image::Header& H, std::vector< RefPtr<Series> >& series)
      {
        assert (series.size() > 0);
        RefPtr<MR::Image::Handler::Base> handler;

        Patient* patient (series[0]->study->patient);
        std::string sbuf = ( patient->name.size() ? patient->name : "unnamed" );
        sbuf += " " + format_ID (patient->ID);
        if (series[0]->modality.size()) sbuf 
          += std::string (" [") + series[0]->modality + "]";
        if (series[0]->name.size()) 
          sbuf += std::string (" ") + series[0]->name;
        H.comments().push_back (sbuf);
        H.name() = sbuf;

        // build up sorted list of frames:
        std::vector<Frame*> frames;

        // loop over series list:
        for (std::vector< RefPtr<Series> >::const_iterator series_it = series.begin(); series_it != series.end(); ++series_it) {
          Series& series (**series_it);

          try {
            series.read();
          }
          catch (Exception& E) { 
            E.display();
            throw Exception ("error reading series " + str (series.number) + " of DICOM image \"" + H.name() + "\""); 
          }

          std::sort (series.begin(), series.end(), PtrComp());

          // loop over images in each series:
          for (Series::const_iterator image_it = series.begin(); image_it != series.end(); ++image_it) {
            Image& image (**image_it);

            // if multi-frame, loop over frames in image:
            if (image.frames.size()) {
              std::sort (image.frames.begin(), image.frames.end(), PtrComp());
              for (std::vector< RefPtr<Frame> >::const_iterator frame_it = image.frames.begin(); frame_it != image.frames.end(); ++frame_it)
                frames.push_back (*frame_it);
            }
            // otherwise add image frame:
            else 
              frames.push_back (&image);
          }
        }

        std::vector<size_t> dim = Frame::count (frames);

        if (dim[0]*dim[1]*dim[2] > frames.size())
          throw Exception ("missing image frames for DICOM image \"" + H.name() + "\"");

        if (dim[0] > 1) { // switch axes so slice dim is inner-most:
          std::vector<Frame*> list (frames);
          std::vector<Frame*>::iterator it = frames.begin();
          for (size_t k = 0; k < dim[2]; ++k) 
            for (size_t i = 0; i < dim[0]; ++i) 
              for (size_t j = 0; j < dim[1]; ++j) 
                *(it++) = list[i+dim[0]*(j+dim[1]*k)];
        }

        float slice_separation = Frame::get_slice_separation (frames, dim[1]);


        if (series[0]->study->name.size()) {
          sbuf = "study: " + series[0]->study->name;
          H.comments().push_back (sbuf);
        }

        if (patient->DOB.size()) {
          sbuf = "DOB: " + format_date (patient->DOB);
          H.comments().push_back (sbuf);
        }

        if (series[0]->date.size()) {
          sbuf = "DOS: " + format_date (series[0]->date);
          if (series[0]->time.size()) 
            sbuf += " " + format_time (series[0]->time);
          H.comments().push_back (sbuf);
        }





        const Image& image (*(*series[0])[0]);

        size_t nchannels = image.frames.size() ? 1 : image.data_size / (image.dim[0] * image.dim[1] * (image.bits_alloc/8));
        if (nchannels > 1) 
          INFO ("data segment is larger than expected from image dimensions - interpreting as multi-channel data");

        H.set_ndim (3 + (dim[0]*dim[2]>1) + (nchannels>1));

        size_t current_axis = 0;

        if (nchannels > 1) {
          H.stride(3) = 1;
          H.dim(3) = nchannels;
          ++current_axis;
        }

        H.stride(0) = ++current_axis;
        H.dim(0) = image.dim[0];
        H.vox(0) = image.pixel_size[0];

        H.stride(1) = ++current_axis;
        H.dim(1) = image.dim[1];
        H.vox(1) = image.pixel_size[1];

        H.stride(2) = ++current_axis;
        H.dim(2) = dim[1];
        H.vox(2) = slice_separation;

        if (dim[0]*dim[2] > 1) {
          H.stride(current_axis) = current_axis+1;
          H.dim(current_axis) = dim[0]*dim[2];
          ++current_axis;
        }


        if (image.bits_alloc == 8) 
          H.datatype() = DataType::UInt8;
        else if (image.bits_alloc == 16) {
          H.datatype() = DataType::UInt16;
          if (image.is_BE) 
            H.datatype() = DataType::UInt16 | DataType::BigEndian;
          else 
            H.datatype() = DataType::UInt16 | DataType::LittleEndian;
        }
        else throw Exception ("unexpected number of allocated bits per pixel (" + str (image.bits_alloc) 
            + ") in file \"" + H.name() + "\"");

        H.intensity_offset() = image.scale_intercept;
        H.intensity_scale() = image.scale_slope;


        // If multi-frame, take the transform information from the sorted frames; the first entry in the
        // vector should be the first slice of the first volume
        {
          Math::Matrix<float> M(4,4);
          const Frame* frame (image.frames.size() ? image.frames[0] : &static_cast<const Frame&> (image));

          M(0,0) = -frame->orientation_x[0];
          M(1,0) = -frame->orientation_x[1];
          M(2,0) = +frame->orientation_x[2];
          M(3,0) = 0.0;

          M(0,1) = -frame->orientation_y[0];
          M(1,1) = -frame->orientation_y[1];
          M(2,1) = +frame->orientation_y[2];
          M(3,1) = 0.0;

          M(0,2) = -frame->orientation_z[0];
          M(1,2) = -frame->orientation_z[1];
          M(2,2) = +frame->orientation_z[2];
          M(3,2) = 0.0;

          M(0,3) = -frame->position_vector[0];
          M(1,3) = -frame->position_vector[1];
          M(2,3) = +frame->position_vector[2];
          M(3,3) = 1.0;

          H.transform() = M;
          H.DW_scheme() = Frame::get_DW_scheme (frames, dim[1], M);
        }




        for (size_t n = 1; n < frames.size(); ++n) // check consistency of data scaling:
          if (frames[n]->scale_intercept != frames[n-1]->scale_intercept ||
              frames[n]->scale_slope != frames[n-1]->scale_slope)
            throw Exception ("unable to load series due to inconsistent data scaling between DICOM images");


        if (image.images_in_mosaic) {
          INFO ("DICOM image \"" + H.name() + "\" is in mosaic format");
          if (H.dim (2) != 1)
            throw Exception ("DICOM mosaic contains multiple slices in image \"" + H.name() + "\"");

          H.dim(0) = image.acq_dim[0];
          H.dim(1) = image.acq_dim[1];
          H.dim(2) = image.images_in_mosaic;

          if (image.dim[0] % image.acq_dim[0] || image.dim[1] % image.acq_dim[1]) {
            WARN ("acquisition matrix [ " + str (image.acq_dim[0]) + " " + str (image.acq_dim[1]) 
                + " ] does not fit into DICOM mosaic [ " + str (image.dim[0]) + " " + str (image.dim[1]) 
                + " ] -  adjusting matrix size to suit");
            H.dim(0) = image.dim[0] / size_t (float(image.dim[0]) / float(image.acq_dim[0]));
            H.dim(1) = image.dim[1] / size_t (float(image.dim[1]) / float(image.acq_dim[1]));
          }

          float xinc = H.vox(0) * (image.dim[0] - H.dim(0)) / 2.0;
          float yinc = H.vox(1) * (image.dim[1] - H.dim(1)) / 2.0;
          for (size_t i = 0; i < 3; i++) 
            H.transform()(i,3) += xinc * H.transform()(i,0) + yinc * H.transform()(i,1);

          handler = new MR::Image::Handler::Mosaic (H, image.dim[0], image.dim[1], H.dim (0), H.dim (1), H.dim (2));
        }
        else 
          handler = new MR::Image::Handler::Default (H);

        for (size_t n = 0; n < frames.size(); ++n) 
          handler->files.push_back (File::Entry (frames[n]->filename, frames[n]->data));

        return handler;
      }



    }
  }
}

