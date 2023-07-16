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

#include "gui/mrview/tool/odf/item.h"

#include "header.h"
#include "dwi/gradient.h"
#include "math/sphere/SH.h"
#include "math/sphere/set/predefined.h"

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
            lmax (odf_type == odf_type_t::SH ? Math::Sphere::SH::LforN (image.header().size (3)) : -1),
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
            grad = MR::DWI::get_DW_scheme (H);
            shells.reset (new MR::DWI::Shells (grad));
            shell_index = shells->count() - 1;
          } catch (...) { }
          auto entry = H.keyval().find ("directions");
          if (entry != H.keyval().end()) {
            try {
              header_dirs = deserialise_matrix (entry->second);
              Math::Sphere::Set::check (header_dirs, H.size (3));
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
          Eigen::Matrix<default_type, Eigen::Dynamic, Eigen::Dynamic> shell_dirs ((*shells)[index].count(), 3);
          const vector<size_t>& volumes = (*shells)[index].get_volumes();
          for (size_t row = 0; row != volumes.size(); ++row)
            shell_dirs.row (row) = grad.row (volumes[row]).head<3>();
          auto new_dirs = MR::make_unique<Math::Sphere::Set::CartesianWithAdjacency> (shell_dirs);
          std::swap (dirs, new_dirs);
          shell_index = index;
          dir_type = DixelPlugin::dir_t::DW_SCHEME;
        }

        void ODF_Item::DixelPlugin::set_header() {
          if (!header_dirs.rows())
            throw Exception ("No direction scheme defined in header");
          auto new_dirs = MR::make_unique<Math::Sphere::Set::CartesianWithAdjacency> (header_dirs);
          std::swap (dirs, new_dirs);
          dir_type = DixelPlugin::dir_t::HEADER;
        }

        void ODF_Item::DixelPlugin::set_internal (const size_t n) {
          auto new_dirs = MR::make_unique<Math::Sphere::Set::CartesianWithAdjacency> (Math::Sphere::Set::Predefined::load (n));
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
          auto new_dirs = MR::make_unique<Math::Sphere::Set::CartesianWithAdjacency> (Math::Sphere::Set::load (path, true));
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
