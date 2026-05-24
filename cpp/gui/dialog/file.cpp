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

const FileDialogReturn
input_dirpath(QWidget *parent, std::string_view caption, std::optional<std::filesystem::path> start_directory) {
  const QString qstring = QFileDialog::getExistingDirectory(
      parent,
      qstr(caption),
      start_directory.has_value() ? qstr(start_directory.value().string()) : QString(),
      QFileDialog::ShowDirsOnly | FILE_DIALOG_OPTIONS);

  FileDialogReturn result;
  if (!qstring.isEmpty()) {
    result.single_selection = std::filesystem::path(qstring.toUtf8().data());
    result.last_directory = result.single_selection;
  }
  return result;
}

const FileDialogReturn input_filepath(QWidget *parent,
                                      std::string_view caption,
                                      std::string_view filter,
                                      std::optional<std::filesystem::path> start_directory) {
  const QString qstring =
      QFileDialog::getOpenFileName(parent,
                                   qstr(caption),
                                   start_directory.has_value() ? qstr(start_directory.value().string()) : QString(),
                                   qstr(filter),
                                   0,
                                   FILE_DIALOG_OPTIONS);

  FileDialogReturn result;
  if (qstring.size()) {
    result.single_selection = std::filesystem::path(qstring.toUtf8().data());
    result.last_directory = result.single_selection.parent_path();
  }
  return result;
}

const FileDialogReturn input_filepaths(QWidget *parent,
                                       std::string_view caption,
                                       std::string_view filter,
                                       std::optional<std::filesystem::path> start_directory) {
  const QStringList qlist =
      QFileDialog::getOpenFileNames(parent,
                                    qstr(caption),
                                    start_directory.has_value() ? qstr(start_directory.value().string()) : QString(),
                                    qstr(filter),
                                    0,
                                    FILE_DIALOG_OPTIONS);

  FileDialogReturn result;
  if (!qlist.empty()) {
    for (int n = 0; n < qlist.size(); ++n)
      result.multi_selection.emplace_back(std::filesystem::path(qlist[n].toUtf8().data()));
    result.last_directory = result.multi_selection.back().parent_path();
  }
  return result;
}

bool overwrite_files = false;

void check_overwrite_files_func(const std::filesystem::path &path) {
  if (overwrite_files)
    return;

  QMessageBox::StandardButton const response =
      QMessageBox::warning(QApplication::activeWindow(),
                           qstr("confirm file overwrite"),
                           qstr("Action will overwrite file \"" + path.string() + "\" - proceed?"),
                           QMessageBox::Yes | QMessageBox::YesToAll | QMessageBox::Cancel,
                           QMessageBox::Cancel);
  if (response == QMessageBox::Cancel)
    throw Exception("File overwrite cancelled by user request");
  if (response == QMessageBox::YesToAll)
    overwrite_files = true;
}

const FileDialogReturn output_filepath(QWidget *parent,
                                       std::string_view caption,
                                       std::optional<std::filesystem::path> suggested_name,
                                       std::string_view filter,
                                       std::optional<std::filesystem::path> start_directory) {
  overwrite_files = false;

  QString selection;
  if (start_directory.has_value()) {
    selection = suggested_name.has_value() ? qstr((start_directory.value() / suggested_name.value()).string())
                                           : qstr(start_directory.value().string());
  } else if (suggested_name.has_value()) {
    selection = qstr(suggested_name->string());
  }

  const QString qstring = QFileDialog::getSaveFileName(
      parent, qstr(caption), selection, qstr(filter), 0, FILE_DIALOG_OPTIONS | QFileDialog::DontConfirmOverwrite);

  FileDialogReturn result;
  if (!qstring.isEmpty()) {
    result.single_selection = std::filesystem::path(qstring.toUtf8().data());
    result.last_directory = result.single_selection.parent_path();
  }
  return result;
}

const FileDialogReturn
input_imagepath(QWidget *parent, std::string_view caption, std::optional<std::filesystem::path> start_directory) {
  return input_filepath(parent, caption, image_filter_string, start_directory);
}

const FileDialogReturn
input_imagepaths(QWidget *parent, std::string_view caption, std::optional<std::filesystem::path> start_directory) {
  return input_filepaths(parent, caption, image_filter_string, start_directory);
}

const FileDialogReturn output_imagepath(QWidget *parent,
                                        std::string_view caption,
                                        std::optional<std::filesystem::path> suggested_name,
                                        std::optional<std::filesystem::path> start_directory) {
  return output_filepath(parent, caption, suggested_name, image_filter_string, start_directory);
}

} // namespace MR::GUI::Dialog::File
