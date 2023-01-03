/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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

#include <numeric>
#include <tuple>
#include <iterator>
#include <set>
#include <algorithm>
#include <memory>

#include "file/config.h"
#include "file/ofstream.h"
#include "file/path.h"
#include "file/utils.h"
#include "file/mmap.h"
#include "image_helpers.h"
#include "header.h"
#include "formats/list.h"
#include "image_io/default.h"
#include "dwi/gradient.h"

namespace MR
{
  namespace Formats
  {


    typedef struct { NOMEMALIGN
      int sl, ec, dyn, ph, ty, seq, ang, pos, b, grad, asl, ri, rs, ss, pix, size, vox, thick, gap;
    } ParCols;



    inline const ParCols get_column_indices (const float version)
    {
      if (version == 3.0f) {
        return { 0, 1, 2, 3, 4, 5, -1, -1, -1, -1, -1, 7, 8, 9, -1, -1, -1, -1, -1 };
      }
      else if (version == 4.0f) {
        return { 0, 1, 2, 3, 4, 5, -1, -1, -1, -1, -1, 11, 12, 13, 7, -1, -1, -1, -1 };
      }
      else if (version == 4.1f) {
        return { 0, 1, 2, 3, 4, 5, 16, 19, 33, 45, -1, 11, 12, 13, 7, -1, 28, 22, 23 };
      }
      else if (version == 4.2f) {
        return { 0, 1, 2, 3, 4, 5, 16, 19, 33, 45, 48, 11, 12, 13, 7, 9, 28, 22, 23 };
      }
      else
        throw Exception ("unsupported version of PAR/REC: V" + str(version));
    }





    typedef struct { NOMEMALIGN
      int sl, ec, dyn, ph, ty, seq, asl, pix, size[2];
      float b, grad[3], ri, rs, ss, ang[3], pos[3], vox[2], thick, gap;
    } SliceData;


    inline const SliceData parse_line (const std::string& line, const ParCols& cols)
    {
      auto token = split (line, " \t\n", true);
      SliceData data;

      data.sl = to<int> (token[cols.sl]);
      data.ec = to<int> (token[cols.ec]);
      data.dyn = to<int> (token[cols.dyn]);
      data.ph = to<int> (token[cols.ph]);
      data.ty = to<int> (token[cols.ty]);
      data.seq = to<int> (token[cols.seq]);
      data.ri = cols.ri >= 0 ? to<float> (token[cols.ri]) : 0.0;
      data.rs = cols.rs >= 0 ? to<float> (token[cols.rs]) : 1.0;
      data.ss = cols.ss >= 0 ? to<float> (token[cols.ss]) : 1.0;
      data.thick = cols.thick >= 0 ? to<float> (token[cols.thick]) : NaN;
      data.gap = cols.gap >= 0 ? to<float> (token[cols.gap]) : NaN;

      if (cols.size >= 0) {
        data.size[0] = to<int> (token[cols.size]);
        data.size[1] = to<int> (token[cols.size+1]);
      }
      else {
        data.size[0] = data.size[1] = 0;
      }


      if (cols.vox >= 0) {
        data.vox[0] = to<float> (token[cols.vox]);
        data.vox[1] = to<float> (token[cols.vox+1]);
      }
      else {
        data.vox[0] = data.vox[1] = NaN;
      }

      data.b = cols.b >= 0 ? to<float> (token[cols.b]) : NaN;

      if (cols.ang >= 0) {
        data.ang[0] = to<float> (token[cols.ang]);
        data.ang[1] = to<float> (token[cols.ang+1]);
        data.ang[2] = to<float> (token[cols.ang+2]);
      }
      if (cols.pos >= 0) {
        data.pos[0] = to<float> (token[cols.pos]);
        data.pos[1] = to<float> (token[cols.pos+1]);
        data.pos[2] = to<float> (token[cols.pos+2]);
      }
      if (cols.grad >= 0) {
        data.grad[1] = to<float> (token[cols.grad]);
        data.grad[2] = -to<float> (token[cols.grad+1]);
        data.grad[0] = to<float> (token[cols.grad+2]);
      }

      data.pix = cols.pix >= 0 ? to<int> (token[cols.pix]) : 0;

      return data;
    }





    std::unique_ptr<ImageIO::Base> PAR::read (Header& H) const
    {
      if (!Path::has_suffix (H.name(), ".PAR") && !Path::has_suffix (H.name(), ".par"))
        return std::unique_ptr<ImageIO::Base>();

      WARN ("PAR/REC import is currently experimental - please verify the integrity of your data");
      WARN ("  If your data does not import correctly, please report it to the MRtrix3 team");

      std::string rec_file = H.name().substr(0,H.name().size()-4)+".REC";

      std::ifstream in (H.name(), std::ios::binary);
      if (!in)
        throw Exception ("error opening PAR/REC header \"" + H.name() + "\": " + strerror (errno));

      float version = NaN;
      ParCols cols;
      vector<SliceData> slices;

      while (in.good()) {
        std::string line;
        std::getline (in, line);
        line = strip (line);

        if (line.empty())
          continue;

        if (line[0] == '#') { // comment
          auto pos = line.find ("Research image export tool");
          if (pos < line.size()) {
            version = to<float> (split (line.substr(pos), " \t\n", true)[4].substr (1));
            cols = get_column_indices (version);
          }
        }
        else if (line[0] == '.') { // definition
          line = strip (line.substr(1));
          auto keyval = split (line, ":", false, 2);
          keyval[0] = strip (keyval[0]);

          if (keyval[0] == "Patient name")
            add_line (H.keyval()["comments"], "Name: " + strip (keyval[1]));
          else if (keyval[0] == "Examination name")
            add_line (H.keyval()["comments"], "Exam: " + strip (keyval[1]));
          else if (keyval[0] == "Protocol name")
            add_line (H.keyval()["comments"], "Protocol: " + strip (keyval[1]));
          else if (keyval[0] == "Examination date/time")
            add_line (H.keyval()["comments"], "date: " + strip (keyval[1]));


        }
        else { // per-slice info
          if (std::isnan (version))
            throw Exception ("no version information found in file \"" + H.name() + "\"");

          slices.push_back (parse_line (line, cols));
        }
      }

      vector<std::array<float,4>> G;

      int nslices = 0;
      int nvols = 0;
      for (const auto& slice : slices) {
        if (slice.seq != slices[0].seq)
          throw Exception ("non-matching orientations in PAR/REC file \"" + H.name() + "\"");

        if (slice.pix != slices[0].pix)
          throw Exception ("non-matching bit depths in PAR/REC file \"" + H.name() + "\"");

        if (slice.size[0] != slices[0].size[0] || slice.size[1] != slices[0].size[1])
          throw Exception ("non-matching matrix sizes in PAR/REC file \"" + H.name() + "\"");

        if (slice.rs != slices[0].rs || slice.ri != slices[0].ri || slice.ss != slices[0].ss)
          throw Exception ("non-matching rescale coefficients in PAR/REC file \"" + H.name() + "\"");

        if (slice.thick != slices[0].thick)
          throw Exception ("non-matching slice thicknesses in PAR/REC file \"" + H.name() + "\"");

        if (slice.gap != slices[0].gap)
          throw Exception ("non-matching slice gaps in PAR/REC file \"" + H.name() + "\"");

        if (slice.vox[0] != slices[0].vox[0] && slice.vox[1] != slices[0].vox[1])
          throw Exception ("non-matching voxel sizes in PAR/REC file \"" + H.name() + "\"");

        if (slice.sl == 1) {
          nvols++;
          if (std::isfinite (slice.b))
            G.push_back ({ slice.grad[0], slice.grad[1], slice.grad[2], slice.b });
        }

        if (slice.sl > nslices)
          nslices = slice.sl;
      }

      if (size_t (nvols*nslices) != slices.size())
        throw Exception ("mismatch in dimensions when reading PAR/REC file \"" + H.name() + "\"");

      if (nvols > 1) {
        H.ndim() = 4;
        H.size(3) = nvols;
        H.stride(3) = 4;
      }
      else {
        H.ndim() = 3;
      }
      H.size(0) = slices[0].size[0];
      H.size(1) = slices[0].size[1];
      H.size(2) = nslices;

      H.spacing(0) = slices[0].vox[0];
      H.spacing(1) = slices[0].vox[1];
      H.spacing(2) = slices[0].thick + slices[0].gap;

      if (slices[0].gap > 0.0)
        WARN ("slice gap detected in PAR/REC file \"" + H.name() + "\"");


      H.stride(0) = -1;
      H.stride(1) = -2;
      H.stride(2) = 3;

      if (nvols > 1 && slices[0].sl == slices[1].sl) {
        H.stride(2) = 4;
        H.stride(3) = 3;
      }

      H.datatype() = slices[0].pix == 16 ? DataType::UInt16LE : DataType::UInt8;

      H.set_intensity_scaling (1.0/slices[0].ss, slices[0].ri/(slices[0].rs*slices[0].ss));

      if (cols.ang >= 0 && cols.pos >= 0) {

        constexpr double D2R = Math::pi/180.0;
        transform_type M;
        M =
          Eigen::AngleAxis<double> (D2R*slices[0].ang[2], Eigen::Vector3d (-1.0, 0.0, 0.0)) *
          Eigen::AngleAxis<double> (D2R*slices[0].ang[1], Eigen::Vector3d (0.0, 0.0, 1.0)) *
          Eigen::AngleAxis<double> (D2R*slices[0].ang[0], Eigen::Vector3d (0.0, -1.0, 0.0));


	if (slices[0].seq == 2) { // sagittal
          M = M * Eigen::AngleAxis<double> (Math::pi/2.0, Eigen::Vector3d (1.0, 0.0, 0.0))
		* Eigen::AngleAxis<double> (Math::pi/2.0, Eigen::Vector3d (0.0, 1.0, 0.0));
	}
	else if (slices[0].seq == 3) { // coronal
          throw Exception ("Images detected in coronal orientation - not yet supported. Please contact MRtrix3 team for support");
	}

        Eigen::Vector3d p (-slices[0].pos[2], -slices[0].pos[0], slices[0].pos[1]);
        p -= M * Eigen::Vector3d (((H.size(0)-1)*H.spacing(0))/2.0, ((H.size(1)-1)*H.spacing(1))/2.0, 0.0);
        M.pretranslate (p);

        H.transform() = M;
      }

      if (G.size()) {
        if (G.size() != size_t(nvols))
          throw Exception ("mismatch between number of volumes and number of b-values in PAR/REC file \"" + H.name() + "\"");

        Eigen::MatrixXd grad (G.size(), 4);
        for (size_t n = 0; n < G.size(); ++n) {
          grad(n,0) = G[n][0];
          grad(n,1) = G[n][1];
          grad(n,2) = G[n][2];
          grad(n,3) = G[n][3];
        }

        DWI::set_DW_scheme (H, grad);
      }


      std::unique_ptr<ImageIO::Base> io_handler (new ImageIO::Default (H));
      io_handler->files.push_back (File::Entry (rec_file));

      return io_handler;
    }


    bool PAR::check (Header& H, size_t num_axes) const
    {
      return false;
    }

    std::unique_ptr<ImageIO::Base> PAR::create (Header& H) const
    {
      assert (0);
      return std::unique_ptr<ImageIO::Base>();
    }

  }
}



