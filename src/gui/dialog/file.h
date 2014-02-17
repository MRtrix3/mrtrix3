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
        std::string get_save_name (QWidget* parent, const std::string& caption, const std::string& filter = std::string(), const std::string& folder = std::string());

        inline std::string get_image (QWidget* parent, const std::string& caption, const std::string& folder = std::string()) {
          return get_file (parent, caption, image_filter_string, folder);
        }

        inline std::vector<std::string> get_images (QWidget* parent, const std::string& caption, const std::string& folder = std::string()) {
          return get_files (parent, caption, image_filter_string, folder);
        }

        inline std::string get_save_image_name (QWidget* parent, const std::string& caption, const std::string& folder = std::string()) {
          return get_save_name (parent, caption, image_filter_string, folder);
        }

      }
    }
  }
}

#endif

