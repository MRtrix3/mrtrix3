/* Copyright (c) 2008-2026 the MRtrix3 contributors.
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

#include <QMessageBox>

#include "app.h"
#include "dialog/file.h"
#include "formats/list.h"
#include "gui.h"

#ifdef MRTRIX_MACOSX
#define FILE_DIALOG_OPTIONS QFileDialog::DontUseNativeDialog
#else
#define FILE_DIALOG_OPTIONS QFileDialog::Options()
#endif

namespace MR::GUI::Dialog::File {

const std::string image_filter_string = "Medical Images (*" + join(MR::Formats::known_extensions, " *") + ")";

std::filesystem::path get_folder(QWidget *parent, std::string_view caption, std::string *folder) {
  QString qstring = QFileDialog::getExistingDirectory(
      parent, qstr(caption), folder ? qstr(*folder) : QString(), QFileDialog::ShowDirsOnly | FILE_DIALOG_OPTIONS);

  std::string new_folder;
  if (qstring.size()) {
    new_folder = qstring.toUtf8().data();
    if (folder)
      *folder = new_folder;
  }
  return new_folder;
}

std::filesystem::path
get_file(QWidget *parent, std::string_view caption, std::string_view filter, std::string *folder) {
  QString qstring = QFileDialog::getOpenFileName(
      parent, qstr(caption), folder ? qstr(*folder) : QString(), qstr(filter), 0, FILE_DIALOG_OPTIONS);

  std::filesystem::path filepath;
  if (qstring.size()) {
    filepath = qstring.toUtf8().data();
    std::filesystem::path new_folder = filepath.parent_path();
    if (folder)
      *folder = new_folder.string();
  }
  return filepath;
}

std::vector<std::string>
get_files(QWidget *parent, std::string_view caption, std::string_view filter, std::string *folder) {
  QStringList qlist = QFileDialog::getOpenFileNames(
      parent, qstr(caption), folder ? qstr(*folder) : QString(), qstr(filter), 0, FILE_DIALOG_OPTIONS);

  std::vector<std::string> list;
  if (!qlist.empty()) {
    for (int n = 0; n < qlist.size(); ++n)
      list.push_back(qlist[n].toUtf8().data());
    std::filesystem::path new_folder = std::filesystem::path{list[0]}.parent_path();
    if (folder)
      *folder = new_folder.string();
  }
  return list;
}

bool overwrite_files = false;

void check_overwrite_files_func(const std::filesystem::path &name) {
  if (overwrite_files)
    return;

  QMessageBox::StandardButton response =
      QMessageBox::warning(QApplication::activeWindow(),
                           qstr("confirm file overwrite"),
                           qstr("Action will overwrite file \"" + name.string() + "\" - proceed?"),
                           QMessageBox::Yes | QMessageBox::YesToAll | QMessageBox::Cancel,
                           QMessageBox::Cancel);
  if (response == QMessageBox::Cancel)
    throw Exception("File overwrite cancelled by user request");
  if (response == QMessageBox::YesToAll)
    overwrite_files = true;
}

std::filesystem::path get_save_path(QWidget *parent,
                                    std::string_view caption,
                                    const std::filesystem::path &suggested_name,
                                    std::string_view filter,
                                    std::string *folder) {
  overwrite_files = false;

  QString selection;
  if (folder) {
    selection =
        suggested_name.empty() ? qstr(*folder) : qstr((std::filesystem::path(*folder) / suggested_name).string());
  } else if (!suggested_name.empty()) {
    selection = qstr(suggested_name.string());
  }

  QString qstring = QFileDialog::getSaveFileName(
      parent, qstr(caption), selection, qstr(filter), 0, FILE_DIALOG_OPTIONS | QFileDialog::DontConfirmOverwrite);

  std::filesystem::path filename;
  if (qstring.size()) {
    filename = qstring.toUtf8().data();
    std::filesystem::path new_folder = filename.parent_path();
    if (folder)
      *folder = new_folder.string();
  }
  return filename;
}

} // namespace MR::GUI::Dialog::File
