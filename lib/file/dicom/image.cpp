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

#include "file/path.h"
#include "file/dicom/image.h"
#include "file/dicom/series.h"
#include "file/dicom/study.h"
#include "file/dicom/patient.h"
#include "file/dicom/csa_entry.h"

namespace MR {
  namespace File {
    namespace Dicom {



      void Image::parse_item (Element& item, const std::string& dirname)
      {
        // is this tag top-level or per-frame:
        bool is_toplevel = (item.level() == 0);
        for (size_t n = 0; n < item.level(); ++n) {
          if (item.parents[n].group == 0x5200U && (
                item.parents[n].element == 0x9230U ||  // per-frame tag
                item.parents[n].element == 0x9229U)) { // shared across frames tag
            is_toplevel = true; 
            break;
          }
        }



        // process image-specific or per-frame items here:
        if (is_toplevel) {
          switch (item.group) {
            case 0x0018U: 
              switch (item.element) {
                case 0x0050U: 
                  slice_thickness = item.get_float()[0]; 
                  return;
                case 0x0088U:
                  slice_spacing = item.get_float()[0];
                  return;
                case 0x1310U: 
                  acq_dim[0] = std::max (item.get_uint()[0], item.get_uint()[1]);
                  acq_dim[1] = std::max (item.get_uint()[2], item.get_uint()[3]);
                  return;
                case 0x0024U: 
                  sequence_name = item.get_string()[0];
                  if (!sequence_name.size())
                    return;
                  { 
                    int c = sequence_name.size()-1;
                    if (!isdigit (sequence_name[c]))
                      return;
                    while (c >= 0 && isdigit (sequence_name[c])) 
                      --c;
                    ++c;
                    sequence = to<size_t> (sequence_name.substr (c));
                  }
                  return;
                case 0x9087U: 
                  bvalue = item.get_float()[0]; 
                  return;
                case 0x9089U:
                  G[0] = item.get_float()[0];
                  G[1] = item.get_float()[1];
                  G[2] = item.get_float()[2];
                  return;
              }
              return;
            case 0x0020U: 
              switch (item.element) {
                case 0x0011U: 
                  series_num = item.get_uint()[0]; 
                  return;
                case 0x0012U:
                  acq = item.get_uint()[0]; 
                  return;
                case 0x0013U: 
                  instance = item.get_uint()[0]; 
                  return;
                case 0x0032U:
                  position_vector[0] = item.get_float()[0];
                  position_vector[1] = item.get_float()[1];
                  position_vector[2] = item.get_float()[2];
                  return;
                case 0x0037U: 
                  orientation_x[0] = item.get_float()[0];
                  orientation_x[1] = item.get_float()[1];
                  orientation_x[2] = item.get_float()[2];
                  orientation_y[0] = item.get_float()[3];
                  orientation_y[1] = item.get_float()[4];
                  orientation_y[2] = item.get_float()[5];
                  Math::normalise (orientation_x);
                  Math::normalise (orientation_y);
                  return;
                case 0x9157U: 
                  index = item.get_uint();
                  if (frame_dim.size() < index.size())
                    frame_dim.resize (index.size());
                  for (size_t n = 0; n < index.size(); ++n)
                    if (frame_dim[n] < index[n])
                      frame_dim[n] = index[n];
                  return;
              }
              return;
            case 0x0028U:
              switch (item.element) {
                case 0x0010U: 
                  dim[1] = item.get_uint()[0]; 
                  return;
                case 0x0011U:
                  dim[0] = item.get_uint()[0]; 
                  return;
                case 0x0030U:
                  pixel_size[0] = item.get_float()[0];
                  pixel_size[1] = item.get_float()[1]; 
                  return;
                case 0x0100U: 
                  bits_alloc = item.get_uint()[0]; 
                  return;
                case 0x1052U:
                  scale_intercept = item.get_float()[0]; 
                  return;
                case 0x1053U:
                  scale_slope = item.get_float()[0]; 
                  return;
              }
              return;
            case 0xFFFEU: 
              switch (item.element) {
                case 0xE000U:
                  if (item.parents.back().group ==  0x5200U &&
                      item.parents.back().element == 0x9230U) { // multi-frame item
                    if (in_frames) {
                      calc_distance();
                      frames.push_back (RefPtr<Frame> (new Frame (*this)));
                      frame_offset += dim[0] * dim[1] * (bits_alloc/8);
                    }
                    else 
                      in_frames = true;
                  }
                  return;
              }
              return;
            case 0x7FE0U: 
              if (item.element == 0x0010U) {
                data = item.offset (item.data);
                data_size = item.size;
                is_BE = item.is_big_endian();
                return;
              }
              return;

          }

        }




        // process more non-specific stuff here:

        switch (item.group) {
          case 0x0008U: 
            if (item.element == 0x0070U) 
              manufacturer = item.get_string()[0];
            return;
          case 0x0019U: 
            switch (item.element) { // GE DW encoding info:
              case 0x10BBU:
                if (item.get_float().size()) 
                  G[0] = item.get_float()[0]; 
                return;
              case 0x10BCU:
                if (item.get_float().size()) 
                  G[1] = item.get_float()[0]; 
                return;
              case 0x10BDU:
                if (item.get_float().size()) 
                  G[2] = item.get_float()[0]; 
                return;
              case 0x100CU: //Siemens private DW encoding tags:
                if (item.get_int().size()) 
                  bvalue = item.get_int()[0]; 
                return;
              case 0x100EU: 
                if (item.get_float().size() == 3) {
                  G[0] = item.get_float()[0];
                  G[1] = item.get_float()[1];
                  G[2] = item.get_float()[2];
                }
                return;
            }
            return;
          case 0x0029U: // Siemens CSA entry
            if (item.element == 0x1010U || item.element == 0x1020U) 
              decode_csa (item.data, item.data + item.size);
            return;
          case 0x0043U: // GEMS_PARMS_01 block
            if (item.element == 0x1039U) {
              if (item.get_int().size()) 
                bvalue = item.get_int()[0];
              DW_scheme_wrt_image = true;
            }
            return;
          case 0x2001U: // Philips DW encoding info: 
            if (item.element == 0x1003) 
              bvalue = item.get_float()[0];
            return;
          case 0x2005U: // Philips DW encoding info: 
            switch (item.element) {
              case 0x10B0U: 
                G[0] = item.get_float()[0]; 
                return;
              case 0x10B1U:
                G[1] = item.get_float()[0]; 
                return;
              case 0x10B2U:
                G[2] = item.get_float()[0]; 
                return;
            }
            return;
        }


      }






      void Image::read ()
      {
        Element item;
        item.set (filename);

        while (item.read()) 
          parse_item (item);

        calc_distance();

        if (frame_offset > 0)
          frames.push_back (RefPtr<Frame> (new Frame (*this)));

        for (size_t n = 0; n < frames.size(); ++n) 
          frames[n]->data = data + frames[n]->frame_offset;
      }









      void Image::decode_csa (const uint8_t* start, const uint8_t* end)
      {
        CSAEntry entry (start, end);

        while (entry.parse()) {
          if (strcmp ("B_value", entry.key()) == 0) 
            bvalue = entry.get_float();
          else if (strcmp ("DiffusionGradientDirection", entry.key()) == 0) 
            entry.get_float (G);
          else if (strcmp ("NumberOfImagesInMosaic", entry.key()) == 0) 
            images_in_mosaic = entry.get_int();
          else if (strcmp ("SliceNormalVector", entry.key()) == 0) 
            entry.get_float (orientation_z);
        }

        if (G[0] && bvalue)
          if (fabs(G[0]) > 1.0 && fabs(G[1]) > 1.0 && fabs(G[2]) > 1.0)
            bvalue = G[0] = G[1] = G[2] = 0.0;

      }



      std::ostream& operator<< (std::ostream& stream, const Frame& item)
      {
        stream << ( item.instance == UINT_MAX ? 0 : item.instance ) << "#" 
          << ( item.acq == UINT_MAX ? 0 : item.acq) << ":"
          << ( item.sequence == UINT_MAX ? 0 : item.sequence ) << " " 
          << item.dim[0] << "x" << item.dim[1] << ", "
          << item.pixel_size[0] << "x" << item.pixel_size[1] << " x " 
          << item.slice_thickness << " (" << item.slice_spacing << ") mm, z = " << item.distance 
          << ( item.index.size() ? ", index = " + str(item.index) : std::string() ) << ", [ "
          << item.position_vector[0] << " " << item.position_vector[1] << " " << item.position_vector[2] << " ] [ "
          << item.orientation_x[0] << " " << item.orientation_x[1] << " " << item.orientation_x[2] << " ] [ "
          << item.orientation_y[0] << " " << item.orientation_y[1] << " " << item.orientation_y[2] << " ]";
        if (isfinite (item.bvalue)) {
          stream << ", b = " << item.bvalue;
          if (item.bvalue > 0.0)
            stream << ", G = [ " << item.G[0] << " " << item.G[1] << " " << item.G[2] << " ]";
        }
        stream << " (\"" << item.filename << "\", " << item.data << ")";


        return stream;
      }



      std::ostream& operator<< (std::ostream& stream, const Image& item)
      {
        stream << ( item.filename.size() ? item.filename : "file not set" ) << ":\n" 
          << ( item.sequence_name.size() ? item.sequence_name : "sequence not set" ) << " [" 
          << (item.manufacturer.size() ? item.manufacturer : std::string("unknown manufacturer")) << "] "
          << (item.frames.size() > 0 ? str(item.frames.size()) + " frames with dim " + str(item.frame_dim) : std::string());
        if (item.frames.size()) {
          for (size_t n = 0; n < item.frames.size(); ++n)
            stream << "  " << static_cast<const Frame&>(*item.frames[n]) << "\n";
        }
        else 
          stream << "  " << static_cast<const Frame&>(item) << "\n";

        return stream;
      }






      namespace {

        inline void update_count (size_t num, std::vector<size_t>& dim, std::vector<size_t>& index)
        {
          for (size_t n = 0; n < num; ++n) {
            if (dim[n] && index[n] != dim[n])
              throw Exception ("dimensions mismatch in DICOM series");
            index[n] = 1;
          }
          ++index[num];
          dim[num] = index[num];
        }

      }


      std::vector<size_t> Frame::count (const std::vector<Frame*>& frames) 
      {
        std::vector<size_t> dim (3, 0);
        std::vector<size_t> index (3, 1);
        const Frame* previous = frames[0];

        for (std::vector<Frame*>::const_iterator frame_it = frames.begin()+1; frame_it != frames.end(); ++frame_it) {
          const Frame& frame (**frame_it);

          if (frame.series_num != previous->series_num ||
              frame.acq != previous->acq) 
            update_count (2, dim, index);
          else if (frame.distance != previous->distance) 
            update_count (1, dim, index);
          else 
            update_count (0, dim, index);

          previous = &frame;

        }

        if (!dim[0]) dim[0] = 1;
        if (!dim[1]) dim[1] = 1;
        if (!dim[2]) dim[2] = 1;

        return dim;
      }





      float Frame::get_slice_separation (const std::vector<Frame*>& frames, size_t nslices)
      {
        bool slicesep_warning_issued = false;
        bool slicegap_warning_issued = false;

        if (nslices < 2) 
          return isfinite (frames[0]->slice_spacing) ? 
            frames[0]->slice_spacing : frames[0]->slice_thickness;

        float slice_separation = NAN;
        for (size_t n = 0; n < nslices-1; ++n) {
          float current_slice_separation = frames[n+1]->distance - frames[n]->distance;
          if (!isfinite (slice_separation)) {
            slice_separation = current_slice_separation;
            continue;
          }

          if (!slicegap_warning_issued) {
            if (Math::abs (current_slice_separation - frames[n]->slice_thickness) > 1e-4) {
              WARN ("slice gap detected");
              slicegap_warning_issued = true;
              slice_separation = current_slice_separation;
            }
          }

          if (!slicesep_warning_issued) {
            if (Math::abs (current_slice_separation - slice_separation) > 1e-4) {
              slicesep_warning_issued = true;
              WARN ("slice separation is not constant");
            }
          }
        }

        return slice_separation;
      }





      Math::Matrix<float> Frame::get_DW_scheme (const std::vector<Frame*>& frames, size_t nslices, const Math::Matrix<float>& image_transform)
      {
        Math::Matrix<float> G;

        if (!isfinite (frames[0]->bvalue)) {
          DEBUG ("no DW encoding information found in DICOM frames");
          return G;
        }

        const size_t nDW = frames.size() / nslices;

        G.allocate (nDW, 4);
        const bool rotate_DW_scheme = frames[0]->DW_scheme_wrt_image;
        for (size_t n = 0; n < nDW; ++n) {
          const Frame& frame (*frames[n*nslices]);

          G(n,3) = frame.bvalue;
          G(n,0) = G(n,1) = G(n,2) = 0.0;

          if (G(n,3)) {
            float norm = Math::norm (frame.G);
            G(n,3) *= norm;
            if (norm) {
              float d[] = { 
                frame.G[0]/norm, 
                frame.G[1]/norm, 
                frame.G[2]/norm 
              };
              if (rotate_DW_scheme) {
                G(n,0) = image_transform(0,0)*d[0] + image_transform(0,1)*d[1] - image_transform(0,2)*d[2];
                G(n,1) = image_transform(1,0)*d[0] + image_transform(1,1)*d[1] - image_transform(1,2)*d[2];
                G(n,2) = image_transform(2,0)*d[0] + image_transform(2,1)*d[1] - image_transform(2,2)*d[2];
              }
              else { 
                G(n,0) = -d[0];
                G(n,1) = -d[1];
                G(n,2) =  d[2];
              }
            }
          }

        }

        return G;
      }


    }
  }
}

