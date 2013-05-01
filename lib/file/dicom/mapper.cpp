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
#include "image/handler/default.h"
#include "image/handler/mosaic.h"
#include "file/dicom/mapper.h"
#include "file/dicom/image.h"
#include "file/dicom/series.h"
#include "file/dicom/study.h"
#include "file/dicom/patient.h"
#include "file/dicom/tree.h"

namespace MR
{
  namespace File
  {
    namespace Dicom
    {

      MR::Image::Handler::Base* dicom_to_mapper (MR::Image::Header& H, std::vector< RefPtr<Series> >& series)
      {
        assert (series.size() > 0);
        Ptr<MR::Image::Handler::Base> handler;

        Patient* patient (series[0]->study->patient);
        std::string sbuf = (patient->name.size() ? patient->name : "unnamed");
        sbuf += " " + format_ID (patient->ID);
        if (series[0]->modality.size()) sbuf += std::string (" [") + series[0]->modality + "]";
        if (series[0]->name.size()) sbuf += std::string (" ") + series[0]->name;
        H.comments().push_back (sbuf);
        H.name() = sbuf;

        for (size_t s = 0; s < series.size(); s++) {
          try {
            series[s]->read();
          }
          catch (Exception& E) {
            throw Exception (E, "error reading series " + str (series[s]->number) + " of DICOM image \"" + H.name() + "\"");
          }
        }


        size_t series_count = series[0]->size();
        std::sort (series[0]->begin(), series[0]->end(), PtrComp());
        std::vector<int> dim = series[0]->count ();

        for (size_t s = 1; s < series.size(); s++) {
          if (series[s]->size() != series_count)
            throw Exception ("DICOM series selected do not have the same number of images");

          std::sort (series[s]->begin(), series[s]->end(), PtrComp());
          std::vector<int> dim_tmp = series[s]->count();
          if (dim[0] != dim_tmp[0] || dim[1] != dim_tmp[1] || dim[2] != dim_tmp[2])
            WARN ("DICOM series selected do not have the same dimensions");
        }



        bool slicesep_warning = false;

        const float slice_thickness = (*series[0]) [0]->slice_thickness;
        float slice_separation = (*series[0]) [0]->slice_spacing;
        if (!finite (slice_separation))
          slice_separation = slice_thickness;

        for (size_t s = 0; s < series.size(); s++) {
          float previous_distance = (*series[s]) [0]->distance;
          for (int i = 1; i < dim[1]; i++) {
            const Image& image (* (*series[s]) [i*dim[0]]);
            float sep = image.distance - previous_distance;

            if (finite (slice_separation))
              if (Math::abs (sep - slice_separation) > 1e-4)
                slicesep_warning = true;

            if (!finite (slice_separation) || sep > slice_separation + 1e-4)
              slice_separation = sep;

            previous_distance = image.distance;
          }
        }

        if (slicesep_warning)
          WARN ("slice separation is not constant");

        if (Math::abs (slice_separation - slice_thickness) > 1e-4)
          WARN ("slice gap detected");



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

        const Image& image (* (*series[0]) [0]);

        series_count = 3;
        size_t expected_data_size = image.dim[0] * image.dim[1] * (image.bits_alloc/8);
        if (image.data_size > expected_data_size) {
          INFO ("data segment is larger than expected from image dimensions - interpreting as multi-channel data");
          series_count++;
        }
        if (dim[0] > 1) series_count++;
        if (dim[2] > 1) series_count++;
        if (series.size() > 1) series_count++;
        H.set_ndim (series_count);

        ssize_t current_axis = 0;
        if (image.data_size > expected_data_size) {
          H.stride(3) = 1;
          H.dim(3) = image.data_size / expected_data_size;
          H.vox(3) = NAN;
          ++current_axis;
        }

        H.stride(0) = current_axis+1;
        H.dim(0) = image.dim[0];
        H.vox(0) = image.pixel_size[0];
        ++current_axis;

        H.stride(1) = current_axis+1;
        H.dim(1) = image.dim[1];
        H.vox(1) = image.pixel_size[1];
        ++current_axis;

        H.stride(2) = current_axis+1;
        H.dim(2) = dim[1];
        H.vox(2) = slice_separation;
        ++current_axis;

        if (dim[0] > 1) {
          H.stride(current_axis) = current_axis+1;
          H.dim(current_axis) = dim[0];
          current_axis++;
        }

        if (dim[2] > 1) {
          H.stride(current_axis) = current_axis+1;
          H.dim(current_axis) = dim[2];
          current_axis++;
        }

        if (series.size() > 1) {
          H.stride(current_axis) = current_axis+1;
          H.dim(current_axis) = series.size();
        }

        if (image.bits_alloc == 8)
          H.datatype() = DataType::UInt8;
        else if (image.bits_alloc == 16) {
          if (image.is_BE) 
            H.datatype() = DataType::UInt16 | DataType::BigEndian;
          else 
            H.datatype() = DataType::UInt16 | DataType::LittleEndian;
        }
        else throw Exception ("unexpected number of allocated bits per pixel ("
                                + str (image.bits_alloc) + ") in file \"" + H.name() + "\"");

        H.intensity_offset() = image.scale_intercept;
        H.intensity_scale() = image.scale_slope;

        Math::Matrix<float>& M (H.transform());
        M.allocate (4,4);

        M (0,0) = -image.orientation_x[0];
        M (1,0) = -image.orientation_x[1];
        M (2,0) = image.orientation_x[2];
        M (3,0) = 0.0;

        M (0,1) = -image.orientation_y[0];
        M (1,1) = -image.orientation_y[1];
        M (2,1) = image.orientation_y[2];
        M (3,1) = 0.0;

        M (0,2) = -image.orientation_z[0];
        M (1,2) = -image.orientation_z[1];
        M (2,2) = image.orientation_z[2];
        M (3,2) = 0.0;

        M (0,3) = -image.position_vector[0];
        M (1,3) = -image.position_vector[1];
        M (2,3) = image.position_vector[2];
        M (3,3) = 1.0;

        if (image.images_in_mosaic) {
          INFO ("DICOM image \"" + H.name() + "\" is in mosaic format");
          if (H.dim (2) != 1)
            throw Exception ("DICOM mosaic contains multiple slices in image \"" + H.name() + "\"");

          if (image.dim[0] % image.acq_dim[0] || image.dim[1] % image.acq_dim[1])
            throw Exception ("acquisition matrix [ " + str (image.acq_dim[0]) + " " + str (image.acq_dim[1])
                             + " ] does not fit into DICOM mosaic [ " + str (image.dim[0]) + " " + str (image.dim[1])
                             + " ] in image \"" + H.name() + "\"");

          H.dim(0) = image.acq_dim[0];
          H.dim(1) = image.acq_dim[1];
          H.dim(2) = image.images_in_mosaic;

          handler = new MR::Image::Handler::Mosaic (H, image.dim[0], image.dim[1], H.dim (0), H.dim (1), H.dim (2));
        }
        else 
          handler = new MR::Image::Handler::Default (H);

        std::vector<const Image*> DW_scheme;

        for (size_t s = 0; s < series.size(); s++) {
          int index[3], n;
          for (index[2] = 0; index[2] < dim[2]; index[2]++) {
            for (index[0] = 0; index[0] < dim[0]; index[0]++) {
              n = index[0] + dim[0]*dim[1]*index[2];
              if (!isnan ( (*series[s]) [n]->bvalue)) DW_scheme.push_back ((*series[s])[n]);
              for (index[1] = 0; index[1] < dim[1]; index[1]++) {
                n = index[0] + dim[0]* (index[1] + dim[1]*index[2]);
                handler->files.push_back (File::Entry ( (*series[s]) [n]->filename, (*series[s]) [n]->data));
              }
            }
          }
        }


        if (DW_scheme.size()) {
          Math::Matrix<float>& G (H.DW_scheme());
          G.allocate (DW_scheme.size(), 4);
          bool is_not_GE = image.manufacturer.compare (0, 2, "GE");
          for (size_t s = 0; s < DW_scheme.size(); s++) {
            if ( (G (s,3) = DW_scheme[s]->bvalue) != 0.0) {
              float norm = Math::norm (DW_scheme[s]->G);
              G (s,3) *= (norm*norm);
              if (norm) {
                float d[] = { DW_scheme[s]->G[0]/norm, DW_scheme[s]->G[1]/norm, DW_scheme[s]->G[2]/norm };
                if (is_not_GE) {
                  G (s,0) = - d[0];
                  G (s,1) = - d[1];
                  G (s,2) = d[2];
                }
                else {
                  G (s,0) = M (0,0) *d[0] + M (0,1) *d[1] - M (0,2) *d[2];
                  G (s,1) = M (1,0) *d[0] + M (1,1) *d[1] - M (1,2) *d[2];
                  G (s,2) = M (2,0) *d[0] + M (2,1) *d[1] - M (2,2) *d[2];
                }
              }
              else G (s,0) = G (s,1) = G (s,2) = 0.0;
            }
            else G (s,0) = G (s,1) = G (s,2) = 0.0;
          }
        }

        return handler.release();
      }


    }
  }
}

