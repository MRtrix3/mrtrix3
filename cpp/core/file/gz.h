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

#include <cassert>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <zlib.h>

#include "debug.h"
#include "exception.h"
#include "file/path.h"
#include "mrtrix.h"
#include "types.h"

namespace MR::File {

class GZ {
public:
  GZ() : gz(nullptr) {}
  GZ(std::string_view fname, std::string_view mode) : gz(nullptr) { open(fname, mode); }
  ~GZ() {
    try {
      close();
    } catch (...) {
      FAIL("error closing GZ file \"" + filename + "\": " + error());
      App::exit_error_code = 1;
    }
  }

  std::string name() const { return filename; }

  void open(std::string_view fname, std::string_view mode) {
    close();
    filename = fname;
    if (!MR::Path::exists(filename))
      throw Exception("cannot access file \"" + filename + "\": No such file or directory");

    gz = gzopen(filename.c_str(), std::string(mode).c_str());
    if (!gz)
      throw Exception("error opening file \"" + filename + "\": " + strerror(errno));
  }

  void close() {
    if (gz) {
      if (gzclose(gz))
        throw Exception("error closing GZ file \"" + filename + "\": " + error());
      filename.clear();
      gz = nullptr;
    }
  }

  bool is_open() const { return gz; }
  bool eof() const {
    assert(gz);
    return gzeof(gz);
  }
  int64_t tell() const {
    assert(gz);
    return gztell(gz);
  }
  int64_t tellg() const { return tell(); }

  void seek(int64_t offset) {
    assert(gz);
    z_off_t pos = gzseek(gz, offset, SEEK_SET);
    if (pos < 0)
      throw Exception("error seeking in GZ file \"" + filename + "\": " + error());
  }

  int read(void *const s, size_t n) {
    assert(gz);
    int n_read = gzread(gz, s, n);
    if (n_read < 0)
      throw Exception("error uncompressing GZ file \"" + filename + "\": " + error());
    return n_read;
  }

  void write(const void *const s, size_t n) {
    assert(gz);
    if (gzwrite(gz, s, n) <= 0)
      throw Exception("error writing to GZ file \"" + filename + "\": " + error());
  }

  void write(std::string_view s) {
    assert(gz);
    if (gzputs(gz, std::string(s).c_str()) < 0)
      throw Exception("error writing to GZ file \"" + filename + "\": " + error());
  }

  std::string getline() {
    assert(gz);
    std::string string;
    int c = 0;
    do {
      c = gzgetc(gz);
      if (c < 0) {
        if (eof())
          break;
        throw Exception("error uncompressing GZ file \"" + filename + "\": " + error());
      }
      string += char(c);
    } while (c != '\n');
    if (!string.empty() && (string[string.size() - 1] == 015 || string[string.size() - 1] == '\n'))
      string.resize(string.size() - 1);
    return string;
  }

  template <typename T> T get() {
    T val;
    if (read(&val, sizeof(T)) != sizeof(T))
      throw Exception("error uncompressing GZ file \"" + filename + "\": " + error());
    return val;
  }

  template <typename T> T get(int64_t offset) {
    seek(offset);
    return get<T>();
  }

  template <typename T> T *get(T *buf, size_t n) {
    if (read(buf, n * sizeof(T)) != n * sizeof(T))
      throw Exception("error uncompressing GZ file \"" + filename + "\": " + error());
    return buf;
  }

  template <typename T> T *get(int64_t offset, T *buf, size_t n) {
    seek(offset);
    return get<T>(buf, n);
  }

protected:
  gzFile gz;
  std::string filename;

  const std::string error() {
    int error_number;
    const char *s = gzerror(gz, &error_number); // check_syntax off
    if (error_number == Z_ERRNO)
      s = strerror(errno);
    return std::string(s);
  }
};

} // namespace MR::File
