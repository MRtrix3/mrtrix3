/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

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

    constexpr double pi = 3.14159265358979323846;
    constexpr double pi_2 = pi / 2.0;
    constexpr double pi_4 = pi / 4.0;
    constexpr double sqrt2 = 1.41421356237309504880;
    constexpr double sqrt1_2 = 1.0 / sqrt2;

    /** @} */



    /** @defgroup elfun Elementary Functions
      @{ */

    template <typename T> inline T pow2 (const T& v) { return v*v; }
    template <typename T> inline T pow3 (const T& v) { return pow2 (v) *v; }
    template <typename T> inline T pow4 (const T& v) { return pow2 (pow2 (v)); }
    template <typename T> inline T pow5 (const T& v) { return pow4 (v) *v; }
    template <typename T> inline T pow6 (const T& v) { return pow2 (pow3 (v)); }
    template <typename T> inline T pow7 (const T& v) { return pow6 (v) *v; }
    template <typename T> inline T pow8 (const T& v) { return pow2 (pow4 (v)); }
    template <typename T> inline T pow9 (const T& v) { return pow8 (v) *v; }
    template <typename T> inline T pow10 (const T& v) { return pow8 (v) *pow2 (v); }


    //! template function with cast to different type
    /** example:
     * \code
     * float f = 21.412;
     * int x = round<int> (f);
     * \endcode
     */
    template <typename I, typename T> inline I round (const T x) throw ()
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
    template <typename I, typename T> inline I floor (const T x) throw ()
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
    template <typename I, typename T> inline I ceil (const T x) throw ()
    {
      return static_cast<I> (std::ceil (x));
    }

    //! swap values in arrays
    /** \param a the first array containing the values to be swapped
     * \param b the second array containing the values to be swapped
     * \param size the number of elements to swap
     * \param stride_a the increment between successive elements in the first array
     * \param stride_b the increment between successive elements in the second array */
    template <typename T> inline void swap (T* a, T* b, const int size, const int stride_a = 1, const int stride_b = 1) throw ()
    {
      T* const end (a + size*stride_a);
      for (; a < end; a += stride_a, b += stride_b) 
        std::swap (*a, *b);
    }

    //! find maximum value in array
    /** \param x the array containing the values to be searched
     * \param size the number of elements to search
     * \param index the index in the array of the result
     * \param stride the increment between successive elements in the array
     * \return the maximum value found in the array */
    template <typename T> inline T max (const T* const x, const int size, int& index, const int stride = 1) throw ()
    {
      T cval = *x, c;
      index = 0;
      for (int i = 1; i < size; i++)
        if ( (c = x[i*stride]) > cval) {
          cval = c;
          index = i;
        }
      return c;
    }

    //! find minimum value in array
    /** \param x the array containing the values to be searched
     * \param size the number of elements to search
     * \param index the index in the array of the result
     * \param stride the increment between successive elements in the array
     * \return the minimum value found in the array */
    template <typename T> inline T min (const T* const x, const int size, int& index, const int stride = 1) throw ()
    {
      T cval = *x, c;
      index = 0;
      for (int i = 1; i < size; i++)
        if ( (c = x[i*stride]) < cval) {
          cval = c;
          index = i;
        }
      return c;
    }

    //! find maximum absolute value in array
    /** \param x the array containing the values to be searched
     * \param size the number of elements to search
     * \param index the index in the array of the result
     * \param stride the increment between successive elements in the array
     * \return the maximum absolute value found in the array */
    template <typename T> inline T absmax (const T* const x, const int size, int& index, const int stride = 1) throw ()
    {
      T cval = abs (*x), c;
      index = 0;
      for (int i = 1; i < size; i++)
        if ( (c = abs (x[i*stride])) > cval) {
          cval = c;
          index = i;
        }
      return c;
    }


    /** @} */
  }




  //! write the matrix \a M to file
  template <class MatrixType>
    void save_matrix (const MatrixType& M, const std::string& filename) 
    {
      File::OFStream out (filename);
      for (ssize_t i = 0; i < M.rows(); i++) {
        for (ssize_t j = 0; j < M.cols(); j++)
          out << str(M(i,j), 10) << " ";
        out << "\n";
      }
    }

  //! read the matrix data from \a filename
  template <class ValueType = default_type>
    Eigen::Matrix<ValueType, Eigen::Dynamic, Eigen::Dynamic> load_matrix (const std::string& filename) 
    {
      std::ifstream stream (filename, std::ios_base::in | std::ios_base::binary);
      std::vector<std::vector<ValueType>> V;
      std::string sbuf, entry;

      while (getline (stream, sbuf)) {
        sbuf = strip (sbuf.substr (0, sbuf.find_first_of ('#')));
        if (sbuf.empty()) 
          continue;

        V.push_back (std::vector<ValueType>());

        std::istringstream line (sbuf);
        while (line >> entry) 
          V.back().push_back (to<ValueType> (entry));
        if (line.bad())
          throw Exception (strerror (errno));

        if (V.size() > 1)
          if (V.back().size() != V[0].size())
            throw Exception ("uneven rows in matrix");
      }
      if (stream.bad()) 
        throw Exception (strerror (errno));

      if (!V.size())
        throw Exception ("no data in file");

      Eigen::Matrix<ValueType, Eigen::Dynamic, Eigen::Dynamic> M (V.size(), V[0].size());

      for (ssize_t i = 0; i < M.rows(); i++) 
        for (ssize_t j = 0; j < M.cols(); j++)
          M(i,j) = V[i][j];

      return M;
    }


        //! write the vector \a V to file
  template <class VectorType>
    void save_vector (const VectorType& V, const std::string& filename) 
    {
      File::OFStream out (filename);
      for (ssize_t i = 0; i < V.size() - 1; i++)
        out << str(V[i], 10) << " ";
      out << str(V[V.size() - 1], 10);
    }

  //! read the vector data from \a filename
  template <class ValueType = default_type>
    Eigen::Matrix<ValueType, Eigen::Dynamic, 1> load_vector (const std::string& filename) 
    {
      std::ifstream stream (filename, std::ios_base::in | std::ios_base::binary);
      std::vector<ValueType> vec;
      std::string entry;
      while (stream >> entry) 
        vec.push_back (to<ValueType> (entry));
      if (stream.bad()) 
        throw Exception (strerror (errno));

      Eigen::Matrix<ValueType, Eigen::Dynamic, 1> V (vec.size());
      for (ssize_t n = 0; n < V.size(); n++)
        V[n] = vec[n];
      return V;
    }



}

#endif



