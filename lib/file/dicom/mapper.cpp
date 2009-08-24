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
#include "file/dicom/mapper.h"
#include "file/dicom/image.h"
#include "file/dicom/series.h"
#include "file/dicom/study.h"
#include "file/dicom/patient.h"
#include "file/dicom/tree.h"

namespace MR {
  namespace File {
    namespace Dicom {

      namespace {
        const char* FormatDICOM = "DICOM";
      }


      void dicom_to_mapper (MR::Image::Header& H, std::vector< RefPtr<Series> >& series)
      {
        assert (series.size() > 0);
        H.format = FormatDICOM;

        Patient* patient (series[0]->study->patient);
        std::string sbuf = ( patient->name.size() ? patient->name : "unnamed" );
        sbuf += " " + format_ID (patient->ID);
        if (series[0]->modality.size()) sbuf += std::string (" [") + series[0]->modality + "]";
        if (series[0]->name.size()) sbuf += std::string (" ") + series[0]->name;
        H.comments.push_back (sbuf);
        H.name() = sbuf;

        for (size_t s = 0; s < series.size(); s++) {
          try { series[s]->read(); }
          catch (Exception) { 
            throw Exception ("error reading series " + str (series[s]->number) + " of DICOM image \"" + H.name() + "\""); 
          }
        }


        size_t series_count = series[0]->size();
        sort (series[0]->begin(), series[0]->end());
        std::vector<int> dim = series[0]->count ();

        for (size_t s = 1; s < series.size(); s++) {
          if (series[s]->size() != series_count) 
            throw Exception ("DICOM series selected do not have the same number of images");

          sort (series[s]->begin(), series[s]->end());
          std::vector<int> dim_tmp = series[s]->count();
          if (dim[0] != dim_tmp[0] || dim[1] != dim_tmp[1] || dim[2] != dim_tmp[2]) 
            error ("WARNING: DICOM series selected do not have the same dimensions");
        }



        bool slicesep_warning = false;
        bool slicegap_warning = false;


        float slice_separation = (*series[0])[0]->slice_thickness;
        for (size_t s = 0; s < series.size(); s++) {
          float previous_distance = (*series[s])[0]->distance;
          for (int i = 1; i < dim[1]; i++) {
            const Image& image (*(*series[s])[i*dim[0]]);
            if (!slicegap_warning) {
              if (Math::abs (image.distance - previous_distance - image.slice_thickness) > 1e-4) {
                error ("WARNING: slice gap detected");
                slicegap_warning = true;
                slice_separation = image.distance - previous_distance;
              }
            }

            if (!slicesep_warning) {
              if (Math::abs(image.distance - previous_distance - slice_separation) > 1e-4) {
                slicesep_warning = true;
                error ("WARNING: slice separation is not constant");
              }
            }
            previous_distance = image.distance;
          }
        }



        if (series[0]->study->name.size()) {
          sbuf = "study: " + series[0]->study->name;
          H.comments.push_back (sbuf);
        }

        if (patient->DOB.size()) {
          sbuf = "DOB: " + format_date (patient->DOB);
          H.comments.push_back (sbuf);
        }

        if (series[0]->date.size()) {
          sbuf = "DOS: " + format_date (series[0]->date);
          if (series[0]->time.size()) 
            sbuf += " " + format_time (series[0]->time);
          H.comments.push_back (sbuf);
        }

        MR::Image::Axes& axes (H.axes);
        const Image& image (*(*series[0])[0]);

        series_count = 3;
        size_t expected_data_size = image.dim[0] * image.dim[1] * (image.bits_alloc/8);
        if (image.data_size > expected_data_size) {
          info ("data segment is larger than expected from image dimensions - interpreting as multi-channel data");
          series_count++;
        }
        if (dim[0] > 1) series_count++;
        if (dim[2] > 1) series_count++;
        if (series.size() > 1) series_count++;
        axes.ndim() = series_count;

        int current_axis = 0;
        if (image.data_size > expected_data_size) {
          axes.order(3) = 0;
          axes.dim(3) = image.data_size / expected_data_size;
          axes.vox(3) = NAN;
          axes.description(3).clear();
          axes.units(3).clear();
          current_axis = 1;
        }

        axes.order(0) = current_axis;
        axes.dim(0) = image.dim[0];
        axes.vox(0) = image.pixel_size[0];
        axes.description(0) = MR::Image::Axes::left_to_right;
        axes.units(0) = MR::Image::Axes::millimeters;
        current_axis++;

        axes.order(1) = current_axis;
        axes.dim(1) = image.dim[1];
        axes.vox(1) = image.pixel_size[1];
        axes.description(1) = MR::Image::Axes::posterior_to_anterior;
        axes.units(1) = MR::Image::Axes::millimeters;
        current_axis++;

        axes.order(2) = current_axis;
        axes.dim(2) = dim[1];
        axes.vox(2) = slice_separation;
        axes.description(2) = MR::Image::Axes::inferior_to_superior;
        axes.units(2) = MR::Image::Axes::millimeters;
        current_axis++;

        if (dim[0] > 1) {
          axes.order(current_axis) = current_axis;
          axes.dim(current_axis) = dim[0];
          axes.description(current_axis) = "sequence";
          current_axis++;
        }

        if (dim[2] > 1) {
          axes.order(current_axis) = current_axis;
          axes.dim(current_axis) = dim[2];
          axes.description(current_axis) = "acquisition";
          current_axis++;
        }

        if (series.size() > 1) {
          axes.order(current_axis) = current_axis;
          axes.dim(current_axis) = series.size();
          axes.description(current_axis) = "series";
        }

        if (image.bits_alloc == 8) H.datatype() = DataType::UInt8;
        else if (image.bits_alloc == 16) {
          H.datatype() = DataType::UInt16;
          if (image.is_BE) H.datatype().set_flag (DataType::BigEndian);
          else H.datatype().set_flag (DataType::LittleEndian);
        }
        else throw Exception ("unexpected number of allocated bits per pixel (" + str (image.bits_alloc) 
            + ") in file \"" + H.name() + "\"");

        H.offset = image.scale_intercept;
        H.scale = image.scale_slope;

        Math::Matrix<float> M(4,4);

        M(0,0) = -image.orientation_x[0];
        M(1,0) = -image.orientation_x[1];
        M(2,0) = image.orientation_x[2];
        M(3,0) = 0.0;

        M(0,1) = -image.orientation_y[0];
        M(1,1) = -image.orientation_y[1];
        M(2,1) = image.orientation_y[2];
        M(3,1) = 0.0;

        M(0,2) = -image.orientation_z[0];
        M(1,2) = -image.orientation_z[1];
        M(2,2) = image.orientation_z[2];
        M(3,2) = 0.0;

        M(0,3) = -image.position_vector[0];
        M(1,3) = -image.position_vector[1];
        M(2,3) = image.position_vector[2];
        M(3,3) = 1.0;

        uint8_t* mem = NULL;
        uint8_t* data = NULL;

        if (image.images_in_mosaic) {
          if (axes.dim(2) != 1) 
            throw Exception ("DICOM mosaic contains multiple slices in image \"" + H.name() + "\"");

          if (image.dim[0] % image.acq_dim[0] || image.dim[1] % image.acq_dim[1]) 
            throw Exception ("acquisition matrix [ " + str (image.acq_dim[0]) + " " + str (image.acq_dim[1]) 
                + " ] does not fit into DICOM mosaic [ " + str (image.dim[0]) + " " + str (image.dim[1]) 
                + " ] in image \"" + H.name() + "\"");

          ProgressBar::init (dim[0]*dim[2]*series.size(), "DICOM image contains mosaic files - reformating..."); 

          axes.dim(0) = image.acq_dim[0];
          axes.dim(1) = image.acq_dim[1];
          axes.dim(2) = image.images_in_mosaic;
          off64_t msize = H.datatype().bytes();
          for (size_t i = 0; i < axes.ndim(); i++) 
            msize *= axes.dim(i);

          try { mem = new uint8_t [msize]; }
          catch (...) { throw Exception ("failed to allocate memory for image data!"); }

          data = mem;
        }

        std::vector<const Image*> DW_scheme;

        for (size_t s = 0; s < series.size(); s++) {
          int index[3], n;
          for (index[2] = 0; index[2] < dim[2]; index[2]++) {
            for (index[0] = 0; index[0] < dim[0]; index[0]++) {
              n = index[0] + dim[0]*dim[1]*index[2];
              if (!isnan((*series[s])[n]->bvalue)) DW_scheme.push_back ((*series[s])[n].get());
              for (index[1] = 0; index[1] < dim[1]; index[1]++) {
                n = index[0] + dim[0]*(index[1] + dim[1]*index[2]);

                if (image.images_in_mosaic) {
                  File::MMap fmap;
                  fmap.init ((*series[s])[n]->filename);
                  fmap.map();
                  size_t nbytes = H.data_type.bytes();
                  size_t nbytes_row = nbytes * image.acq_dim[0];
                  uint8_t* mosaic_data = (uint8_t*) fmap.address() + (*series[s])[n]->data;
                  size_t nx = 0, ny = 0;
                  for (size_t z = 0; z < image.images_in_mosaic; z++) {
                    size_t ox = nx*image.acq_dim[0];
                    size_t oy = ny*image.acq_dim[1];
                    for (size_t y = 0; y < image.acq_dim[1]; y++) {
                      memcpy (data, mosaic_data + nbytes*(ox+image.dim[0]*(y+oy)), nbytes_row);
                      data += nbytes_row;
                    }

                    nx++;
                    if (nx >= image.dim[0]/image.acq_dim[0]) { nx = 0; ny++; }
                  }

                  ProgressBar::inc();
                }
                else dmap.add ((*series[s])[n]->filename, (*series[s])[n]->data);
              }
            }
          }
        }

        H.transform_matrix = M;


        if (DW_scheme.size()) {
          H.DW_scheme.allocate (DW_scheme.size(), 4);
          for (size_t s = 0; s < DW_scheme.size(); s++) {
            if ((H.DW_scheme(s, 3) = DW_scheme[s]->bvalue) != 0.0) {
              float norm = Math::norm (DW_scheme[s]->G);
              H.DW_scheme(s, 3) *= norm;
              H.DW_scheme(s, 0) = -DW_scheme[s]->G[0]/norm;
              H.DW_scheme(s, 1) = -DW_scheme[s]->G[1]/norm;
              H.DW_scheme(s, 2) = DW_scheme[s]->G[2]/norm;
            }
            else H.DW_scheme(s, 0) = H.DW_scheme(s, 1) = H.DW_scheme(s, 2) = 0.0;
          }
        }

        if (image.images_in_mosaic) {
          dmap.add (mem);
          ProgressBar::done();
        }

      }






    }
  }
}

