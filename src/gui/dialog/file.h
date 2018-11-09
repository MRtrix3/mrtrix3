/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
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
        void check_overwrite_files_func (const std::string& name);

        std::string get_folder (QWidget* parent, const std::string& caption, const std::string& folder = std::string());
        std::string get_file (QWidget* parent, const std::string& caption, const std::string& filter = std::string(), const std::string& folder = std::string());
        vector<std::string> get_files (QWidget* parent, const std::string& caption, const std::string& filter = std::string(), const std::string& folder = std::string()); 
        std::string get_save_name (QWidget* parent, const std::string& caption, const std::string& suggested_name = std::string(), const std::string& filter = std::string(), const std::string& folder = std::string());

        inline std::string get_image (QWidget* parent, const std::string& caption, const std::string& folder = std::string()) {
          return get_file (parent, caption, image_filter_string, folder);
        }

        inline vector<std::string> get_images (QWidget* parent, const std::string& caption, const std::string& folder = std::string()) {
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

