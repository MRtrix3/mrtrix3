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

#pragma once

#include "file/path.h"
#include "opengl/glutils.h"

namespace MR::GUI::Dialog::File {

extern const std::string image_filter_string;
void check_overwrite_files_func(const std::filesystem::path &name);

class FileDialogReturn {
public:
  std::filesystem::path single_selection;
  std::vector<std::filesystem::path> multi_selection;
  std::filesystem::path last_directory;
  bool empty() const { return single_selection.empty() && multi_selection.empty(); }
};

const FileDialogReturn input_dirpath(QWidget *parent,
                                     std::string_view caption,
                                     std::optional<std::filesystem::path> start_directory = std::nullopt);
const FileDialogReturn input_filepath(QWidget *parent,
                                      std::string_view caption,
                                      std::string_view filter = "",
                                      std::optional<std::filesystem::path> start_directory = std::nullopt);
const FileDialogReturn input_filepaths(QWidget *parent,
                                       std::string_view caption,
                                       std::string_view filter = std::string(),
                                       std::optional<std::filesystem::path> start_directory = std::nullopt);
const FileDialogReturn output_filepath(QWidget *parent,
                                       std::string_view caption,
                                       std::optional<std::filesystem::path> suggested_name = std::nullopt,
                                       std::string_view filter = "",
                                       std::optional<std::filesystem::path> start_directory = std::nullopt);

const FileDialogReturn input_imagepath(QWidget *parent,
                                       std::string_view caption,
                                       std::optional<std::filesystem::path> start_directory = std::nullopt);
const FileDialogReturn input_imagepaths(QWidget *parent,
                                        std::string_view caption,
                                        std::optional<std::filesystem::path> start_directory = std::nullopt);
const FileDialogReturn output_imagepath(QWidget *parent,
                                        std::string_view caption,
                                        std::optional<std::filesystem::path> suggested_name = std::nullopt,
                                        std::optional<std::filesystem::path> start_directory = std::nullopt);

} // namespace MR::GUI::Dialog::File
