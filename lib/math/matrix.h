/*
   Copyright 2008 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 19/05/09.

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

#ifndef __math_matrix_h__
#define __math_matrix_h__

#include <fstream>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_blas.h>

#include "ptr.h"
#include "math/math.h"
#include "math/vector.h"

//! \cond skip

#define LOOP(op) for (size_t i = 0; i < size1; i++) { for (size_t j = 0; j < size2; j++) { op; } }

//! \endcond


namespace MR {
  namespace Math {

    //! \cond skip

    template <typename T> class GSLMatrix;
    template <> class GSLMatrix <float> : public gsl_matrix_float { public: void set (float* p) { data = p; } };
    template <> class GSLMatrix <double> : public gsl_matrix { public: void set (double* p) { data = p; } };

    //! \endcond



    /** @addtogroup linalg 
      @{ */

    //! A matrix class
    /*! This class is a thin wrapper around the GSL matrix classes, and can be
     * passed to existing GSL functions by pointer using the gsl() member
     * functions. */
    template <typename T> class Matrix : protected GSLMatrix<T> 
    {
      public:
        template <typename U> friend class Matrix;
        typedef T value_type;
        typedef typename Vector<T>::View VectorView;

        //! A class to reference existing Matrix data
        /*! This class is used purely to access and modify the elements of
         * existing Matrices. It cannot perform allocation/deallocation or
         * resizing operations. It is designed to be returned by members
         * functions of the Matrix classes to allow convenient access to
         * specific portions of the data (i.e. a subset of a Matrix).
         * 
         * For example:
         * \code
* // create instance of Matrix:
         * Math::Matrix<float> M (10,10);
         *
         * // set all elements to zero:
         * M = 0.0;
         *
         * // set diagonal elements to 1 (this uses the diagonal() function,
         * // which returns a Matrix::View):
         * M.diagonal() = 1.0;
         * 
         * // create instance of Vector from data file:
         * Math::Vector<double> V ("mydatafile.txt");
         *
         * // set every other element of the bottom row to the contents of V
         * // (in this case, this assumes that V has size 5):
         * M.row(9).sub(0,10,2) = V;
         *
         * // in the above, M.row(9) returns the 9th row as a Vector::View,
         * // and the .sub(0,10,2) return a Vector::View of this, skipping
         * // every other element (i.e. stride 2).
         * \endcode
         */
        class View : public Matrix<T> {
          public:

            //! assign the specified \a value to all elements of the matrix
            View& operator= (T value) throw () 
            {
              LOOP (operator()(i,j) = value);
              return (*this); 
            }

            //! assign the values in \a M to the corresponding elements of the matrix
            View& operator= (const Matrix<T>& M) throw () 
            {
              assert (rows() == M.rows() && columns() == M.columns());
              LOOP (operator()(i,j) = M(i,j));
              return (*this);
            }

            //! assign the values in \a M to the corresponding elements of the matrix
            template <typename U> View& operator= (const Matrix<U>& M) throw () 
            {
              assert (rows() == M.rows() && columns() == M.columns());
              LOOP (operator()(i,j) = M(i,j));
              return (*this);
            }

          private:
            View (T* data, size_t nrows, size_t ncolumns, size_t row_skip) throw ()
            {  
              Matrix<T>::size1 = nrows;
              Matrix<T>::size2 = ncolumns; 
              Matrix<T>::tda = row_skip;
              Matrix<T>::set (data);
              Matrix<T>::block = NULL;
              Matrix<T>::owner = 0; 
            }

            friend class Matrix<T>;
        };

        //! construct empty matrix
        Matrix () throw () 
        {
          size1 = size2 = tda = 0;
          data = NULL; 
          block = NULL; 
          owner = 1; 
        }

        //! copy constructor
        Matrix (const Matrix& M)
        { 
          initialize (M.rows(), M.columns());
          LOOP (operator()(i,j) = M(i,j));
        }

        //! copy constructor
        template <typename U> Matrix (const Matrix<U>& M)
        { 
          initialize (M.rows(), M.columns());
          LOOP (operator()(i,j) = M(i,j));
        }

        //! construct matrix of size \a nrows by \a ncolumns
	/** \note the elements of the matrix are left uninitialised. */
        Matrix (size_t nrows, size_t ncolumns) { initialize (nrows, ncolumns); }

	//! construct from existing data array
        /*! \note with this constructor, the matrix is not responsible for
         * managing data allocation or deallocation. The matrix will simply
         * allow access to the data provided using the Matrix interface. The
         * underlying data array must remain accessible during the lifetime of
         * the Matrix class. */
        Matrix (T* data, size_t nrows, size_t ncolumns) throw () 
        { 
	  size1 = nrows; 
          size2 = tda = ncolumns;
	  set (data); 
          block = NULL;
          owner = 0; 
        }
	//! construct from existing data array with non-standard row stride
        /*! \note with this constructor, the matrix is not responsible for
         * managing data allocation or deallocation. The matrix will simply
         * allow access to the data provided using the Matrix interface. The
         * underlying data array must remain accessible during the lifetime of
         * the Matrix class. */
        Matrix (T* data, size_t nrows, size_t ncolumns, size_t row_skip) throw ()
        {  
	  size1 = nrows;
          size2 = ncolumns; 
          tda = row_skip;
	  set (data);
          block = NULL;
          owner = 0; 
        }

	//! construct a matrix by reading from the text file \a filename
        Matrix (const std::string& filename)
        { 
	  size1 = size2 = tda = 0;
	  data = NULL;
          block = NULL;
          owner = 1; 
          load (filename); 
        } 

	//! destructor
        ~Matrix () 
        {
          if (block) { 
            assert (owner);
            GSLBlock<T>::free (block);
          }
        }

	//! deallocate the matrix data
        Matrix& clear ()
        { 
          if (block) {
            assert (owner);
	    GSLBlock<T>::free (block);
          }
	  size1 = size2 = tda = 0;
	  data = NULL; block = NULL; owner = 1; 
          return (*this);
        }
	//! allocate the matrix to have the same size as \a M
        Matrix& allocate (const Matrix& M) { allocate (M.rows(), M.columns()); return (*this); }
	//! allocate the matrix to have the same size as \a M
        template <typename U> Matrix& allocate (const Matrix<U>& M) { allocate (M.rows(), M.columns()); return (*this); }

	//! allocate the matrix to have size \a nrows by \a ncolumns
        Matrix& allocate (size_t nrows, size_t ncolumns) 
        {
          if (rows() == nrows && columns() == ncolumns)
            return (*this);
          if (block) {
            assert (owner);
            if (block->size < nrows * ncolumns) { 
	      GSLBlock<T>::free (block);
	      block = NULL;
            }
          }

          if (!block && nrows*ncolumns) {
            block = GSLBlock<T>::alloc (nrows * ncolumns);
            if (!block) 
              throw Exception ("Failed to allocate memory for Matrix data");  
          }
	  size1 = nrows;
          size2 = tda = ncolumns;
          owner = 1;
          data = block ? block->data : NULL;
          return (*this);
        }

	//! resize the matrix to have size \a nrows by \a ncolumns, preserving existing data
        /*! The \c fill_value argument determines what value the elements of
         * the Matrix will be set to in case the size requested exceeds the
         * current size. */
        Matrix& resize (size_t nrows, size_t ncolumns, value_type fill_value = value_type (0.0))
        {
          if (nrows == 0 || ncolumns == 0) {
            size1 = size2 = 0; 
            return (*this);
          }
          if (!block) {
            allocate (nrows, ncolumns);
            operator= (fill_value);
            return (*this);
          }
          if (ncolumns > tda || nrows*tda > block->size) {
            Matrix M (nrows, ncolumns);
            M.sub (0, rows(), 0, columns()) = *this;
            M.sub (rows(), M.rows(), 0, columns()) = fill_value;
            M.sub (0, M.rows(), columns(), M.columns()) = fill_value;
            swap (M);
            return (*this); 
          }
          size1 = nrows;
          size2 = ncolumns;
          return (*this); 
        }

	//! read matrix data from the text file \a filename
        Matrix& load (const std::string& filename)
        {
          std::ifstream in (filename.c_str());
          if (!in) 
            throw Exception ("cannot open matrix file \"" + filename + "\": " + strerror (errno));
          try {
            in >> *this; 
          }
          catch (Exception& E) { 
            throw Exception (E, "error loading matrix file \"" + filename + "\""); 
          }
          return (*this);
        }

	//! write to text file \a filename
        void save (const std::string& filename) const
        {
          std::ofstream out (filename.c_str());
          if (!out)
            throw Exception ("cannot open matrix file \"" + filename + "\": " + strerror (errno));
          out << *this;
        }

	//! used to obtain a pointer to the underlying GSL structure
	GSLMatrix<T>* gsl () { return (this); }

	//! used to obtain a pointer to the underlying GSL structure
        const GSLMatrix<T>* gsl () const { return (this); }

	//! assign the specified \a value to all elements of the matrix
        Matrix&  operator= (T value) throw ()
        {
          LOOP (operator()(i,j) = value);
          return (*this); 
        }

	//! assign the values in \a M to the corresponding elements of the matrix
        Matrix&  operator= (const Matrix& M) 
        { 
          allocate (M);
          LOOP (operator()(i,j) = M(i,j)); 
          return (*this); 
        }

	//! assign the values in \a M to the corresponding elements of the matrix
        template <typename U> Matrix& operator= (const Matrix<U>& M)
        {
          allocate (M);
          LOOP (operator()(i,j) = M(i,j)); 
          return (*this); 
        }

	//! comparison operator
        bool operator== (const Matrix& M) throw () { return (! (*this != M)); }

	//! comparison operator
        bool operator!= (const Matrix& M) throw ()
        { 
          if (rows() != M.rows() || columns() != M.columns()) return (true);
          LOOP (if (operator()(i,j) != M(i,j)) return (true)); 
          return (false);
        }

	//! swap contents with \a M without copying
        void swap (Matrix& M) throw ()
        { 
          char c [sizeof (Matrix)];
          memcpy (&c, this, sizeof (Matrix));
          memcpy (this, &M, sizeof (Matrix));
          memcpy (&M, &c, sizeof (Matrix));
        }

	//! return reference to element at \a i, \a j
        T& operator() (size_t i, size_t j) throw ()
        { 
	  assert (i < rows()); assert (j < columns()); 
	  return (ptr()[i * tda + j]); 
	}

	//! return const reference to element at \a i, \a j
        const T& operator() (size_t i, size_t j) const throw ()
        { 
	  assert (i < rows()); assert (j < columns()); 
	  return (ptr()[i * tda + j]); 
	}

	//! return number of rows of matrix
        size_t rows () const throw () { return (size1); }

	//! return number of columns of matrix
        size_t columns () const throw () { return (size2); }

	//! true if matrix points to existing data
        bool is_set () const throw () { return (ptr()); }

	//! return a pointer to the underlying data
        T* ptr () throw () { return ((T*) (data)); }

	//! return a pointer to the underlying data
        const T* ptr () const throw () { return ((const T*) (data)); }

	//! return the row stride
        size_t row_stride () const throw () { return (tda); }

	//! set all elements of matrix to zero
        Matrix& zero () { LOOP (operator()(i,j) = 0.0); return (*this); }

	//! set all diagonal elements of matrix to one, and all others to zero
        Matrix& identity () throw () { LOOP (operator()(i,j) = (i==j?1.0:0.0)); return (*this); }

	//! add \a value to all elements of the matrix
        Matrix& operator+= (T value) throw () { LOOP (operator()(i,j) += value); return (*this); }
	//! subtract \a value from all elements of the matrix
        Matrix& operator-= (T value) throw () { LOOP (operator()(i,j) -= value); return (*this); }
	//! multiply all elements of the matrix by \a value 
        Matrix& operator*= (T value) throw () { LOOP (operator()(i,j) *= value); return (*this); }
	//! divide all elements of the matrix by \a value 
        Matrix& operator/= (T value) throw () { LOOP (operator()(i,j) /= value); return (*this); }

	//! add each element of \a M to the corresponding element of the matrix
        Matrix& operator+= (const Matrix& M) throw ()
        { 
          assert (rows() == M.rows() && columns() == M.columns());
          LOOP (operator()(i,j) += M(i,j));
          return (*this); 
        }

	//! subtract each element of \a M from the corresponding element of the matrix
        Matrix& operator-= (const Matrix& M) throw ()
        { 
          assert (rows() == M.rows() && columns() == M.columns());
          LOOP (operator()(i,j) -= M(i,j)); 
          return (*this);
        }

	//! multiply each element of the matrix by the corresponding element of \a M
        Matrix& operator*= (const Matrix& M) throw ()
        {
          assert (rows() == M.rows() && columns() == M.columns());
          LOOP (operator()(i,j) *= M(i,j)); 
          return (*this);
        }

	//! divide each element of the matrix by the corresponding element of \a M
        Matrix& operator/= (const Matrix& M) throw ()
        { 
          assert (rows() == M.rows() && columns() == M.columns());
          LOOP (operator()(i,j) /= M(i,j));
          return (*this); 
        }

	//! return a Matrix::View corresponding to a submatrix of the matrix
        View sub (size_t from_row, size_t to_row, size_t from_column, size_t to_column) throw ()
        {
          assert (from_row <= to_row && to_row <= rows());
          assert (from_column <= to_column && to_column <= columns());
          return (View (ptr() + from_row*tda + from_column, 
                  to_row-from_row, to_column-from_column, tda));
        }

	//! return a Matrix::View corresponding to a submatrix of the matrix
        const View sub (size_t from_row, size_t to_row, size_t from_column, size_t to_column) const throw ()
        {
          assert (from_row <= to_row && to_row <= rows());
          assert (from_column <= to_column && to_column <= columns());
          return (View (ptr() + from_row*tda + from_column, 
                  to_row-from_row, to_column-from_column, tda));
        }

	//! return a Vector::View corresponding to a row of the matrix
        VectorView row (size_t index = 0) throw ()
        { 
	  assert (index < rows()); 
	  return (VectorView (ptr()+index*tda, size2, 1)); 
	}
	//! return a Vector::View corresponding to a row of the matrix
        const VectorView row (size_t index = 0) const throw ()
        { 
	  assert (index < rows()); 
	  return (VectorView (const_cast<T*> (ptr())+index*tda, size2, 1)); 
	}

	//! return a Vector::View corresponding to a column of the matrix
        VectorView column (size_t index = 0) throw ()
        { 
	  assert (index < columns());
	  return (VectorView (ptr()+index, size1, tda)); 
	}

	//! return a Vector::View corresponding to a column of the matrix
        const VectorView column (size_t index = 0) const throw () { 
	  assert (index < columns());
	  return (VectorView (const_cast<T*> (ptr())+index, size1, tda)); 
	}

	//! return a Vector::View corresponding to the diagonal of the matrix
        VectorView diagonal () throw () 
        {
	  assert (rows() > 0 && columns() > 0);
	  return (VectorView (ptr(), MIN(size1,size2), tda+1)); 
	}

	//! return a Vector::View corresponding to the diagonal of the matrix
        const VectorView diagonal () const throw () 
        {
	  assert (rows() > 0 && columns() > 0);
	  return (VectorView (const_cast<T*> (ptr()), MIN(size1,size2), tda+1)); 
	}

	//! return a Vector::View corresponding to a diagonal of the matrix
	/** \param offset the diagonal to obtain. If \a offset > 0, return the corresponding upper diagonal. 
	  If \a offset < 0, return the corresponding lower diagonal. */ 
        VectorView diagonal (int offset) throw ()
        {
          assert (rows() > 0 && columns() > 0);
          if (offset == 0) return (diagonal());
          if (offset < 0) 
            return (VectorView (ptr()-tda*offset, 
                  MIN(size1+offset,size2+offset), tda+1));
          return (VectorView (ptr()+offset,
	       	MIN(size1-offset,size2-offset), tda+1));
        }

	//! return a Vector::View corresponding to a diagonal of the matrix
        /*! \copydetails Matrix<T>::diagonal(int) */
        const VectorView diagonal (int offset) const throw ()
        {
          assert (rows() > 0 && columns() > 0);
          if (offset == 0) return (diagonal());
          if (offset < 0) 
            return (VectorView (const_cast<T*> (ptr()-tda*offset), 
                  MIN(size1+offset,size2+offset), tda+1));
          return (VectorView (const_cast<T*> (ptr()+offset),
	       	MIN(size1-offset,size2-offset), tda+1));
        }

	//! swap two rows of the matrix
        void swap_rows (size_t n, size_t m) 
        {
	  for (size_t i = 0; i < size2; i++)
	    std::swap (operator()(n,i), operator()(m,i)); 
	}

	//! swap two columns of the matrix
        void swap_columns (size_t n, size_t m)
        { 
	  for (size_t i = 0; i < size1; i++) 
	    std::swap (operator()(i,n), operator()(i,m)); 
	}


	//! write the matrix \a M to \a stream as text
        friend std::ostream& operator<< (std::ostream& stream, const Matrix& M) 
        {
          for (size_t i = 0; i < M.rows(); i++) {
            for (size_t j = 0; j < M.columns(); j++) 
              stream << M(i,j) << " ";
            stream << "\n";
          }
          return (stream);
        }

	//! read the matrix data from \a stream and assign to the matrix \a M
        friend std::istream& operator>> (std::istream& stream, Matrix& M)
        {
          std::vector< RefPtr< std::vector<T> > > V;
          do {
            std::string sbuf;
            getline (stream, sbuf);
            if (stream.bad()) throw Exception (strerror (errno));
            if (stream.eof()) break;

            sbuf = strip (sbuf.substr (0, sbuf.find_first_of ('#')));
            if (sbuf.size()) {
              V.push_back (RefPtr< std::vector<T> > (new std::vector<T>));

              std::istringstream stream (sbuf);
              while (true) {
                T val;
                stream >> val;
                if (stream.fail()) break;
                V.back()->push_back (val);
              }

              if (V.size() > 1)  
                if (V.back()->size() != V[0]->size())
                  throw Exception ("uneven rows in matrix");
            }
          } while (stream.good());

          M.allocate (V.size(), V[0]->size());

          for (size_t i = 0; i < M.rows(); i++) {
            const std::vector<T>& W (*V[i]);
            for (size_t j = 0; j < M.columns(); j++)
              M(i,j) = W[j];
          }

          return (stream);
        }

      protected:
        using GSLMatrix<T>::size1;
        using GSLMatrix<T>::size2;
        using GSLMatrix<T>::tda;
        using GSLMatrix<T>::block;
        using GSLMatrix<T>::owner;
        using GSLMatrix<T>::data;
        using GSLMatrix<T>::set;

        void initialize (size_t nrows, size_t ncolumns) 
        { 
          if (nrows && ncolumns) {
            block = GSLBlock<T>::alloc (nrows * ncolumns);
            if (!block) 
              throw Exception ("Failed to allocate memory for Matrix data");  
          }
          else block = NULL;
	  size1 = nrows; 
          size2 = tda = ncolumns;
          owner = 1;
          data = block ? block->data : NULL;
        }
    };








    /** @defgroup gemv Matrix-vector multiplication
      @{ */

    //! Computes the matrix-vector product \a y = \a alpha \a op_A (\a A) \a x + \a beta \a y 
    /** \param y the target vector
     * \param beta used to add a multiple of \a y to the final product
     * \param alpha used to scale the product
     * \param op_A determines how to use the matrix \a A:
     *    - CblasNoTrans: use A
     *    - CblasTrans: use transpose of A
     *    - CblasConjTrans: use conjugate transpose of A
     * \param A a Matrix
     * \param x a Vector
     * \return a reference to the target vector \a y
     */
    template <typename T> inline Vector<T>& mult (Vector<T>& y, T beta, T alpha, CBLAS_TRANSPOSE op_A, const Matrix<T>& A, const Vector<T>& x) 
    { 
      gemv (op_A, alpha, A, x, beta, y);
      return (y); 
    }

    //! Computes the matrix-vector product \a y = \a alpha \a op_A (\a A) \a x, allocating storage for \a y
    /** \param y the target vector
     * \param alpha used to scale the product
     * \param op_A determines how to use the matrix \a A:
     *    - CblasNoTrans: use A
     *    - CblasTrans: use transpose of A
     *    - CblasConjTrans: use conjugate transpose of A
     * \param A a Matrix
     * \param x a Vector
     * \note this version will perform the appropriate allocation for \a y. 
     * \return a reference to the target vector \a y
     */
    template <typename T> inline Vector<T>& mult (Vector<T>& y, T alpha, CBLAS_TRANSPOSE op_A, const Matrix<T>& A, const Vector<T>& x) { 
      y.allocate (A.rows());
      mult (y, T(0.0), alpha, op_A, A, x);
      return (y); 
    }

    //! Computes the matrix-vector product \a y = \a A \a x
    /** \param y the target vector
     * \param A a Matrix
     * \param x a Vector
     * \return a reference to the target vector \a y
     */
    template <typename T> inline Vector<T>& mult (Vector<T>& y, const Matrix<T>& A, const Vector<T>& x) {
      return (mult (y, T(1.0), CblasNoTrans, A, x)); 
    }

    /** @} */



    /** @defgroup gemm General matrix-matrix multiplication
      @{ */


    //! computes the general matrix-matrix multiplication \a C = \a alpha \a op_A (\a A) \a op_B (\a B) + \a beta \a C
    /** \param C the target matrix
     * \param beta used to add a multiple of \a C to the final product
     * \param alpha used to scale the product
     * \param op_A determines how to use the matrix \a A:
     *    - CblasNoTrans: use A
     *    - CblasTrans: use transpose of A
     *    - CblasConjTrans: use conjugate transpose of A
     * \param A a Matrix
     * \param op_B determines how to use the matrix \a B:
     *    - CblasNoTrans: use B
     *    - CblasTrans: use transpose of B
     *    - CblasConjTrans: use conjugate transpose of B
     * \param B a Matrix
     * \return a reference to the target matrix \a C
     */
    template <typename T> inline Matrix<T>& mult (Matrix<T>& C, T beta, T alpha, CBLAS_TRANSPOSE op_A, const Matrix<T>& A, CBLAS_TRANSPOSE op_B, const Matrix<T>& B) 
    { 
      gemm (op_A, op_B, alpha, A, B, beta, C);
      return (C); 
    }

    //! computes the general matrix-matrix multiplication \a C = \a alpha \a op_A (\a A) \a op_A (\a B), allocating storage for \a C
    /** \param C the target matrix
     * \param alpha used to scale the product
     * \param op_A determines how to use the matrix \a A:
     *    - CblasNoTrans: use A
     *    - CblasTrans: use transpose of A
     *    - CblasConjTrans: use conjugate transpose of A
     * \param A a Matrix
     * \param op_B determines how to use the matrix \a B:
     *    - CblasNoTrans: use B
     *    - CblasTrans: use transpose of B
     *    - CblasConjTrans: use conjugate transpose of B
     * \param B a Matrix
     * \note this version will perform the appropriate allocation for \a C 
     * \return a reference to the target matrix \a C
     */
    template <typename T> inline Matrix<T>& mult (Matrix<T>& C, T alpha, CBLAS_TRANSPOSE op_A, const Matrix<T>& A, CBLAS_TRANSPOSE op_B, const Matrix<T>& B) { 
      C.allocate (op_A == CblasNoTrans ? A.rows() : A.columns(), op_B == CblasNoTrans ? B.columns() : B.rows());
      mult (C, T(0.0), alpha, op_A, A, op_B, B);
      return (C); 
    }

    //! computes the simplified general matrix-matrix multiplication \a C = \a A \a B
    /** \param C the target matrix
     * \param A a Matrix
     * \param B a Matrix
     * \return a reference to the target matrix \a C
     */
    template <typename T> inline Matrix<T>& mult (Matrix<T>& C, const Matrix<T>& A, const Matrix<T>& B) { 
      return (mult (C, T(1.0), CblasNoTrans, A, CblasNoTrans, B)); 
    }

    /** @} */






    /** @defgroup symm Symmetric matrix-matrix multiplication
      @{ */

    //! computes \a C = \a alpha \a A \a B + \a beta \a C or \a C = \a alpha \a B \a A + \a beta \a C, where \a A is symmetric
    /** Computes the symmetric matrix-matrix product \a C = \a alpha \a A \a B + \a beta \a C 
     * or \a C = \a alpha \a B \a A + \a beta \a C, where the matrix A is symmetric.
     * \param C the target matrix
     * \param side determines the order of the product:
     *    - CblasLeft: C = alpha*A*B + beta*C
     *    - CblasRight: C = alpha*B*A + beta*C 
     * \param beta used to add a multiple of \a C to the final product
     * \param alpha used to scale the product
     * \param uplo determines which part of \a A will be used:
     *    - CblasUpper: upper triangle and diagonal of A
     *    - CblasLower: lower triangle and diagonal of A
     * \param A a Matrix
     * \param B a Matrix
     * \return a reference to the target matrix \a C
     */
    template <typename T> inline Matrix<T>& mult (Matrix<T>& C, CBLAS_SIDE side, T beta, T alpha, CBLAS_UPLO uplo, const Matrix<T>& A, const Matrix<T>& B) 
    { 
      symm (side, uplo, alpha, A, B, beta, C);
      return (C);
    }

    //! computes \a C = \a alpha \a A \a B or \a C = \a alpha \a B \a A, where \a A is symmetric, and allocates storage for \a C
    /** Computes the symmetric matrix-matrix product \a C = \a alpha \a A \a B 
     * or \a C = \a alpha \a B \a A, where the matrix A is symmetric.
     * \param C the target matrix
     * \param side determines the order of the product:
     *    - CblasLeft: C = alpha*A*B 
     *    - CblasRight: C = alpha*B*A 
     * \param alpha used to scale the product
     * \param uplo determines which part of \a A will be used:
     *    - CblasUpper: upper triangle and diagonal of A
     *    - CblasLower: lower triangle and diagonal of A
     * \param A a Matrix
     * \param B a Matrix
     * \note this version will perform the appropriate allocation for \a C 
     * \return a reference to the target matrix \a C
     */
    template <typename T> inline Matrix<T>& mult (Matrix<T>& C, CBLAS_SIDE side, T alpha, CBLAS_UPLO uplo, const Matrix<T>& A, const Matrix<T>& B) 
    { 
      C.allocate (A);
      symm (side, uplo, alpha, A, B, T(0.0), C);
      return (C);
    }

    /** @} */



    //! solve for \a y in the triangular matrix problem: \a op_A(\a A) \a y = \a x
    /** \param x the target vector. On input, \a x should be set to \a y
     * \param A a Matrix
     * \param uplo determines which part of \a A will be used:
     *    - CblasUpper: upper triangle and diagonal of A
     *    - CblasLower: lower triangle and diagonal of A
     * \param op_A determines how to use the matrix \a A:
     *    - CblasNoTrans: use A
     *    - CblasTrans: use transpose of A
     *    - CblasConjTrans: use conjugate transpose of A
     * \param diag determines how the diagonal of \a A is used:
     *    - CblasUnit: diagonal elements of \a A are assumed to be unity
     *    - CblasNonUnit: diagonal elements of \a A are used
     * \return a reference to the target vector \a x
     */
    template <typename T> inline Vector<T>& solve_triangular (Vector<T>& x, const Matrix<T>& A, CBLAS_UPLO uplo = CblasUpper, CBLAS_TRANSPOSE op_A = CblasNoTrans, CBLAS_DIAG diag = CblasNonUnit)
    {
      trsv (uplo, op_A, diag, A, x);
      return (x);
    }

    //! rank-1 update: \a A = \a alpha \a x \a y^T + \a A
    /** \param A the target matrix. 
     * \param x a Vector
     * \param y a Vector
     * \param alpha used to scale the product
     * \return a reference to the target matrix \a A
     */
    template <typename T> inline Matrix<T>& rank1_update (Matrix<T>& A, const Vector<T>& x, const Vector<T>& y, T alpha = 1.0)
    {
      ger (alpha, x, y, A);
      return (A);
    }

    //! symmetric rank-1 update: \a A = \a alpha \a x \a x^T + \a A, for symmetric \a A
    /** \param A the symmetric target matrix. 
     * \param x a Vector
     * \param alpha used to scale the product
     * \param uplo determines which part of \a A will be used:
     *    - CblasUpper: upper triangle and diagonal of A
     *    - CblasLower: lower triangle and diagonal of A
     * \return a reference to the target matrix \a A
     */
    template <typename T> inline Matrix<T>& sym_rank1_update (Matrix<T>& A, const Vector<T>& x, T alpha = 1.0, CBLAS_UPLO uplo = CblasUpper)
    {
      syr (uplo, alpha, x, A);
      return (A);
    }

    //! symmetric rank-N update: \a C = \a alpha \a op_A(\a A) op_A(\a A)^T + \a beta C, for symmetric \a A
    /** \param C the target matrix. If \a beta is non-zero, \a C should be symmetric
     * \param A a Matrix
     * \param op_A determines how to use the matrix \a A:
     *    - CblasNoTrans: use A
     *    - CblasTrans: use transpose of A
     *    - CblasConjTrans: use conjugate transpose of A
     * \param uplo determines which part of \a C will be used:
     *    - CblasUpper: upper triangle and diagonal of C
     *    - CblasLower: lower triangle and diagonal of C
     * \param alpha used to scale the product
     * \param beta used to add a multiple of \a C to the final product
     * \return a reference to the target matrix \a C
     */
    template <typename T> inline Matrix<T>& rankN_update (Matrix<T>& C, const Matrix<T>& A, CBLAS_TRANSPOSE op_A = CblasNoTrans, CBLAS_UPLO uplo = CblasUpper, T alpha = 1.0, T beta = 0.0)
    {
      syrk (uplo, op_A, alpha, A, beta, C);
      return (C);
    }

    //! compute transpose \a A = \a B^T
    /** \param A the target matrix. 
     * \param B a Matrix to be transposed
     * \return a reference to the target matrix \a A
     * \note this version will perform the appropriate allocation for \a A 
     */
    template <typename T> inline Matrix<T>& transpose (Matrix<T>& A, const Matrix<T>& B)
    {
      A.allocate (B.columns(), B.rows());
      for (size_t i = 0; i < B.rows(); i++)
        for (size_t j = 0; j < B.columns(); j++)
          A(j,i) = B(i,j);
      return (A);
    }
    /** @} */






    //! \cond skip

    namespace {
      // double definitions:

      inline void gemm (CBLAS_TRANSPOSE op_A, CBLAS_TRANSPOSE op_B, double alpha, const Matrix<double>& A, const Matrix<double>& B, double beta, Matrix<double>& C)
      { gsl_blas_dgemm (op_A, op_B, alpha, A.gsl(), B.gsl(), beta, C.gsl()); }

      inline void gemv (CBLAS_TRANSPOSE op_A, double alpha, const Matrix<double>& A, const Vector<double>& x, double beta, Vector<double>& y)
      { gsl_blas_dgemv (op_A, alpha, A.gsl(), x.gsl(), beta, y.gsl()); }

      inline void symm (CBLAS_SIDE side, CBLAS_UPLO uplo, double alpha, const Matrix<double>& A, const Matrix<double>& B, double beta, Matrix<double>& C)
       { gsl_blas_dsymm (side, uplo, alpha, A.gsl(), B.gsl(), beta, C.gsl()); }

      inline void trsv (CBLAS_UPLO uplo, CBLAS_TRANSPOSE op_A, CBLAS_DIAG diag, const Matrix<double>& A, Vector<double>& x)
      { gsl_blas_dtrsv (uplo, op_A, diag, A.gsl(), x.gsl()); }

      inline void ger (double alpha, const Vector<double>& x, const Vector<double>& y, Matrix<double>& A)
      { gsl_blas_dger (alpha, x.gsl(), y.gsl(), A.gsl()); }

      inline void syr (CBLAS_UPLO uplo, double alpha, const Vector<double>& x, Matrix<double>& A)
      { gsl_blas_dsyr (uplo, alpha, x.gsl(), A.gsl()); }

      inline void syrk (CBLAS_UPLO uplo, CBLAS_TRANSPOSE op_A, double alpha, const Matrix<double>& A, double beta, Matrix<double>& C)
      { gsl_blas_dsyrk (uplo, op_A, alpha, A.gsl(), beta, C.gsl()); }

      // float definitions:


      inline void gemm (CBLAS_TRANSPOSE op_A, CBLAS_TRANSPOSE op_B, float alpha, const Matrix<float>& A, const Matrix<float>& B, float beta, Matrix<float>& C)
      { gsl_blas_sgemm (op_A, op_B, alpha, A.gsl(), B.gsl(), beta, C.gsl()); }

      inline void gemv (CBLAS_TRANSPOSE op_A, float alpha, const Matrix<float>& A, const Vector<float>& x, float beta, Vector<float>& y)
      { gsl_blas_sgemv (op_A, alpha, A.gsl(), x.gsl(), beta, y.gsl()); }

      inline void symm (CBLAS_SIDE side, CBLAS_UPLO uplo, float alpha, const Matrix<float>& A, const Matrix<float>& B, float beta, Matrix<float>& C)
      { gsl_blas_ssymm (side, uplo, alpha, A.gsl(), B.gsl(), beta, C.gsl()); }

      inline void trsv (CBLAS_UPLO uplo, CBLAS_TRANSPOSE op_A, CBLAS_DIAG diag, const Matrix<float>& A, Vector<float>& x)
      { gsl_blas_strsv (uplo, op_A, diag, A.gsl(), x.gsl()); }

      inline void ger (float alpha, const Vector<float>& x, const Vector<float>& y, Matrix<float>& A)
      { gsl_blas_sger (alpha, x.gsl(), y.gsl(), A.gsl()); }

      inline void syr (CBLAS_UPLO uplo, float alpha, const Vector<float>& x, Matrix<float>& A)
      { gsl_blas_ssyr (uplo, alpha, x.gsl(), A.gsl()); }

      inline void syrk (CBLAS_UPLO uplo, CBLAS_TRANSPOSE op_A, float alpha, const Matrix<float>& A, float beta, Matrix<float>& C)
      { gsl_blas_ssyrk (uplo, op_A, alpha, A.gsl(), beta, C.gsl()); }

    }

    //! \endcond

  }
}

#undef LOOP

#endif

