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


