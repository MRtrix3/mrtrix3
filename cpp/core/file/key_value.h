/* Copyright (c) 2008-2025 the MRtrix3 contributors.
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

#include "mrtrix.h"
#include "types.h"
#include <fstream>

namespace MR::File {

class OFStream;

namespace KeyValue {

class Reader {
public:
  Reader() {}
  Reader(const std::string &file, const char *first_line = nullptr) { open(file, first_line); }

  void open(const std::string &file, const char *first_line = nullptr);
  bool next();
  void close() { in.close(); }

  const std::string &key() const throw() { return (K); }
  const std::string &value() const throw() { return (V); }
  const std::string &name() const throw() { return (filename); }

protected:
  std::string K, V, filename;
  std::ifstream in;
};

void write(File::OFStream &out,
           const KeyValues &keyvals,
           const std::string &prefix,
           const bool add_to_command_history = true);

} // namespace KeyValue
} // namespace MR::File
