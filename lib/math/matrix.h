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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#define LOOP(op) for (size_t i = 0; i < GSLMatrix<T>::size1; i++) { for (size_t j = 0; j < GSLMatrix<T>::size2; j++) { op; } }

#endif /* DOXYGEN_SHOULD_SKIP_THIS */


namespace MR {
  namespace Math {

#ifndef DOXYGEN_SHOULD_SKIP_THIS

    template <typename T> class GSLMatrix;
    template <> class GSLMatrix <float> : public gsl_matrix_float { public: void set (float* p) { data = p; } };
    template <> class GSLMatrix <double> : public gsl_matrix { public: void set (double* p) { data = p; } };

#endif /* DOXYGEN_SHOULD_SKIP_THIS */



    /** @addtogroup linalg 
      @{ */


    //! provides access to data as a matrix
    /** \note this class is not capable of managing its own data allocation. 
      See the Matrix class for a more general interface. */
    template <typename T> class MatrixView : protected GSLMatrix<T> 
    {
      public:
        template <typename U> friend class MatrixView;

	//! construct from existing data array
        MatrixView (T* data, size_t nrows, size_t ncolumns) throw () { 
	  GSLMatrix<T>::size1 = nrows; GSLMatrix<T>::size2 = GSLMatrix<T>::tda = ncolumns;
	  GSLMatrix<T>::set(data); GSLMatrix<T>::block = NULL; GSLMatrix<T>::owner = 0; 
        }
	//! construct from existing data array with non-standard row stride
        MatrixView (T* data, size_t nrows, size_t ncolumns, size_t row_skip) throw () {  
	  GSLMatrix<T>::size1 = nrows; GSLMatrix<T>::size2 = ncolumns; GSLMatrix<T>::tda = row_skip;
	  GSLMatrix<T>::set (data); GSLMatrix<T>::block = NULL; GSLMatrix<T>::owner = 0; 
        }

	//! write to text file \a filename
        void          save (const std::string& filename) const {
          std::ofstream out (filename.c_str());
          if (!out) throw Exception ("cannot open matrix file \"" + filename + "\": " + strerror (errno));
          out << *this;
        }

	//! used to obtain a pointer to the underlying GSL structure
	GSLMatrix<T>* operator& () { return (this); }
	//! used to obtain a pointer to the underlying GSL structure
        const GSLMatrix<T>* operator& () const { return (this); }

	//! assign the specified \a value to all elements of the matrix
        MatrixView<T>&  operator= (T value) throw () { LOOP (operator()(i,j) = value); return (*this); }
	//! assign the values in \a M to the corresponding elements of the matrix
        MatrixView<T>&  operator= (const MatrixView& M) throw () { 
          assert (rows() == M.rows() && columns() == M.columns());
          LOOP (operator()(i,j) = M(i,j)); 
          return (*this); 
        }
	//! assign the values in \a M to the corresponding elements of the matrix
        template <typename U> MatrixView<T>& operator= (const MatrixView<U>& M) {
          assert (rows() == M.rows() && columns() == M.columns());
          LOOP (operator()(i,j) = M(i,j)); 
          return (*this); 
        }

	//! comparison operator
        bool          operator== (const MatrixView& M) throw () { return (! (*this != M)); }
	//! comparison operator
        bool          operator!= (const MatrixView& M) throw ()  { 
          if (rows() != M.rows() || columns() != M.columns()) return (true);
          LOOP (if (operator()(i,j) != M(i,j)) return (true)); 
          return (false);
        }
	//! swap contents with \a M without copying
        void          swap (MatrixView& M) throw () { 
          char c [sizeof (MatrixView)];
          memcpy (&c, this, sizeof (MatrixView));
          memcpy (this, &M, sizeof (MatrixView));
          memcpy (&M, &c, sizeof (MatrixView));
        }

	//! return reference to element at \a i, \a j
        T&       operator() (size_t i, size_t j) throw () { 
	  assert (i < rows()); assert (j < columns()); 
	  return (ptr()[i * GSLMatrix<T>::tda + j]); 
	}
	//! return reference to element at \a i, \a j
        const T& operator() (size_t i, size_t j) const throw () { 
	  assert (i < rows()); assert (j < columns()); 
	  return (ptr()[i * GSLMatrix<T>::tda + j]); 
	}
	//! return number of rows of matrix
        size_t        rows () const throw () { return (GSLMatrix<T>::size1); }
	//! return number of columns of matrix
        size_t        columns () const throw () { return (GSLMatrix<T>::size2); }

	//! true if matrix points to existing data
        bool          is_set () const throw () { return (ptr()); }
	//! set all elements of matrix to zero
        MatrixView<T>&   zero () { LOOP (operator()(i,j) = 0.0); return (*this); }
	//! set all diagonal elements of matrix to one, and all others to zero
        MatrixView<T>&   identity () throw () { LOOP (operator()(i,j) = (i==j?1.0:0.0)); return (*this); }

	//! add \a value to all elements of the matrix
        MatrixView<T>& operator+= (T value) throw () { LOOP (operator()(i,j) += value); return (*this); }
	//! subtract \a value from all elements of the matrix
        MatrixView<T>& operator-= (T value) throw () { LOOP (operator()(i,j) -= value); return (*this); }
	//! multiply all elements of the matrix by \a value 
        MatrixView<T>& operator*= (T value) throw () { LOOP (operator()(i,j) *= value); return (*this); }
	//! divide all elements of the matrix by \a value 
        MatrixView<T>& operator/= (T value) throw () { LOOP (operator()(i,j) /= value); return (*this); }

	//! add each element of \a M to the corresponding element of the matrix
        MatrixView<T>& operator+= (const MatrixView& M) throw () { 
          assert (rows() == M.rows() && columns() == M.columns());
          LOOP (operator()(i,j) += M(i,j));
          return (*this); 
        }
	//! subtract each element of \a M from the corresponding element of the matrix
        MatrixView<T>& operator-= (const MatrixView& M) throw () { 
          assert (rows() == M.rows() && columns() == M.columns());
          LOOP (operator()(i,j) -= M(i,j)); 
          return (*this);
        }
	//! multiply each element of the matrix by the corresponding element of \a M
        MatrixView<T>& operator*= (const MatrixView& M) throw () {
          assert (rows() == M.rows() && columns() == M.columns());
          LOOP (operator()(i,j) *= M(i,j)); 
          return (*this);
        }
	//! divide each element of the matrix by the corresponding element of \a M
        MatrixView<T>& operator/= (const MatrixView& M) throw () { 
          assert (rows() == M.rows() && columns() == M.columns());
          LOOP (operator()(i,j) /= M(i,j));
          return (*this); 
        }

	//! return a MatrixView corresponding to a submatrix of the matrix
        MatrixView<T> view (size_t from_row, size_t to_row, size_t from_column, size_t to_column) throw () {
          assert (from_row <= to_row && to_row <= rows());
          assert (from_column <= to_column && to_column <= columns());
          MatrixView<T> M (MatrixView (ptr() + from_row*GSLMatrix<T>::tda + from_column, to_row-from_row, to_column-from_column, GSLMatrix<T>::tda));
          return (M);
        }

	//! return a MatrixView corresponding to a submatrix of the matrix
        const MatrixView<T> view (size_t from_row, size_t to_row, size_t from_column, size_t to_column) const throw () {
          assert (from_row <= to_row && to_row <= rows());
          assert (from_column <= to_column && to_column <= columns());
          MatrixView<T> M (MatrixView (ptr() + from_row*GSLMatrix<T>::tda + from_column, to_row-from_row, to_column-from_column, GSLMatrix<T>::tda));
          return (M);
        }

	//! return a MatrixView corresponding to a row of the matrix
        VectorView<T> row (size_t index = 0) throw () { 
	  assert (index < rows()); 
	  return (VectorView<T> (ptr()+index*GSLMatrix<T>::tda, GSLMatrix<T>::size2, 1)); 
	}
	//! return a MatrixView corresponding to a row of the matrix
        const VectorView<T> row (size_t index = 0) const throw () { 
	  assert (index < rows()); 
	  return (VectorView<T> (const_cast<T*> (ptr())+index*GSLMatrix<T>::tda, GSLMatrix<T>::size2, 1)); 
	}

	//! return a MatrixView corresponding to a column of the matrix
        VectorView<T> column (size_t index = 0)  throw () { 
	  assert (index < columns());
	  return (VectorView<T> (ptr()+index, GSLMatrix<T>::size1, GSLMatrix<T>::tda)); 
	}
	//! return a MatrixView corresponding to a column of the matrix
        const VectorView<T> column (size_t index = 0) const throw () { 
	  assert (index < columns());
	  return (VectorView<T> (const_cast<T*> (ptr())+index, GSLMatrix<T>::size1, GSLMatrix<T>::tda)); 
	}

	//! return a MatrixView corresponding to the diagonal of the matrix
        VectorView<T> diagonal () const throw () {
	  assert (rows() > 0 && columns() > 0);
	  return (VectorView<T> (ptr(), MIN(GSLMatrix<T>::size1,GSLMatrix<T>::size2), GSLMatrix<T>::tda+1)); 
	}
	//! return a MatrixView corresponding to a diagonal of the matrix
	/** \param offset the diagonal to obtain. If \a offset > 0, return the corresponding upper diagonal. 
	  If \a offset < 0, return the corresponding lower diagonal. */ 
        VectorView<T> diagonal (int offset) const throw () {
          assert (rows() > 0 && columns() > 0);
          if (offset == 0) return (diagonal());
          if (offset < 0) return (VectorView<T> (
		ptr()-GSLMatrix<T>::tda*offset, 
		MIN(GSLMatrix<T>::size1+offset,GSLMatrix<T>::size2+offset), 
		GSLMatrix<T>::tda+1));
          return (VectorView<T> (
		ptr()+offset,
	       	MIN(GSLMatrix<T>::size1-offset,GSLMatrix<T>::size2-offset),
	       	GSLMatrix<T>::tda+1));
        }

	//! swap two rows of the matrix
        void swap_rows (size_t n, size_t m) {
	  for (size_t i = 0; i < GSLMatrix<T>::size2; i++)
	    Math::swap (operator()(n,i), operator()(m,i)); 
	}
	//! swap two columns of the matrix
        void swap_columns (size_t n, size_t m) { 
	  for (size_t i = 0; i < GSLMatrix<T>::size1; i++) 
	    Math::swap (operator()(i,n), operator()(i,m)); 
	}

	//! resize the matrix to have size \a nrows by \a ncolumns
	/** \note no bounds checking and no data allocation/deallocation/copying is performed by this function. */
        MatrixView<T>& resize (size_t nrows, size_t ncolumns) { GSLMatrix<T>::size1 = nrows; GSLMatrix<T>::size2 = ncolumns; return (*this); }
	//! resize the matrix to refer the submatrix specified
	/** \note no bounds checking and no data allocation/deallocation/copying is performed by this function. */
        MatrixView<T>& resize (size_t from_row, size_t to_row, size_t from_column, size_t to_column) throw () {
	  ptr() += from_row*GSLMatrix<T>::tda + from_column;
	  GSLMatrix<T>::size1 = to_row-from_row;
	  GSLMatrix<T>::size2 = to_column-from_column;
          return (*this);
        }

	//! return a pointer to the underlying data
        T* ptr () throw () { return ((T*) (GSLMatrix<T>::data)); }
	//! return a pointer to the underlying data
        const T* ptr () const throw () { return ((const T*) (GSLMatrix<T>::data)); }
	//! return the row stride
        size_t row_stride () const throw () { return (GSLMatrix<T>::tda); }


	//! write the matrix \a M to \a stream as text
        friend std::ostream& operator<< (std::ostream& stream, const MatrixView& M)
        {
          for (size_t i = 0; i < M.rows(); i++) {
            for (size_t j = 0; j < M.columns(); j++) 
              stream << M(i,j) << " ";
            stream << "\n";
          }
          return (stream);
        }


      protected:
        MatrixView () throw () {
	  GSLMatrix<T>::size1 = GSLMatrix<T>::size2 = GSLMatrix<T>::tda = 0;
	  GSLMatrix<T>::data = NULL; GSLMatrix<T>::block = NULL; GSLMatrix<T>::owner = 0; }
    };





    //! The main matrix class 
    template <typename T> class Matrix : public MatrixView<T> 
    {
      public:
        template <typename U> friend class Matrix;

	//! construct empty matrix
        Matrix () throw () { }
	//! construct matrix of size \a nrows by \a ncolumns
	/** \note the elements of the matrix are left uninitialised. */
        Matrix (size_t nrows, size_t ncolumns) { allocate (nrows, ncolumns); }
	//! construct a matrix by copying the elements of \a M
        Matrix (const MatrixView<T>& M) { allocate (M); view() = M; }
	//! construct a matrix by copying the elements of \a M
        template <typename U> Matrix (const MatrixView<U>& M) { allocate (M); view() = M; }
        Matrix (const Matrix<T>& M) { allocate (M); view() = M.view(); }
        template <typename U> Matrix (const Matrix<U>& M) { allocate (M); view() = M.view(); }


	//! construct a matrix by reading from the text file \a filename
        Matrix (const std::string& filename) { load (filename); } 
	//! destructor
        ~Matrix () { if (GSLMatrix<T>::block) { assert (GSLMatrix<T>::owner); GSLBlock<T>::free (GSLMatrix<T>::block); } }

	//! assignment operator: allocate matrix to have the same size as \a M, and copy contents from \a M
        Matrix<T>&       operator= (const Matrix<T>& M) { return (operator= (M.view())); }
	//! assignment operator: allocate matrix to have the same size as \a M, and copy contents from \a M
        Matrix<T>&       operator= (const MatrixView<T>& M) { allocate (M); view() = M; return (*this); }
	//! assignment operator: allocate matrix to have the same size as \a M, and copy contents from \a M
        template <typename U> Matrix& operator= (const Matrix<U>& M) { return (operator=(M.view())); }
	//! assignment operator: allocate matrix to have the same size as \a M, and copy contents from \a M
        template <typename U> Matrix& operator= (const MatrixView<U>& M) { allocate (M); view() = M; return (*this); }

	//! deallocate the matrix data
        Matrix<T>&       clear () { 
          if (GSLMatrix<T>::block) {
            assert (GSLMatrix<T>::owner);
	    GSLBlock<T>::free (GSLMatrix<T>::block);
          }
	  GSLMatrix<T>::size1 = GSLMatrix<T>::size2 = GSLMatrix<T>::tda = 0;
	  GSLMatrix<T>::data = NULL; GSLMatrix<T>::block = NULL; GSLMatrix<T>::owner = 0; 
          return (*this);
        }
	//! allocate the matrix to have the same size as \a M
        Matrix<T>&       allocate (const MatrixView<T>& M) { allocate (M.rows(), M.columns()); return (*this); }
	//! allocate the matrix to have the same size as \a M
        template <typename U> Matrix& allocate (const MatrixView<U>& M) { allocate (M.rows(), M.columns()); return (*this); }
	//! allocate the matrix to have size \a nrows by \a ncolumns
        Matrix<T>&       allocate (size_t nrows, size_t ncolumns) {
          if (GSLMatrix<T>::block) {
            assert (GSLMatrix<T>::owner);
            if (GSLMatrix<T>::block->size < nrows * ncolumns) { 
	      GSLBlock<T>::free (GSLMatrix<T>::block);
	      GSLMatrix<T>::block = NULL;
            }
          }

          if (!GSLMatrix<T>::block && nrows*ncolumns) {
            GSLMatrix<T>::block = GSLBlock<T>::alloc (nrows * ncolumns);
            if (!GSLMatrix<T>::block) throw Exception ("Failed to allocate memory for Matrix data");  
          }
	  GSLMatrix<T>::size1 = nrows; GSLMatrix<T>::size2 = GSLMatrix<T>::tda = ncolumns; GSLMatrix<T>::owner = 1;
          GSLMatrix<T>::data = GSLMatrix<T>::block ? GSLMatrix<T>::block->data : NULL;
          return (*this);
        }

	//! resize matrix to have size \a nrows by \a ncolumns
        Matrix<T>&        resize (size_t nrows, size_t ncolumns) {
          if (nrows <= GSLMatrix<T>::size1 || ncolumns <= GSLMatrix<T>::size2) { 
            GSLMatrix<T>::size1 = nrows;
            GSLMatrix<T>::size2 = ncolumns;
            return (*this); 
          }
          if (GSLMatrix<T>::block) {
            assert (GSLMatrix<T>::owner);
            if (GSLMatrix<T>::block->size >= (nrows-1)*GSLMatrix<T>::tda + ncolumns) {
              GSLMatrix<T>::size1 = nrows; 
              GSLMatrix<T>::size2 = ncolumns; 
              return (*this);
            }
          }
          Matrix M (nrows, ncolumns);
          size_t nr = MIN(GSLMatrix<T>::size1, M.GSLMatrix<T>::size1), nc = MIN(GSLMatrix<T>::size2, M.GSLMatrix<T>::size2);
          M.MatrixView<T>::view (0, nr, 0, nc) = MatrixView<T>::view (0, nr, 0, nc);
          clear();
          swap (M);
          return (*this);
        }

	//! return as MatrixView class
        MatrixView<T>& view () throw () { return (*this); }
	//! return as MatrixView class
        const MatrixView<T>& view () const throw () { return (*this); }

	//! return a MatrixView corresponding to a submatrix of the matrix
        MatrixView<T> view (size_t from_row, size_t to_row, size_t from_column, size_t to_column) throw () {
	  return (MatrixView<T>::view (from_row, to_row, from_column, to_column));
       	}
	//! return a MatrixView corresponding to a submatrix of the matrix
        const MatrixView<T> view (size_t from_row, size_t to_row, size_t from_column, size_t to_column) const throw () {
	  return (MatrixView<T>::view (from_row, to_row, from_column, to_column));
       	}

	//! read matrix data from the text file \a filename
        Matrix<T>&        load (const std::string& filename) {
          std::ifstream in (filename.c_str());
          if (!in) throw Exception ("cannot open matrix file \"" + filename + "\": " + strerror (errno));
          try { in >> *this; }
          catch (Exception E) { throw Exception ("error loading matrix file \"" + filename + "\":" + E.description); }
          return (*this);
        }


	//! read the matrix data from \a stream and assign to the matrix \a M
        friend std::istream& operator>> (std::istream& stream, Matrix<T>& M)  
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
     * \param A a MatrixView
     * \param x a VectorView
     * \return a reference to the target vector \a y
     */
    template <typename T> inline VectorView<T>& mult (VectorView<T>& y, T beta, T alpha, CBLAS_TRANSPOSE op_A, const MatrixView<T>& A, const VectorView<T>& x) 
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
     * \param A a MatrixView
     * \param x a Vector
     * \note this version will perform the appropriate allocation for \a y. 
     * \return a reference to the target vector \a y
     */
    template <typename T> inline Vector<T>& mult (Vector<T>& y, T alpha, CBLAS_TRANSPOSE op_A, const MatrixView<T>& A, const VectorView<T>& x) { 
      y.allocate (A.rows());
      mult (y.view(), T(0.0), alpha, op_A, A, x);
      return (y); 
    }

    //! Computes the matrix-vector product \a y = \a A \a x
    /** \param y the target vector
     * \param A a MatrixView
     * \param x a Vector
     * \return a reference to the target vector \a y
     */
    template <typename T> inline VectorView<T>& mult (VectorView<T>& y, const MatrixView<T>& A, const VectorView<T>& x) {
      return (mult (y, T(0.0), T(1.0), CblasNoTrans, A, x)); 
    }

    //! Simplified matrix-vector multiplication with allocation
    //! Computes the matrix-vector product \a y = \a A \a x, allocating storage for \a y
    /** \param y the target vector
     * \param A a MatrixView
     * \param x a Vector
     * \note this version will perform the appropriate allocation for \a y. 
     * \return a reference to the target vector \a y
     */
    template <typename T> inline Vector<T>& mult (Vector<T>& y, const MatrixView<T>& A, const VectorView<T>& x) {
      y.allocate (A.rows());
      mult (y.view(), A, x); 
      return (y);
    }

    /** @} */



    /** @defgroup gemm General matrix-matrix multiplication
      @{ */


    //! computes the general matrix-matrix multiplication \a C = \a alpha \a op_A (\a A) \a op_A (\a B) + \a beta \a C
    /** \param C the target matrix
     * \param beta used to add a multiple of \a C to the final product
     * \param alpha used to scale the product
     * \param op_A determines how to use the matrix \a A:
     *    - CblasNoTrans: use A
     *    - CblasTrans: use transpose of A
     *    - CblasConjTrans: use conjugate transpose of A
     * \param A a MatrixView
     * \param op_B determines how to use the matrix \a B:
     *    - CblasNoTrans: use B
     *    - CblasTrans: use transpose of B
     *    - CblasConjTrans: use conjugate transpose of B
     * \param B a MatrixView
     * \return a reference to the target matrix \a C
     */
    template <typename T> inline MatrixView<T>& mult (MatrixView<T>& C, T beta, T alpha, CBLAS_TRANSPOSE op_A, const MatrixView<T>& A, CBLAS_TRANSPOSE op_B, const MatrixView<T>& B) 
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
     * \param A a MatrixView
     * \param op_B determines how to use the matrix \a B:
     *    - CblasNoTrans: use B
     *    - CblasTrans: use transpose of B
     *    - CblasConjTrans: use conjugate transpose of B
     * \param B a MatrixView
     * \note this version will perform the appropriate allocation for \a C 
     * \return a reference to the target matrix \a C
     */
    template <typename T> inline Matrix<T>& mult (Matrix<T>& C, T alpha, CBLAS_TRANSPOSE op_A, const MatrixView<T>& A, CBLAS_TRANSPOSE op_B, const MatrixView<T>& B) { 
      C.allocate (A.rows(), B.columns());
      mult (C.view(), T(0.0), alpha, op_A, A, op_B, B);
      return (C); 
    }

    //! computes the simplified general matrix-matrix multiplication \a C = \a A \a B
    /** \param C the target matrix
     * \param A a MatrixView
     * \param B a MatrixView
     * \return a reference to the target matrix \a C
     */
    template <typename T> inline MatrixView<T>& mult (MatrixView<T>& C, const MatrixView<T>& A, const MatrixView<T>& B) { 
      return (mult (C, T(0.0), T(1.0), CblasNoTrans, A, CblasNoTrans, B)); 
    }

    //! computes the simplified general matrix-matrix multiplication \a C = \a A \a B, allocating storage for \a C
    /** \param C the target matrix
     * \param A a MatrixView
     * \param B a MatrixView
     * \note this version will perform the appropriate allocation for \a C 
     * \return a reference to the target matrix \a C
     */
    template <typename T> inline Matrix<T>& mult (Matrix<T>& C, const MatrixView<T>& A, const MatrixView<T>& B) { 
      C.allocate (A.rows(), B.columns());
      mult (C.view(), A, B); 
      return (C);
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
     * \param A a MatrixView
     * \param B a MatrixView
     * \return a reference to the target matrix \a C
     */
    template <typename T> inline MatrixView<T>& mult (MatrixView<T>& C, CBLAS_SIDE side, T beta, T alpha, CBLAS_UPLO uplo, const MatrixView<T>& A, const MatrixView<T>& B) 
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
     * \param A a MatrixView
     * \param B a MatrixView
     * \note this version will perform the appropriate allocation for \a C 
     * \return a reference to the target matrix \a C
     */
    template <typename T> inline Matrix<T>& mult (Matrix<T>& C, CBLAS_SIDE side, T alpha, CBLAS_UPLO uplo, const MatrixView<T>& A, const MatrixView<T>& B) 
    { 
      C.allocate (A);
      symm (side, uplo, alpha, A, B, T(0.0), C);
      return (C);
    }

    /** @} */



    //! solve for \a y in the triangular matrix problem: \a op_A(\a A) \a y = \a x
    /** \param x the target vector. On input, \a x should be set to \a y
     * \param A a MatrixView
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
    template <typename T> inline VectorView<T>& solve_triangular (VectorView<T>& x, const MatrixView<T>& A, CBLAS_UPLO uplo = CblasUpper, CBLAS_TRANSPOSE op_A = CblasNoTrans, CBLAS_DIAG diag = CblasNonUnit)
    {
      trsv (uplo, op_A, diag, A, x);
      return (x);
    }

    //! rank-1 update: \a A = \a alpha \a x \a y^T + \a A
    /** \param A the target matrix. 
     * \param x a VectorView
     * \param y a VectorView
     * \param alpha used to scale the product
     * \return a reference to the target matrix \a A
     */
    template <typename T> inline MatrixView<T>& rank1_update (MatrixView<T>& A, const VectorView<T>& x, const VectorView<T>& y, T alpha = 1.0)
    {
      ger (alpha, x, y, A);
      return (A);
    }

    //! symmetric rank-1 update: \a A = \a alpha \a x \a x^T + \a A, for symmetric \a A
    /** \param A the symmetric target matrix. 
     * \param x a VectorView
     * \param alpha used to scale the product
     * \param uplo determines which part of \a A will be used:
     *    - CblasUpper: upper triangle and diagonal of A
     *    - CblasLower: lower triangle and diagonal of A
     * \return a reference to the target matrix \a A
     */
    template <typename T> inline MatrixView<T>& sym_rank1_update (MatrixView<T>& A, const VectorView<T>& x, T alpha = 1.0, CBLAS_UPLO uplo = CblasUpper)
    {
      syr (uplo, alpha, x, A);
      return (A);
    }

    //! symmetric rank-N update: \a C = \a alpha \a op_A(\a A) op_A(\a A)^T + \a beta C, for symmetric \a A
    /** \param C the target matrix. If \a beta is non-zero, \a C should be symmetric
     * \param A a MatrixView
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
    template <typename T> inline MatrixView<T>& rankN_update (MatrixView<T>& C, const MatrixView<T>& A, CBLAS_TRANSPOSE op_A = CblasNoTrans, CBLAS_UPLO uplo = CblasUpper, T alpha = 1.0, T beta = 0.0)
    {
      syrk (uplo, op_A, alpha, A, beta, C);
      return (C);
    }

    //! compute transpose \a A = \a B^T
    /** \param A the target matrix. 
     * \param B a MatrixView to be transposed
     * \return a reference to the target matrix \a A
     */
    template <typename T> inline MatrixView<T>& transpose (MatrixView<T>& A, const MatrixView<T>& B)
    {
      for (size_t i = 0; i < B.rows(); i++)
        for (size_t j = 0; j < B.columns(); j++)
          A(j,i) = B(i,j);
      return (A);
    }

    //! compute transpose \a A = \a B^T
    /** \param A the target matrix. 
     * \param B a MatrixView to be transposed
     * \return a reference to the target matrix \a A
     * \note this version will perform the appropriate allocation for \a A 
     */
    template <typename T> inline Matrix<T>& transpose (Matrix<T>& A, const MatrixView<T>& B)
    {
      A.allocate (B.columns(), B.rows());
      transpose (A.view(), B);
      return (A);
    }
    /** @} */






#ifndef DOXYGEN_SHOULD_SKIP_THIS
    namespace {
      // double definitions:

      inline void gemm (CBLAS_TRANSPOSE op_A, CBLAS_TRANSPOSE op_B, double alpha, const MatrixView<double>& A, const MatrixView<double>& B, double beta, MatrixView<double>& C)
      { gsl_blas_dgemm (op_A, op_B, alpha, &A, &B, beta, &C); }

      inline void gemv (CBLAS_TRANSPOSE op_A, double alpha, const MatrixView<double>& A, const VectorView<double>& x, double beta, VectorView<double>& y)
      { gsl_blas_dgemv (op_A, alpha, &A, &x, beta, &y); }

      inline void symm (CBLAS_SIDE side, CBLAS_UPLO uplo, double alpha, const MatrixView<double>& A, const MatrixView<double>& B, double beta, MatrixView<double>& C)
       { gsl_blas_dsymm (side, uplo, alpha, &A, &B, beta, &C); }

      inline void trsv (CBLAS_UPLO uplo, CBLAS_TRANSPOSE op_A, CBLAS_DIAG diag, const MatrixView<double>& A, VectorView<double>& x)
      { gsl_blas_dtrsv (uplo, op_A, diag, &A, &x); }

      inline void ger (double alpha, const VectorView<double>& x, const VectorView<double>& y, MatrixView<double>& A)
      { gsl_blas_dger (alpha, &x, &y, &A); }

      inline void syr (CBLAS_UPLO uplo, double alpha, const VectorView<double>& x, MatrixView<double>& A)
      { gsl_blas_dsyr (uplo, alpha, &x, &A); }

      inline void syrk (CBLAS_UPLO uplo, CBLAS_TRANSPOSE op_A, double alpha, const MatrixView<double>& A, double beta, MatrixView<double>& C)
      { gsl_blas_dsyrk (uplo, op_A, alpha, &A, beta, &C); }

      // float definitions:


      inline void gemm (CBLAS_TRANSPOSE op_A, CBLAS_TRANSPOSE op_B, float alpha, const MatrixView<float>& A, const MatrixView<float>& B, float beta, MatrixView<float>& C)
      { gsl_blas_sgemm (op_A, op_B, alpha, &A, &B, beta, &C); }

      inline void gemv (CBLAS_TRANSPOSE op_A, float alpha, const MatrixView<float>& A, const VectorView<float>& x, float beta, VectorView<float>& y)
      { gsl_blas_sgemv (op_A, alpha, &A, &x, beta, &y); }

      inline void symm (CBLAS_SIDE side, CBLAS_UPLO uplo, float alpha, const MatrixView<float>& A, const MatrixView<float>& B, float beta, MatrixView<float>& C)
      { gsl_blas_ssymm (side, uplo, alpha, &A, &B, beta, &C); }

      inline void trsv (CBLAS_UPLO uplo, CBLAS_TRANSPOSE op_A, CBLAS_DIAG diag, const MatrixView<float>& A, VectorView<float>& x)
      { gsl_blas_strsv (uplo, op_A, diag, &A, &x); }

      inline void ger (float alpha, const VectorView<float>& x, const VectorView<float>& y, MatrixView<float>& A)
      { gsl_blas_sger (alpha, &x, &y, &A); }

      inline void syr (CBLAS_UPLO uplo, float alpha, const VectorView<float>& x, MatrixView<float>& A)
      { gsl_blas_ssyr (uplo, alpha, &x, &A); }

      inline void syrk (CBLAS_UPLO uplo, CBLAS_TRANSPOSE op_A, float alpha, const MatrixView<float>& A, float beta, MatrixView<float>& C)
      { gsl_blas_ssyrk (uplo, op_A, alpha, &A, beta, &C); }

    }

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

  }
}

#undef LOOP

#endif

