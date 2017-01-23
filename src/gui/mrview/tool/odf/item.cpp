/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */

#include "gui/mrview/tool/odf/item.h"

#include "header.h"
#include "dwi/gradient.h"
#include "dwi/shells.h"
#include "math/SH.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {


        ODF_Item::ODF_Item (MR::Header&& H, const odf_type_t type, const float scale, const bool hide_negative, const bool color_by_direction) :
            image (std::move (H)),
            odf_type (type),
            lmax (odf_type == odf_type_t::SH ? Math::SH::LforN (image.header().size (3)) : -1),
            scale (scale),
            hide_negative (hide_negative),
            color_by_direction (color_by_direction),
            dixel (odf_type == odf_type_t::DIXEL ? new DixelPlugin (image.header()) : nullptr)
        {
          if (!dixel)
            return;

          // If dixel image is opened, try to intelligently determine the
          //   appropriate source of direction information
          try {
            if (!dixel->shells)
              throw Exception ("No shell data");
            dixel->set_shell (dixel->shells->count() - 1);
            DEBUG ("Image " + image.header().name() + " initialised as dixel ODF using DW scheme");
          } catch (...) {
            try {
              dixel->set_header();
              DEBUG ("Image " + image.header().name() + " initialised as dixel ODF using header directions field");
            } catch (...) {
              try {
                dixel->set_internal (image.header().size (3));
                DEBUG ("Image " + image.header().name() + " initialised as dixel ODF using internal direction set");
              } catch (...) {
                DEBUG ("Image " + image.header().name() + " left uninitialised in ODF tool");
              }
            }
          }
        }

        bool ODF_Item::valid() const {
          if (odf_type == odf_type_t::SH || odf_type == odf_type_t::TENSOR)
            return true;
          assert (dixel);
          if (!dixel->dirs)
            return false;
          return dixel->dirs->size();
        }

        ODF_Item::DixelPlugin::DixelPlugin (const MR::Header& H) :
            dir_type (dir_t::NONE),
            shell_index (0)
        {
          try {
            grad = MR::DWI::get_valid_DW_scheme (H);
            shells.reset (new MR::DWI::Shells (grad));
            shell_index = shells->count() - 1;
          } catch (...) { }
          auto entry = H.keyval().find ("directions");
          if (entry != H.keyval().end()) {
            try {
              const auto lines = split_lines (entry->second);
              if (lines.size() != size_t(H.size (3)))
                throw Exception ("malformed directions field in image \"" + H.name() + "\" - incorrect number of rows");
              for (size_t row = 0; row < lines.size(); ++row) {
                const auto values = parse_floats (lines[row]);
                if (!header_dirs.rows()) {
                  if (values.size() != 2 && values.size() != 3)
                    throw Exception ("malformed directions field in image \"" + H.name() + "\" - should have 2 or 3 columns");
                  header_dirs.resize (lines.size(), values.size());
                } else if (values.size() != size_t(header_dirs.cols())) {
                  header_dirs.resize (0, 0);
                  throw Exception ("malformed directions field in image \"" + H.name() + "\" - variable number of columns");
                }
                for (size_t col = 0; col < values.size(); ++col)
                  header_dirs(row, col) = values[col];
              }
            } catch (Exception& e) {
              DEBUG (e[0]);
              header_dirs.resize (0, 0);
            }
          }
        }

        void ODF_Item::DixelPlugin::set_shell (size_t index) {
          if (!shells)
            throw Exception ("No valid DW scheme defined in header");
          if (index >= shells->count())
            throw Exception ("Shell index is outside valid range");
          Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic> shell_dirs ((*shells)[index].count(), 3);
          const vector<size_t>& volumes = (*shells)[index].get_volumes();
          for (size_t row = 0; row != volumes.size(); ++row)
            shell_dirs.row (row) = grad.row (volumes[row]).head<3>().cast<float>();
          std::unique_ptr<MR::DWI::Directions::Set> new_dirs (new MR::DWI::Directions::Set (shell_dirs));
          std::swap (dirs, new_dirs);
          shell_index = index;
          dir_type = DixelPlugin::dir_t::DW_SCHEME;
        }

        void ODF_Item::DixelPlugin::set_header() {
          if (!header_dirs.rows())
            throw Exception ("No direction scheme defined in header");
          std::unique_ptr<MR::DWI::Directions::Set> new_dirs (new MR::DWI::Directions::Set (header_dirs));
          std::swap (dirs, new_dirs);
          dir_type = DixelPlugin::dir_t::HEADER;
        }

        void ODF_Item::DixelPlugin::set_internal (const size_t n) {
          std::unique_ptr<MR::DWI::Directions::Set> new_dirs (new MR::DWI::Directions::Set (n));
          std::swap (dirs, new_dirs);
          dir_type = DixelPlugin::dir_t::INTERNAL;
        }

        void ODF_Item::DixelPlugin::set_none() {
          if (dirs)
            delete dirs.release();
          dir_type = DixelPlugin::dir_t::NONE;
        }

        void ODF_Item::DixelPlugin::set_from_file (const std::string& path)
        {
          std::unique_ptr<MR::DWI::Directions::Set> new_dirs (new MR::DWI::Directions::Set (path));
          std::swap (dirs, new_dirs);
          dir_type = DixelPlugin::dir_t::FILE;
        }

        Eigen::VectorXf ODF_Item::DixelPlugin::get_shell_data (const Eigen::VectorXf& values) const
        {
          assert (shells);
          const vector<size_t>& volumes ((*shells)[shell_index].get_volumes());
          Eigen::VectorXf result (volumes.size());
          for (size_t i = 0; i != volumes.size(); ++i)
            result[i] = values[volumes[i]];
          return result;
        }

        size_t ODF_Item::DixelPlugin::num_DW_shells() const {
          if (!shells) return 0;
          return shells->count();
        }




      }
    }
  }
}





