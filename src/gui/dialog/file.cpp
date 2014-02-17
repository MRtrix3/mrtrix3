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


#include "gui/dialog/file.h"
#include "image/format/list.h"

namespace MR
{
  namespace GUI
  {
    namespace Dialog
    {
      namespace File 
      {
      
        const std::string image_filter_string = "Medical Images (*" + join (MR::Image::Format::known_extensions, " *") + ")";




        std::string get_folder ( QWidget* parent, const std::string& caption, const std::string& folder) 
        {
          QString qstring = QFileDialog::getExistingDirectory (parent, caption.c_str(), folder.size() ? QString(folder.c_str()) : QString());
          if (qstring.size()) {
            std::string folder = qstring.toUtf8().data();
            QDir::setCurrent (Path::dirname (folder).c_str());
            return folder;
          }
          return std::string();
        }




        std::string get_file (QWidget* parent, const std::string& caption, const std::string& filter, const std::string& folder)
        {
          QString qstring = QFileDialog::getOpenFileName (parent, caption.c_str(), folder.size() ? QString(folder.c_str()) : QString(), filter.c_str());
          if (qstring.size()) {
            std::string name = qstring.toUtf8().data();
            QDir::setCurrent (Path::dirname (name).c_str());
            return name;
          }
          return std::string();
        }





        std::vector<std::string> get_files (QWidget* parent, const std::string& caption, const std::string& filter, const std::string& folder)
        {
          QStringList qlist = QFileDialog::getOpenFileNames (parent, caption.c_str(), folder.size() ? QString(folder.c_str()) : QString(), filter.c_str());
          std::vector<std::string> list;
          if (qlist.size()) {
            for (int n = 0; n < qlist.size(); ++n) 
              list.push_back (qlist[n].toUtf8().data());
            QDir::setCurrent (Path::dirname (list[0]).c_str());
          }
          return list;
        }




        std::string get_save_name (QWidget* parent, const std::string& caption, const std::string& filter, const std::string& folder)
        {
          QString qstring = QFileDialog::getSaveFileName (parent, caption.c_str(), folder.size() ? QString(folder.c_str()) : QString(), filter.c_str());
          if (qstring.size()) {
            std::string folder = qstring.toUtf8().data();
            QDir::setCurrent (Path::dirname (folder).c_str());
          }
          return std::string();
        }

      }
    }
  }
}


