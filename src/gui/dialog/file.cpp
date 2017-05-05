/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#include <QMessageBox>

#include "app.h"
#include "gui/dialog/file.h"
#include "formats/list.h"

#ifdef MRTRIX_MACOSX
# define FILE_DIALOG_OPTIONS QFileDialog::DontUseNativeDialog
#else 
# define FILE_DIALOG_OPTIONS static_cast<QFileDialog::Options> (0)
#endif

namespace MR
{
  namespace GUI
  {
    namespace Dialog
    {
      namespace File 
      {
      
        const std::string image_filter_string = "Medical Images (*" + join (MR::Formats::known_extensions, " *") + ")";




        std::string get_folder (QWidget* parent, const std::string& caption, const std::string& folder) 
        {
          QString qstring = QFileDialog::getExistingDirectory (parent, caption.c_str(), folder.size() ? QString(folder.c_str()) : QString(), QFileDialog::ShowDirsOnly | FILE_DIALOG_OPTIONS);
          if (qstring.size()) {
            std::string folder = qstring.toUtf8().data();
            QDir::setCurrent (Path::dirname (folder).c_str());
            return folder;
          }
          return std::string();
        }




        std::string get_file (QWidget* parent, const std::string& caption, const std::string& filter, const std::string& folder)
        {
          QString qstring = QFileDialog::getOpenFileName (parent, caption.c_str(), folder.size() ? QString(folder.c_str()) : QString(), filter.c_str(), 0, FILE_DIALOG_OPTIONS);
          if (qstring.size()) {
            std::string name = qstring.toUtf8().data();
            QDir::setCurrent (Path::dirname (name).c_str());
            return name;
          }
          return std::string();
        }





        vector<std::string> get_files (QWidget* parent, const std::string& caption, const std::string& filter, const std::string& folder)
        {
          QStringList qlist = QFileDialog::getOpenFileNames (parent, caption.c_str(), folder.size() ? QString(folder.c_str()) : QString(), filter.c_str(), 0, FILE_DIALOG_OPTIONS);
          vector<std::string> list;
          if (qlist.size()) {
            for (int n = 0; n < qlist.size(); ++n) 
              list.push_back (qlist[n].toUtf8().data());
            QDir::setCurrent (Path::dirname (list[0]).c_str());
          }
          return list;
        }


        bool overwrite_files = false;

        void check_overwrite_files_func (const std::string& name) 
        {
          if (overwrite_files)
            return;

          QMessageBox::StandardButton response = QMessageBox::warning (QApplication::activeWindow(), 
              "confirm file overwrite", ("Action will overwrite file \"" + name + "\" - proceed?").c_str(), 
                QMessageBox::Yes | QMessageBox::YesToAll | QMessageBox::Cancel, QMessageBox::Cancel);
          if (response == QMessageBox::Cancel)
            throw Exception ("File overwrite cancelled by user request");
          if (response == QMessageBox::YesToAll)
            overwrite_files = true;
        }



        std::string get_save_name (QWidget* parent, const std::string& caption, const std::string& suggested_name, const std::string& filter, const std::string& folder)
        {
          overwrite_files = false;

          QString selection;
          if (folder.size()) {
            if (suggested_name.size())
              selection = MR::Path::join (folder, suggested_name).c_str();
            else 
              selection = folder.c_str();
          }
          else if (suggested_name.size())
            selection = suggested_name.c_str();
          QString qstring = QFileDialog::getSaveFileName (parent, caption.c_str(), selection, 
              filter.c_str(), 0, FILE_DIALOG_OPTIONS | QFileDialog::DontConfirmOverwrite);
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


