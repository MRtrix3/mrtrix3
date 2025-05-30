/* Copyright (c) 2008-2025 the MRtrix3 contributors.
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

        std::string get_folder (QWidget* parent, const std::string& caption, std::string* folder = nullptr);
        std::string get_file (QWidget* parent, const std::string& caption, const std::string& filter = std::string(), std::string* folder = nullptr);
        vector<std::string> get_files (QWidget* parent, const std::string& caption, const std::string& filter = std::string(), std::string* folder = nullptr);
        std::string get_save_name (QWidget* parent, const std::string& caption, const std::string& suggested_name = std::string(), const std::string& filter = std::string(), std::string* folder = nullptr);

        inline std::string get_image (QWidget* parent, const std::string& caption, std::string* folder = nullptr) {
          return get_file (parent, caption, image_filter_string, folder);
        }

        inline vector<std::string> get_images (QWidget* parent, const std::string& caption, std::string* folder = nullptr) {
          return get_files (parent, caption, image_filter_string, folder);
        }

        inline std::string get_save_image_name (QWidget* parent, const std::string& caption, const std::string& suggested_name = std::string(), std::string* folder = nullptr) {
          return get_save_name (parent, caption, suggested_name, image_filter_string, folder);
        }

      }
    }
  }
}

#endif

