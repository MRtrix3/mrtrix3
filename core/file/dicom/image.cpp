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

#include "exception.h"
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
        if (item.ignore_when_parsing())
          return;

        switch (item.group) {
          case 0x0008U:
            switch (item.element) {
              case 0x0070U:
                manufacturer = item.get_string()[0];
                return;
              case 0x0008U:
                image_type = join (item.get_string(), " ");
                return;
              case 0x0032U:
                acquisition_time = item.get_time();
                return;
              default:
                return;
            }

          case 0x0018U:
            switch (item.element) {
              case 0x0024U:
                sequence_name = item.get_string (0);
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
              case 0x0050U:
                slice_thickness = item.get_float (0, slice_thickness);
                return;
              case 0x0080U:
                repetition_time = item.get_float (0, repetition_time);
                return;
              case 0x0081U:
                echo_time = item.get_float (0, echo_time);
                return;
              case 0x0082U:
                inversion_time = item.get_float (0, inversion_time);
                return;
              case 0x0086U:
                echo_index = item.get_int (0, echo_index);
                return;
              case 0x0088U:
                slice_spacing = item.get_float (0, slice_spacing);
                return;
              case 0x0091U:
                echo_train_length = item.get_int (0, echo_train_length);
                return;
              case 0x0095U:
                pixel_bandwidth = item.get_float (0, pixel_bandwidth);
                return;
              case 0x1310U:
                {
                  auto d = item.get_uint();
                  item.check_size (d, 4);
                  acq_dim[0] = std::max (d[0], d[1]);
                  acq_dim[1] = std::max (d[2], d[3]);
                  if (d[0] == 0 && d[3] == 0)
                    std::swap (acq_dim[0], acq_dim[1]);
                }
                return;
              case 0x1312U:
                if (item.get_string (0) == "ROW")
                  pe_axis = 0;
                else if (item.get_string (0) == "COL")
                  pe_axis = 1;
                return;
              case 0x1314U:
                flip_angle = item.get_float (0, flip_angle);
                return;
              case 0x9074U:
                acquisition_time = item.get_datetime().second;
                return;
              case 0x9082U:
                echo_time = item.get_float (0, echo_time);
                return;
              case 0x9087U:
                { // ugly hack to handle badly formatted Philips data:
                  default_type v = item.get_float (0, bvalue);
                  if (v < 1.0e10)
                    bvalue = v;
                }
                return;
              case 0x9089U:
                { // ugly hack to handle badly formatted Philips data:
                  default_type v = item.get_float (0, G[0]);
                  if (v < 1.0e10) {
                    G[0] = v;
                    G[1] = item.get_float (1, G[1]);
                    G[2] = item.get_float (2, G[2]);
                  }
                }
            }
            return;
          case 0x0019U:
            switch (item.element) { // GE DW encoding info:
              case 0x10BBU:
                G[0] = item.get_float (0, G[0]);
                return;
              case 0x10BCU:
                G[1] = item.get_float (0, G[1]);
                return;
              case 0x10BDU:
                G[2] = item.get_float (0, G[2]);
                return;
              case 0x100CU: //Siemens private DW encoding tags:
                bvalue = item.get_float (0, bvalue);
                return;
              case 0x100EU:
                {
                  auto d = item.get_float();
                  if (d.size() >= 3) {
                    G[0] = d[0];
                    G[1] = d[1];
                    G[2] = d[2];
                  }
                }
                return;
            }
            return;
          case 0x0020U:
            switch (item.element) {
              case 0x000EU:
                ignore_series_num = item.is_in_series_ref_sequence();
                return;
              case 0x0011U:
                series_num = item.get_uint (0, series_num);
                return;
              case 0x0012U:
                acq = item.get_uint (0, acq);
                return;
              case 0x0013U:
                instance = item.get_uint (0, instance);
                return;
              case 0x0032U:
                {
                  auto d = item.get_float();
                  item.check_size (d, 3);
                  position_vector[0] = d[0];
                  position_vector[1] = d[1];
                  position_vector[2] = d[2];
                }
                return;
              case 0x0037U:
                {
                  auto d = item.get_float();
                  item.check_size (d, 6);
                  orientation_x[0] = d[0];
                  orientation_x[1] = d[1];
                  orientation_x[2] = d[2];
                  orientation_y[0] = d[3];
                  orientation_y[1] = d[4];
                  orientation_y[2] = d[5];
                  orientation_x.normalize();
                  orientation_y.normalize();
                }
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
              case 0x0002U:
                samples_per_pixel = item.get_uint (0, samples_per_pixel);
                return;
              case 0x0010U:
                dim[1] = item.get_uint (0, dim[1]);
                return;
              case 0x0011U:
                dim[0] = item.get_uint (0, dim[0]);
                return;
              case 0x0030U:
                {
                  auto d = item.get_float();
                  item.check_size (d, 2);
                  pixel_size[0] = d[0];
                  pixel_size[1] = d[1];
                }
                return;
              case 0x0100U:
                bits_alloc = item.get_uint (0, bits_alloc);
                return;
              case 0x1052U:
                scale_intercept = item.get_float (0, scale_intercept);
                return;
              case 0x1053U:
                scale_slope = item.get_float (0, scale_slope);
                return;
            }
            return;
          case 0x0029U: // Siemens CSA entry
            if (item.element == 0x1010U ||
                item.element == 0x1020U ||
                item.element == 0x1110U ||
                item.element == 0x1120U ||
                item.element == 0x1210U ||
                item.element == 0x1220U) {
              decode_csa (item.data, item.data + item.size);
            }
            return;
          case 0x0043U: // GEMS_PARMS_01 block
            if (item.element == 0x1039U) {
              if (item.get_int().size())
                bvalue = item.get_int()[0];
              DW_scheme_wrt_image = true;
            }
            return;
          case 0x2001U: // Philips DW encoding info:
            switch (item.element) {
              case 0x1003:
                bvalue = item.get_float (0, bvalue);
                return;
              case 0x1004:
                philips_orientation = item.get_string(0, "\0")[0];
                return;
            }
            return;
          case 0x2005U: // Philips DW encoding info:
            switch (item.element) {
              case 0x10B0U:
                G[0] = item.get_float (0, G[0]);
                return;
              case 0x10B1U:
                G[1] = item.get_float (0, G[1]);
                return;
              case 0x10B2U:
                G[2] = item.get_float (0, G[2]);
                return;
              case 0x1413:
                grad_number = item.get_int()[0];
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
          case 0xFFFEU:
            switch (item.element) {
              case 0xE000U:
                if (item.parents.size() &&
                    item.parents.back().group ==  0x5200U &&
                    item.parents.back().element == 0x9230U) { // multi-frame item
                  if (in_frames) {
                    calc_distance();
                    frames.push_back (std::shared_ptr<Frame> (new Frame (*this)));
                    frame_offset += dim[0] * dim[1] * (bits_alloc/8) * samples_per_pixel;
                  }
                  else
                    in_frames = true;
                }
                return;
            }
            return;
        }


      }






      void Image::read ()
      {
        {
          Element item;
          item.set (filename);

          while (item.read()){
            try {
              parse_item (item);
            }
            catch (Exception& E) {
              DEBUG (printf ("error reading tag (%04X,%04X):", item.group, item.element));
              E.display(3);
            }
          }

          calc_distance();

          if (frame_offset > 0)
            frames.push_back (std::shared_ptr<Frame> (new Frame (*this)));

          for (size_t n = 0; n < frames.size(); ++n)
            frames[n]->data = data + frames[n]->frame_offset;
        }
        check_app_exit_code();
      }






      namespace {
        template <typename T, class Functor>
          void phoenix_scalar (const KeyValues& keyval, const std::string& key, const Functor& functor, T& field) {
            const auto it = keyval.find (key);
            if (it == keyval.end())
              return;
            field = functor (it->second);
          }
        template <typename T>
          void phoenix_vector (const KeyValues& keyval, const std::string& key, vector<T>& data) {
            data.clear();
            for (size_t index = 0; ; ++index) {
              const auto it = keyval.find (key + "[" + str(index) + "]");
              if (it == keyval.end())
                return;
              data.push_back (to<T> (it->second));
            }
          }
      }


      void Image::decode_csa (const uint8_t* start, const uint8_t* end)
      {
        CSAEntry entry (start, end);

        while (entry.parse()) {
          if (strcmp ("B_value", entry.key()) == 0)
            bvalue = entry.get_float();
          else if (strcmp ("BandwidthPerPixelPhaseEncode", entry.key()) == 0)
            bandwidth_per_pixel_phase_encode = entry.get_float();
          else if (strcmp ("DiffusionGradientDirection", entry.key()) == 0)
            entry.get_float (G);
          else if (strcmp ("ImageOrientationPatient", entry.key()) == 0) {
            Eigen::Matrix<default_type,6,1> v;
            entry.get_float (v);
            if (v.allFinite()) {
              orientation_x = v.head(3);
              orientation_y = v.tail(3);
              orientation_x.normalize();
              orientation_y.normalize();
            }
          }
          else if (strcmp ("ImagePositionPatient", entry.key()) == 0) {
            Eigen::Matrix<default_type,3,1> v;
            entry.get_float (v);
            if (v.allFinite())
              position_vector = v;
          }
          else if (strcmp ("MosaicRefAcqTimes", entry.key()) == 0) {
            mosaic_slices_timing.resize (entry.num_items());
            entry.get_float (mosaic_slices_timing);
          }
          else if (strcmp ("MrPhoenixProtocol", entry.key()) == 0) {
            const vector<std::string> phoenix = entry.get_string();
            const auto keyval = read_csa_ascii (phoenix);
            phoenix_scalar (keyval,
                "sDiffusion.dsScheme",
                [] (const std::string& value) -> size_t { return to<size_t> (value); },
                bipolar_flag);
            phoenix_scalar (keyval,
                "sKSpace.ucPhasePartialFourier",
                [] (const std::string& value) -> default_type
                {
                switch (to<size_t> (value)) {
                case 1: return 0.5;
                case 2: return 0.675;
                case 4: return 0.75;
                case 8: return 0.875;
                default: return NaN;
                }
                },
                partial_fourier);
            phoenix_scalar (keyval,
                "ucReadOutMode",
                [] (const std::string& value) -> size_t { return to<size_t> (value); },
                readoutmode_flag);
            phoenix_vector (keyval,
                "adFlipAngleDegree",
                flip_angles);

          }
          else if (strcmp ("NumberOfImagesInMosaic", entry.key()) == 0)
            images_in_mosaic = entry.get_int();
          else if (strcmp ("PhaseEncodingDirectionPositive", entry.key()) == 0)
            pe_sign = (entry.get_int() > 0) ? 1 : -1;
          else if (strcmp ("SliceNormalVector", entry.key()) == 0)
            entry.get_float (orientation_z);
          else if (strcmp ("TimeAfterStart", entry.key()) == 0)
            time_after_start = entry.get_float();
        }

        if (G[0] && bvalue)
          if (fabs(G[0]) > 1.0 && fabs(G[1]) > 1.0 && fabs(G[2]) > 1.0)
            bvalue = G[0] = G[1] = G[2] = 0.0;
      }



      KeyValues Image::read_csa_ascii (const vector<std::string>& data)
      {

        auto split_keyval = [] (const std::string& s) -> std::pair<std::string, std::string> {
          const size_t delimiter = s.find_first_of ("=");
          if (delimiter == std::string::npos)
            return std::make_pair (std::string(), std::string());
          return std::make_pair (strip (s.substr(0, delimiter)), strip (s.substr(delimiter+1)) );
        };

        KeyValues result;
        for (const auto& item : data) {
          const auto lines = split_lines (item);
          bool do_read = false;
          for (const auto& line : lines) {
            if (line.substr(0, 17) == "### ASCCONV BEGIN") {
              do_read = true;
              continue;
            }
            if (line.substr(0, 15) == "### ASCCONV END") {
              do_read = false;
              continue;
            }
            if (do_read) {
              const auto keyval = split_keyval (line);
              result[keyval.first] = keyval.second;
            }
          }
          if (do_read) {
            WARN("Siemens CSA ASCII section malformed (not appropriately ended)");
          }
        }
        return result;
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
        if (std::isfinite (item.bvalue)) {
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

        inline void update_count (size_t num, vector<size_t>& dim, vector<size_t>& index)
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


      vector<size_t> Frame::count (const vector<Frame*>& frames)
      {
        vector<size_t> dim (3, 0);
        vector<size_t> index (3, 1);
        const Frame* previous = frames[0];

        for (auto frame_it = frames.cbegin()+1; frame_it != frames.cend(); ++frame_it) {
          const Frame& frame (**frame_it);

          if ((!frame.ignore_series_num && frame.series_num != previous->series_num ) ||
              frame.acq != previous->acq || frame.distance < previous->distance)
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





      default_type Frame::get_slice_separation (const vector<Frame*>& frames, size_t nslices)
      {
        default_type max_gap = 0.0;
        default_type min_separation = std::numeric_limits<default_type>::infinity();
        default_type max_separation = 0.0;
        default_type sum_separation = 0.0;

        if (nslices < 2)
          return std::isfinite (frames[0]->slice_spacing) ?
            frames[0]->slice_spacing : frames[0]->slice_thickness;

        for (size_t n = 0; n < nslices-1; ++n) {
          const default_type separation = frames[n+1]->distance - frames[n]->distance;
          const default_type gap = abs (separation - frames[n]->slice_thickness);
          max_gap = std::max (gap, max_gap);
          min_separation = std::min (min_separation, separation);
          max_separation = std::max (max_separation, separation);
          sum_separation += separation;
        }

        if (max_gap > 1e-4)
          WARN ("slice gap detected (maximum gap: " + str(max_gap, 3) + "mm)");
        if (max_separation - min_separation > 2e-4)
          WARN ("slice separation is not constant (from " + str(min_separation, 8) + " to " + str(max_separation, 8) + "mm)");

        return (sum_separation / default_type(nslices-1));
      }





      std::string Frame::get_DW_scheme (const vector<Frame*>& frames, const size_t nslices, const transform_type& image_transform)
      {
        if (!std::isfinite (frames[0]->bvalue)) {
          DEBUG ("no DW encoding information found in DICOM frames");
          return { };
        }

        std::string dw_scheme;
        const size_t nDW = frames.size() / nslices;

        const bool rotate_DW_scheme = frames[0]->DW_scheme_wrt_image;
        for (size_t n = 0; n < nDW; ++n) {
          const Frame& frame (*frames[n*nslices]);
          std::array<default_type,4> g = {{ 0.0, 0.0, 0.0, frame.bvalue }};
          if (g[3] && std::isfinite (frame.G[0]) && std::isfinite (frame.G[1]) && std::isfinite (frame.G[2])) {

            if (rotate_DW_scheme) {
              g[0] = image_transform(0,0)*frame.G[0] + image_transform(0,1)*frame.G[1] - image_transform(0,2)*frame.G[2];
              g[1] = image_transform(1,0)*frame.G[0] + image_transform(1,1)*frame.G[1] - image_transform(1,2)*frame.G[2];
              g[2] = image_transform(2,0)*frame.G[0] + image_transform(2,1)*frame.G[1] - image_transform(2,2)*frame.G[2];
            } else {
              g[0] = -frame.G[0];
              g[1] = -frame.G[1];
              g[2] =  frame.G[2];
            }

          }
          add_line (dw_scheme, str(g[0]) + "," + str(g[1]) + "," + str(g[2]) + "," + str(g[3]));
        }

        return dw_scheme;
      }



      Eigen::MatrixXd Frame::get_PE_scheme (const vector<Frame*>& frames, const size_t nslices)
      {
        const size_t num_volumes = frames.size() / nslices;
        Eigen::MatrixXd pe_scheme = Eigen::MatrixXd::Zero (num_volumes, 4);

        for (size_t n = 0; n != num_volumes; ++n) {
          const Frame& frame (*frames[n*nslices]);
          if (frame.pe_axis == 3 || !frame.pe_sign) {
            DEBUG ("no phase-encoding information found in DICOM frames");
            return { };
          }
          pe_scheme (n, frame.pe_axis) = frame.pe_sign;
          if (std::isfinite (frame.bandwidth_per_pixel_phase_encode)) {
            const default_type effective_echo_spacing = 1.0 / (frame.bandwidth_per_pixel_phase_encode * frame.acq_dim[frame.pe_axis]);
            pe_scheme(n, 3) = effective_echo_spacing * (frame.acq_dim[frame.pe_axis] - 1);
          }
        }

        if (pe_scheme.col(3).sum())
          return pe_scheme;
        return pe_scheme.leftCols(3);
      }



    }
  }
}

