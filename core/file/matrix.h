/* Copyright (c) 2008-2024 the MRtrix3 contributors.
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

#include <string>

#include "exception.h"
#include "file/key_value.h"
#include "file/npy.h"
#include "file/ofstream.h"
#include "file/path.h"
#include "types.h"

namespace MR::File::Matrix {

namespace {

//! write the matrix \a M to text file
template <class MatrixType>
void save_matrix_text(const MatrixType &M,
                      const std::string &filename,
                      const KeyValues &keyvals = KeyValues(),
                      const bool add_to_command_history = true) {
  DEBUG("saving " + str(M.rows()) + "x" + str(M.cols()) + " matrix to text file \"" + filename + "\"...");
  File::OFStream out(filename);
  File::KeyValue::write(out, keyvals, "# ", add_to_command_history);
  Eigen::IOFormat fmt(
      Eigen::FullPrecision, Eigen::DontAlignCols, std::string(1, Path::delimiter(filename)), "\n", "", "", "", "");
  out << M.format(fmt);
  out << "\n";
}

//! read matrix text data into a 2D vector \a filename
template <class ValueType = default_type>
std::vector<std::vector<ValueType>> load_matrix_2D_vector(const std::string &filename,
                                                          std::vector<std::string> *comments = nullptr) {
  std::ifstream stream(filename, std::ios_base::in | std::ios_base::binary);
  if (!stream)
    throw Exception("Unable to open numerical data text file \"" + filename + "\": " + strerror(errno));
  std::vector<std::vector<ValueType>> V;
  std::string sbuf, cbuf;
  size_t hash;

  while (getline(stream, sbuf)) {
    hash = sbuf.find_first_of('#');
    if (comments and hash != std::string::npos) {
      cbuf = strip(sbuf.substr(hash));
      if (sbuf.size() > 1)
        comments->push_back(cbuf.substr(1));
    }

    sbuf = strip(sbuf.substr(0, hash));
    if (sbuf.empty())
      continue;

    const auto elements = MR::split(sbuf, " ,;\t", true);
    V.push_back(std::vector<ValueType>());
    try {
      for (const auto &entry : elements)
        V.back().push_back(to<ValueType>(entry));
    } catch (Exception &e) {
      e.push_back("Cannot load row " + str(V.size()) + " of file \"" + filename +
                  "\" as delimited numerical matrix data:");
      e.push_back(sbuf);
      throw e;
    }

    if (V.size() > 1)
      if (V.back().size() != V[0].size())
        throw Exception("uneven rows in matrix text file \"" + filename + "\" " + "(first row: " + str(V[0].size()) +
                        " columns; row " + str(V.size()) + ": " + str(V.back().size()) + " columns)");
  }
  if (stream.bad())
    throw Exception(strerror(errno));

  if (!V.size())
    throw Exception("no data in matrix text file \"" + filename + "\"");

  return V;
}

//! read matrix text data into an Eigen::Matrix \a filename
template <class ValueType = default_type>
Eigen::Matrix<ValueType, Eigen::Dynamic, Eigen::Dynamic> load_matrix_text(const std::string &filename) {
  DEBUG("loading matrix file \"" + filename + "\"...");
  const std::vector<std::vector<ValueType>> V = load_matrix_2D_vector<ValueType>(filename);

  Eigen::Matrix<ValueType, Eigen::Dynamic, Eigen::Dynamic> M(V.size(), V[0].size());
  for (ssize_t i = 0; i < M.rows(); i++)
    for (ssize_t j = 0; j < M.cols(); j++)
      M(i, j) = V[i][j];

  DEBUG("found " + str(M.rows()) + "x" + str(M.cols()) + " matrix in text file \"" + filename + "\"");
  return M;
}

//! write the vector \a V to text file
template <class VectorType>
void save_vector_text(const VectorType &V,
                      const std::string &filename,
                      const KeyValues &keyvals,
                      const bool add_to_command_history) {
  DEBUG("saving vector of size " + str(V.size()) + " to text file \"" + filename + "\"...");
  File::OFStream out(filename);
  File::KeyValue::write(out, keyvals, "# ", add_to_command_history);
  const char d(Path::delimiter(filename));
  for (decltype(V.size()) i = 0; i < V.size() - 1; i++)
    out << str(V[i], 10) << d;
  out << str(V[V.size() - 1], 10) << "\n";
}

} // namespace

//! write the matrix \a M to file
template <class MatrixType>
void save_matrix(const MatrixType &M,
                 const std::string &filename,
                 const KeyValues &keyvals = KeyValues(),
                 const bool add_to_command_history = true) {
  if (Path::has_suffix(filename, {"npy", ".NPY"}))
    File::NPY::save_matrix(M, filename);
  else
    save_matrix_text<MatrixType>(M, filename, keyvals, add_to_command_history);
}

//! read matrix data into an Eigen::Matrix \a filename
template <class ValueType = default_type>
Eigen::Matrix<ValueType, Eigen::Dynamic, Eigen::Dynamic> load_matrix(const std::string &filename) {
  if (Path::has_suffix(filename, {"npy", ".NPY"}))
    return File::NPY::load_matrix<ValueType>(filename);
  else
    return load_matrix_text<ValueType>(filename);
}

//! read matrix data from \a filename into an Eigen::Tranform class
template <class VectorType> inline transform_type load_transform(const std::string &filename, VectorType &centre) {
  DEBUG("loading transform file \"" + filename + "\"...");

  std::vector<std::string> comments;
  const std::vector<std::vector<default_type>> V = load_matrix_2D_vector<>(filename, &comments);

  if (V.empty())
    throw Exception("transform in file " + filename + " is empty");

  if (V[0].size() != 4)
    throw Exception("transform in file " + filename + " is invalid: does not contain 4 columns.");

  if (V.size() != 3 && V.size() != 4)
    throw Exception("transform in file " + filename + " is invalid: must contain either 3 or 4 rows.");

  transform_type M;
  for (ssize_t i = 0; i < 3; i++)
    for (ssize_t j = 0; j < 4; j++)
      M(i, j) = V[i][j];

  if (centre.size() == 3) {
    const std::string key = " centre: ";
    const std::string key_legacy = "centre ";
    centre[0] = NaN;
    centre[1] = NaN;
    centre[2] = NaN;
    std::vector<std::string> elements;
    for (auto &line : comments) {
      if (strncmp(line.c_str(), key.c_str(), key.size()) == 0)
        elements = split(strip(line.substr(key.size())), " ,;\t", true);
      else if (strncmp(line.c_str(), key_legacy.c_str(), key_legacy.size()) == 0)
        elements = split(strip(line.substr(key_legacy.size())), " ,;\t", true);
      if (elements.size()) {
        if (elements.size() != 3)
          throw Exception("could not parse centre in transformation file " + filename + ": " +
                          strip(line.substr(key.size())));
        try {
          centre[0] = to<default_type>(elements[0]);
          centre[1] = to<default_type>(elements[1]);
          centre[2] = to<default_type>(elements[2]);
        } catch (...) {
          throw Exception("File \"" + filename +
                          "\" contains non-numerical data in centre: " + strip(line.substr(key.size())));
        }
        break;
      }
    }
  }

  return M;
}

inline transform_type load_transform(const std::string &filename) {
  Eigen::VectorXd c;
  return load_transform(filename, c);
}

//! write the transform \a M to file
inline void save_transform(const transform_type &M,
                           const std::string &filename,
                           const KeyValues &keyvals = KeyValues(),
                           const bool add_to_command_history = true) {
  DEBUG("saving transform to file \"" + filename + "\"...");
  File::OFStream out(filename);
  File::KeyValue::write(out, keyvals, "# ", add_to_command_history);
  const char d(Path::delimiter(filename));
  Eigen::IOFormat fmt(Eigen::FullPrecision, Eigen::DontAlignCols, std::string(1, d), "\n", "", "", "", "");
  out << M.matrix().format(fmt);
  out << "\n0" << d << "0" << d << "0" << d << "1\n";
}

template <class Derived>
inline void save_transform(const transform_type &M,
                           const Eigen::MatrixBase<Derived> &centre,
                           const std::string &filename,
                           const KeyValues &keyvals = KeyValues(),
                           const bool add_to_command_history = true) {
  if (centre.rows() != 3 or centre.cols() != 1)
    throw Exception("save_transform() requires 3x1 vector as centre");
  KeyValues local_keyvals = keyvals;
  Eigen::IOFormat centrefmt(Eigen::FullPrecision, Eigen::DontAlignCols, " ", "\n", "", "", "", "\n");
  std::ostringstream os;
  os << centre.transpose().format(centrefmt);
  local_keyvals.insert(std::pair<std::string, std::string>("centre", os.str()));
  save_transform(M, filename, local_keyvals, add_to_command_history);
}

//! write the vector \a V to file
template <class VectorType>
void save_vector(const VectorType &V,
                 const std::string &filename,
                 const KeyValues &keyvals = KeyValues(),
                 const bool add_to_command_history = true) {
  if (Path::has_suffix(filename, {".npy", ".NPY"}))
    File::NPY::save_vector(V, filename);
  else
    save_vector_text(V, filename, keyvals, add_to_command_history);
}

//! read the vector data from \a filename
template <class ValueType = default_type>
Eigen::Matrix<ValueType, Eigen::Dynamic, 1> load_vector(const std::string &filename) {
  auto vec = load_matrix<ValueType>(filename);
  if (vec.cols() == 1)
    return vec.col(0);
  if (vec.rows() > 1)
    throw Exception("file \"" + filename + "\" contains 2D matrix, not 1D vector");
  return vec.row(0);
}

} // namespace MR::File::Matrix
