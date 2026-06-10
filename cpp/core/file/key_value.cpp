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

#include "file/key_value.h"

#include <cerrno>
#include <fstream>

#include "app.h"
#include "exception.h"
#include "file/ofstream.h"

namespace MR::File::KeyValue {

void Reader::open(const std::filesystem::path &file, std::string_view first_line) {
  filepath.clear();
  DEBUG("reading key/value file \"" + file.string() + "\"...");
  in.open(file, std::ios::in | std::ios::binary);
  if (!in)
    throw Exception("failed to open key/value file \"" + file.string() + "\": " + MR::C_strerror(errno));
  if (!first_line.empty()) {
    std::string sbuf;
    getline(in, sbuf);
    if (sbuf.compare(0, first_line.size(), first_line) != 0) {
      in.close();
      throw Exception("invalid first line for key/value file \"" + file.string() + "\"" + //
                      " (expected \"" + first_line + "\")");
    }
  }
  filepath = file;
}

bool Reader::next() {
  while (in.good()) {
    std::string sbuf;
    getline(in, sbuf);
    if (in.bad())
      throw Exception("error reading key/value file \"" + filepath.string() + "\": " + MR::C_strerror(errno));

    sbuf = strip(sbuf.substr(0, sbuf.find_first_of('#')));
    if (sbuf == "END") {
      in.setstate(std::ios::eofbit);
      return false;
    }

    if (!sbuf.empty()) {
      const size_t colon = sbuf.find_first_of(':');
      if (colon == std::string::npos) {
        INFO("malformed key/value entry (\"" + sbuf + "\") in file \"" + filepath.string() + "\" - ignored");
      } else {
        K = strip(sbuf.substr(0, colon));
        V = strip(sbuf.substr(colon + 1));
        if (K.empty()) {
          INFO("malformed key/value entry (\"" + sbuf + "\") in file \"" + filepath.string() + "\" - ignored");
        } else
          return true;
      }
    }
  }
  return false;
}

void write(File::OFStream &out, const KeyValues &keyvals, std::string_view prefix, const bool add_to_command_history) {
  bool command_history_appended = false;
  for (const auto &keyval : keyvals) {
    const auto lines = split_lines(keyval.second);
    for (const auto &line : lines)
      out << prefix << keyval.first << ": " << line << "\n";
    if (add_to_command_history && keyval.first == "command_history") {
      out << prefix << "command_history: " << App::command_history_string << "\n";
      command_history_appended = true;
    }
  }
  if (add_to_command_history && !command_history_appended)
    out << prefix << "command_history: " << App::command_history_string << "\n";
}

} // namespace MR::File::KeyValue
