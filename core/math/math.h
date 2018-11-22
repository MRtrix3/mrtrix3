/* Copyright (c) 2008-2019 the MRtrix3 contributors.
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

#ifndef __math_math_h__
#define __math_math_h__

#include <cmath>
#include <cstdlib>

#include "types.h"
#include "mrtrix.h"
#include "exception.h"
#include "file/ofstream.h"

namespace MR
{
  namespace Math
  {

    /** @defgroup mathconstants Mathematical constants
      @{ */

    constexpr double e = 2.71828182845904523536;
    constexpr double pi = 3.14159265358979323846;
    constexpr double pi_2 = pi / 2.0;
    constexpr double pi_4 = pi / 4.0;
    constexpr double sqrt2 = 1.41421356237309504880;
    constexpr double sqrt1_2 = 1.0 / sqrt2;

    /** @} */



    /** @defgroup elfun Elementary Functions
      @{ */

    template <typename T> inline constexpr T pow2 (const T& v) { return v*v; }
    template <typename T> inline constexpr T pow3 (const T& v) { return pow2 (v) *v; }
    template <typename T> inline constexpr T pow4 (const T& v) { return pow2 (pow2 (v)); }
    template <typename T> inline constexpr T pow5 (const T& v) { return pow4 (v) *v; }
    template <typename T> inline constexpr T pow6 (const T& v) { return pow2 (pow3 (v)); }
    template <typename T> inline constexpr T pow7 (const T& v) { return pow6 (v) *v; }
    template <typename T> inline constexpr T pow8 (const T& v) { return pow2 (pow4 (v)); }
    template <typename T> inline constexpr T pow9 (const T& v) { return pow8 (v) *v; }
    template <typename T> inline constexpr T pow10 (const T& v) { return pow8 (v) *pow2 (v); }


    template <typename I, typename T> inline constexpr I round (const T x) throw ()
    {
      return static_cast<I> (std::round (x));
    }
    //! template function with cast to different type
    /** example:
     * \code
     * float f = 21.412;
     * int x = floor<int> (f);
     * \endcode
     */
    template <typename I, typename T> inline constexpr I floor (const T x) throw ()
    {
      return static_cast<I> (std::floor (x));
    }
    //! template function with cast to different type
    /** example:
     * \code
     * float f = 21.412;
     * int x = ceil<int> (f);
     * \endcode
     */
    template <typename I, typename T> inline constexpr I ceil (const T x) throw ()
    {
      return static_cast<I> (std::ceil (x));
    }

    /** @} */
  }

  /** @defgroup elfun Eigen helper functions
      @{ */
  //! check if all elements of an Eigen MatrixBase object are finite
  template<typename Derived>
  inline bool is_finite(const Eigen::MatrixBase<Derived>& x)
  {
     return ( (x - x).array() == (x - x).array()).all();
  }

  //! check if all elements of an Eigen MatrixBase object are a number
  template<typename Derived>
  inline bool is_nan(const Eigen::MatrixBase<Derived>& x)
  {
     return ((x.array() == x.array())).all();
  }
  /** @} */

  //! convenience functions for SFINAE on std:: / Eigen containers
  template <class Cont>
  class is_eigen_type { NOMEMALIGN
    typedef char yes[1], no[2];
    template<typename C> static yes& test(typename Cont::Scalar);
    template<typename C> static no&  test(...);
    public:
    static const bool value = sizeof(test<Cont>(0)) == sizeof(yes);
  };

  //! Get the underlying scalar value type for both std:: containers and Eigen
  template <class Cont, typename ReturnType = int>
  class container_value_type { NOMEMALIGN
    public:
     using type = typename Cont::value_type;
  };
  template <class Cont>
  class container_value_type <Cont, typename std::enable_if<is_eigen_type<Cont>::value, int>::type> { NOMEMALIGN
    public:
      using type = typename Cont::Scalar;
  };


  //! write the matrix \a M to file
  template <class MatrixType>
    void save_matrix (const MatrixType& M, const std::string& filename)
    {
      DEBUG ("saving " + str(M.rows()) + "x" + str(M.cols()) + " matrix to file \"" + filename + "\"...");
      File::OFStream out (filename);
      Eigen::IOFormat fmt(Eigen::FullPrecision, Eigen::DontAlignCols, " ", "\n", "", "", "", "");
      out << M.format(fmt);
      out << "\n";
    }

  //! read matrix data into a 2D vector \a filename
  template <class ValueType = default_type>
    vector<vector<ValueType>> load_matrix_2D_vector (const std::string& filename)
    {
      std::ifstream stream (filename, std::ios_base::in | std::ios_base::binary);
      vector<vector<ValueType>> V;
      std::string sbuf;

      while (getline (stream, sbuf)) {
        sbuf = strip (sbuf.substr (0, sbuf.find_first_of ('#')));
        if (sbuf.empty())
          continue;

        V.push_back (vector<ValueType>());

        const auto elements = MR::split (sbuf, " ,;\t", true);
        try {
          for (const auto& entry : elements)
            V.back().push_back (to<ValueType> (entry));
        } catch (...) {
          throw Exception ("File \"" + filename + "\" contains non-numerical data");
        }

        if (V.size() > 1)
          if (V.back().size() != V[0].size())
            throw Exception ("uneven rows in matrix file \"" + filename + "\"");
      }
      if (stream.bad())
        throw Exception (strerror (errno));

      if (!V.size())
        throw Exception ("no data in matrix file \"" + filename + "\"");

      return V;
    }

  //! read matrix data into an Eigen::Matrix \a filename
  template <class ValueType = default_type>
    Eigen::Matrix<ValueType, Eigen::Dynamic, Eigen::Dynamic> load_matrix (const std::string& filename)
    {
      DEBUG ("loading matrix file \"" + filename + "\"...");
      vector<vector<ValueType>> V;
      try {
        V = load_matrix_2D_vector<ValueType> (filename);
      } catch (Exception& e) {
        throw e;
      }

      Eigen::Matrix<ValueType, Eigen::Dynamic, Eigen::Dynamic> M (V.size(), V[0].size());

      for (ssize_t i = 0; i < M.rows(); i++)
        for (ssize_t j = 0; j < M.cols(); j++)
          M(i,j) = V[i][j];

      DEBUG ("found " + str(M.rows()) + "x" + str(M.cols()) + " matrix in file \"" + filename + "\"");
      return M;
    }

  //! read matrix data from a text string \a spec
  template <class ValueType = default_type>
    Eigen::Matrix<ValueType, Eigen::Dynamic, Eigen::Dynamic> parse_matrix (const std::string& spec)
    {
      Eigen::Matrix<ValueType, Eigen::Dynamic, Eigen::Dynamic> M;
      const auto lines = split_lines (spec);
      for (size_t row = 0; row < lines.size(); ++row) {
        const auto values = parse_floats (lines[row]);
        if (M.cols() == 0)
          M.resize (lines.size(), values.size());
        else if (M.cols() != ssize_t (values.size()))
          throw Exception ("error converting string to matrix - uneven number of entries per row");
        for (size_t col = 0; col < values.size(); ++col)
          M(row, col) = values[col];
      }
      return M;
    }

  //! read matrix data from \a filename into an Eigen::Tranform class
  inline transform_type load_transform (const std::string& filename)
  {
    DEBUG ("loading transform file \"" + filename + "\"...");

    vector<vector<default_type>> V;
    try {
      V = load_matrix_2D_vector<> (filename);
    } catch (Exception& e) {
      throw e;
    }

    if (V.empty())
      throw Exception ("transform in file " + filename + " is empty");

    if (V[0].size() != 4)
      throw Exception ("transform in file " + filename + " is invalid: does not contain 4 columns.");

    if (V.size() != 3 && V.size() != 4)
      throw Exception ("transform in file " + filename + " is invalid: must contain either 3 or 4 rows.");

    transform_type M;

    for (ssize_t i = 0; i < 3; i++)
      for (ssize_t j = 0; j < 4; j++)
        M(i,j) = V[i][j];

    return M;
  }

  //! write the transform \a M to file
  inline void save_transform (const transform_type& M, const std::string& filename)
  {
    DEBUG ("saving transform to file \"" + filename + "\"...");
    File::OFStream out (filename);
    Eigen::IOFormat fmt(Eigen::FullPrecision, Eigen::DontAlignCols, " ", "\n", "", "", "", "");
    out << M.matrix().format(fmt);
    out << "\n0 0 0 1\n";
  }

  //! write the vector \a V to file
  template <class VectorType>
    void save_vector (const VectorType& V, const std::string& filename)
    {
      DEBUG ("saving vector of size " + str(V.size()) + " to file \"" + filename + "\"...");
      File::OFStream out (filename);
      for (decltype(V.size()) i = 0; i < V.size() - 1; i++)
        out << str(V[i], 10) << " ";
      out << str(V[V.size() - 1], 10) << "\n";
    }

  //! read the vector data from \a filename
  template <class ValueType = default_type>
    Eigen::Matrix<ValueType, Eigen::Dynamic, 1> load_vector (const std::string& filename)
    {
      auto vec = load_matrix<ValueType> (filename);
      if (vec.cols() == 1)
        return vec.col(0);
      if (vec.rows() > 1)
        throw Exception ("file \"" + filename + "\" contains matrix, not vector");
      return vec.row(0);
    }


}

#endif



