/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


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

