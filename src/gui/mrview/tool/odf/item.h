/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 13/11/09.

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

#ifndef __gui_mrview_tool_odf_item_h__
#define __gui_mrview_tool_odf_item_h__

#include <memory>

#include "dwi/shells.h"
#include "dwi/directions/set.h"
#include "gui/dwi/renderer.h"
#include "gui/mrview/gui_image.h"

namespace MR
{

  class Header;

  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {



        class ODF_Item {
            typedef GUI::DWI::Renderer::mode_t mode_t;
          public:
            ODF_Item (MR::Header&& H, const float scale, const bool hide_negative, const bool color_by_direction);

            bool valid() const;

            MRView::Image image;
            mode_t mode;
            int lmax;
            float scale;
            bool hide_negative, color_by_direction;

            class DixelPlugin
            {
              public:
                enum dir_t { DW_SCHEME, HEADER, INTERNAL, NONE, FILE };

                DixelPlugin (const MR::Header&);

                void set_shell (size_t index);
                void set_header();
                void set_internal (const size_t n);
                void set_none();
                void set_from_file (const std::string& path);

                Eigen::VectorXf get_shell_data (const Eigen::VectorXf& values) const;

                size_t num_DW_shells() const;

                dir_t dir_type;
                Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic> header_dirs;
                Eigen::Matrix<double, Eigen::Dynamic, 4> grad;
                std::unique_ptr<MR::DWI::Shells> shells;
                size_t shell_index;
                std::unique_ptr<MR::DWI::Directions::Set> dirs;

            } dixel;
        };



      }
    }
  }
}

#endif





