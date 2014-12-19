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

#ifndef __gui_dialog_file_h__
#define __gui_dialog_file_h__

#include "file/path.h"
#include "gui/opengl/gl.h"

namespace MR
{
  namespace GUI
  {
    namespace Dialog
    {
      namespace File
      {

        extern const std::string image_filter_string;

        std::string get_folder (QWidget* parent, const std::string& caption, const std::string& folder = std::string());
        std::string get_file (QWidget* parent, const std::string& caption, const std::string& filter = std::string(), const std::string& folder = std::string());
        std::vector<std::string> get_files (QWidget* parent, const std::string& caption, const std::string& filter = std::string(), const std::string& folder = std::string()); 
        std::string get_save_name (QWidget* parent, const std::string& caption, const std::string& suggested_name = std::string(), const std::string& filter = std::string(), const std::string& folder = std::string());

        inline std::string get_image (QWidget* parent, const std::string& caption, const std::string& folder = std::string()) {
          return get_file (parent, caption, image_filter_string, folder);
        }

        inline std::vector<std::string> get_images (QWidget* parent, const std::string& caption, const std::string& folder = std::string()) {
          return get_files (parent, caption, image_filter_string, folder);
        }

        inline std::string get_save_image_name (QWidget* parent, const std::string& caption, const std::string& suggested_name = std::string(), const std::string& folder = std::string()) {
          return get_save_name (parent, caption, suggested_name, image_filter_string, folder);
        }

      }
    }
  }
}

#endif

