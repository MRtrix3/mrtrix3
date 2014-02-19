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
          std::string name;
          if (qstring.size()) {
            name = qstring.toUtf8().data();
            QDir::setCurrent (Path::dirname (name).c_str());
          }
          return name;
        }

      }
    }
  }
}


