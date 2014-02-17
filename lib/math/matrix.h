#ifndef __math_matrix_h__
#define __math_matrix_h__

#include <fstream>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_linalg.h>

#include "ptr.h"
#include "math/math.h"
#include "math/vector.h"

#ifdef __math_complex_h__
#include <gsl/gsl_matrix_complex_double.h>
#include <gsl/gsl_matrix_complex_float.h>
#endif

//! \cond skip

#define LOOP(op) for (size_t i = 0; i < size1; i++) { for (size_t j = 0; j < size2; j++) { op; } }

//! \endcond


namespace MR
{
  namespace Math
  {

    //! \cond skip

    template <typename ValueType> class GSLMatrix;
    template <> class GSLMatrix <float> : public gsl_matrix_float
    {
      public:
        void set (float* p) {
          data = p;
        }
    };
    template <> class GSLMatrix <double> : public gsl_matrix
    {
      public:
        void set (double* p) {
          data = p;
        }
    };

#ifdef __math_complex_h__

    template <> class GSLMatrix <cfloat> : public gsl_matrix_complex_float
    {
      public:
        void set (cfloat* p) {
          data = (float*) p;
        }
    };
    template <> class GSLMatrix <cdouble> : public gsl_matrix_complex
    {
      public:
        void set (cdouble* p) {
          data = (double*) p;
        }
    };

#endif

    //! \endcond



    /** @addtogroup linalg
      @{ */

    //! A matrix class
    /*! This class is a thin wrapper around the GSL matrix classes, and can be
     * passed to existing GSL functions by pointer using the gsl() member
     * functions.
     *
     * Here are some examples:
     * \code
     * // create instance of Matrix:
     * Math::Matrix<float> M (10,10);
     *
     * // set all elements to zero:
     * M = 0.0;
     *
     * // set diagonal elements to 1 (this uses the diagonal() function,
     * // which returns a Vector::View):
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
    template <typename ValueType> 
      class Matrix : protected GSLMatrix<ValueType>
    {
      public:
        template <typename U> friend class Matrix;
        typedef ValueType value_type;
        typedef typename Vector<ValueType>::View VectorView;

        class View;

        //! construct empty matrix
        Matrix () throw () {
          size1 = size2 = tda = 0;
          data = NULL;
          block = NULL;
          owner = 1;
        }

        //! construct from View
        Matrix (const View& V) {
          Matrix<ValueType>::size1 = V.size1;
          Matrix<ValueType>::size2 = V.size2;
          Matrix<ValueType>::tda = V.tda;
          Matrix<ValueType>::set (V.data);
          Matrix<ValueType>::block = NULL;
          Matrix<ValueType>::owner = 0;
        }

        //! copy constructor
        Matrix (const Matrix& M) {
          initialize (M.rows(), M.columns());
          LOOP (operator() (i,j) = M (i,j));
        }

        //! copy constructor
        template <typename U> Matrix (const Matrix<U>& M) {
          initialize (M.rows(), M.columns());
          LOOP (operator() (i,j) = M (i,j));
        }

        //! construct matrix of size \a nrows by \a ncolumns
        /** \note the elements of the matrix are left uninitialised. */
        Matrix (size_t nrows, size_t ncolumns) {
          initialize (nrows, ncolumns);
        }

        //! construct from existing data array
        /*! \note with this constructor, the matrix is not responsible for
         * managing data allocation or deallocation. The matrix will simply
         * allow access to the data provided using the Matrix interface. The
         * underlying data array must remain accessible during the lifetime of
         * the Matrix class. */
        Matrix (ValueType* data, size_t nrows, size_t ncolumns) throw () {
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
        Matrix (ValueType* data, size_t nrows, size_t ncolumns, size_t row_skip) throw () {
          size1 = nrows;
          size2 = ncolumns;
          tda = row_skip;
          set (data);
          block = NULL;
          owner = 0;
        }

        //! construct a matrix by reading from the text file \a filename
        Matrix (const std::string& filename) {
          size1 = size2 = tda = 0;
          data = NULL;
          block = NULL;
          owner = 1;
          load (filename);
        }

        //! destructor
        ~Matrix () {
          if (block) {
            assert (owner);
            GSLBlock<ValueType>::free (block);
          }
        }

        //! deallocate the matrix data
        Matrix& clear () {
          if (block) {
            assert (owner);
            GSLBlock<ValueType>::free (block);
          }
          size1 = size2 = tda = 0;
          data = NULL;
          block = NULL;
          owner = 1;
          return *this;
        }
        //! allocate the matrix to have the same size as \a M
        Matrix& allocate (const Matrix& M) {
          allocate (M.rows(), M.columns());
          return *this;
        }
        //! allocate the matrix to have the same size as \a M
        template <typename U> Matrix& allocate (const Matrix<U>& M) {
          allocate (M.rows(), M.columns());
          return *this;
        }

        //! allocate the matrix to have size \a nrows by \a ncolumns
        Matrix& allocate (size_t nrows, size_t ncolumns) {
          if (rows() == nrows && columns() == ncolumns)
            return *this;
          if (!owner)
            throw Exception ("attempt to allocate view of a Matrix!");
          if (block) {
            if (block->size < nrows * ncolumns) {
              GSLBlock<ValueType>::free (block);
              block = NULL;
            }
          }

          if (!block && nrows*ncolumns) {
            block = GSLBlock<ValueType>::alloc (nrows * ncolumns);
            if (!block)
              throw Exception ("Failed to allocate memory for Matrix data");
          }
          size1 = nrows;
          size2 = tda = ncolumns;
          owner = 1;
          data = block ? block->data : NULL;
          return *this;
        }

        //! resize the matrix to have size \a nrows by \a ncolumns, preserving existing data
        /*! The \c fill_value argument determines what value the elements of
         * the Matrix will be set to in case the size requested exceeds the
         * current size. */
        Matrix& resize (size_t nrows, size_t ncolumns, value_type fill_value = value_type (0.0)) {
          if (!owner)
            throw Exception ("attempt to resize view of a Matrix!");
          if (nrows == 0 || ncolumns == 0) {
            size1 = size2 = 0;
            return *this;
          }
          if (nrows == size1 && ncolumns == size2) {
            return *this;
          }
          if (!block) {
            allocate (nrows, ncolumns);
            operator= (fill_value);
            return *this;
          }
          if (ncolumns > tda || nrows*tda > block->size) {
            Matrix M (nrows, ncolumns);
            M.sub (0, rows(), 0, columns()) = *this;
            M.sub (rows(), M.rows(), 0, columns()) = fill_value;
            M.sub (0, M.rows(), columns(), M.columns()) = fill_value;
            swap (M);
            return *this;
          }
          size1 = nrows;
          size2 = ncolumns;
          return *this;
        }

        //! read matrix data from the text file \a filename
        Matrix& load (const std::string& filename) {
          std::ifstream in (filename.c_str());
          if (!in)
            throw Exception ("cannot open matrix file \"" + filename + "\": " + strerror (errno));
          try {
            in >> *this;
          }
          catch (Exception& E) {
            throw Exception (E, "error loading matrix file \"" + filename + "\"");
          }
          return *this;
        }

        //! write to text file \a filename
        void save (const std::string& filename) const {
          std::ofstream out (filename.c_str());
          if (!out)
            throw Exception ("cannot open matrix file \"" + filename + "\": " + strerror (errno));
          out << *this;
        }

        //! used to obtain a pointer to the underlying GSL structure
        GSLMatrix<ValueType>* gsl () {
          return this;
        }

        //! used to obtain a pointer to the underlying GSL structure
        const GSLMatrix<ValueType>* gsl () const {
          return this;
        }

        //! assign the specified \a value to all elements of the matrix
        Matrix&  operator= (ValueType value) throw () {
          LOOP (operator() (i,j) = value);
          return *this;
        }

        //! assign the values in \a M to the corresponding elements of the matrix
        Matrix&  operator= (const Matrix& M) {
          allocate (M);
          LOOP (operator() (i,j) = M (i,j));
          return *this;
        }

        //! assign the values in \a M to the corresponding elements of the matrix
        template <typename U> Matrix& operator= (const Matrix<U>& M) {
          allocate (M);
          LOOP (operator() (i,j) = M (i,j));
          return *this;
        }

        //! comparison operator
        bool operator== (const Matrix& M) throw () {
          return ! (*this != M);
        }

        //! comparison operator
        bool operator!= (const Matrix& M) throw () {
          if (rows() != M.rows() || columns() != M.columns()) return true;
          LOOP (if (operator() (i,j) != M (i,j)) return true);
          return false;
        }

        //! swap contents with \a M without copying
        void swap (Matrix& M) throw () {
          char c [sizeof (Matrix)];
          memcpy (&c, this, sizeof (Matrix));
          memcpy (this, &M, sizeof (Matrix));
          memcpy (&M, &c, sizeof (Matrix));
        }

        //! return reference to element at \a i, \a j
        ValueType& operator() (size_t i, size_t j) throw () {
          assert (i < rows());
          assert (j < columns());
          return ptr() [i * tda + j];
        }

        //! return const reference to element at \a i, \a j
        const ValueType& operator() (size_t i, size_t j) const throw () {
          assert (i < rows());
          assert (j < columns());
          return ptr() [i * tda + j];
        }

        //! return number of rows of matrix
        size_t rows () const throw () {
          return size1;
        }

        //! return number of columns of matrix
        size_t columns () const throw () {
          return size2;
        }

        //! true if matrix points to existing data
        bool is_set () const throw () {
          return ptr();
        }

        //! return a pointer to the underlying data
        ValueType* ptr () throw () {
          return (ValueType*) (data);
        }

        //! return a pointer to the underlying data
        const ValueType* ptr () const throw () {
          return (const ValueType*) (data);
        }

        //! return the row stride
        size_t row_stride () const throw () {
          return tda;
        }

        //! set all elements of matrix to zero
        Matrix& zero () {
          LOOP (operator() (i,j) = 0.0);
          return *this;
        }

        //! set all diagonal elements of matrix to one, and all others to zero
        Matrix& identity () throw () {
          LOOP (operator() (i,j) = (i==j?1.0:0.0));
          return *this;
        }

        //! add \a value to all elements of the matrix
        Matrix& operator+= (ValueType value) throw () {
          LOOP (operator() (i,j) += value);
          return *this;
        }
        //! subtract \a value from all elements of the matrix
        Matrix& operator-= (ValueType value) throw () {
          LOOP (operator() (i,j) -= value);
          return *this;
        }
        //! multiply all elements of the matrix by \a value
        Matrix& operator*= (ValueType value) throw () {
          LOOP (operator() (i,j) *= value);
          return *this;
        }
        //! divide all elements of the matrix by \a value
        Matrix& operator/= (ValueType value) throw () {
          LOOP (operator() (i,j) /= value);
          return *this;
        }

        //! add each element of \a M to the corresponding element of the matrix
        Matrix& operator+= (const Matrix& M) throw () {
          assert (rows() == M.rows() && columns() == M.columns());
          LOOP (operator() (i,j) += M (i,j));
          return *this;
        }

        //! subtract each element of \a M from the corresponding element of the matrix
        Matrix& operator-= (const Matrix& M) throw () {
          assert (rows() == M.rows() && columns() == M.columns());
          LOOP (operator() (i,j) -= M (i,j));
          return *this;
        }

        //! multiply each element of the matrix by the corresponding element of \a M
        Matrix& operator*= (const Matrix& M) throw () {
          assert (rows() == M.rows() && columns() == M.columns());
          LOOP (operator() (i,j) *= M (i,j));
          return *this;
        }

        //! divide each element of the matrix by the corresponding element of \a M
        Matrix& operator/= (const Matrix& M) throw () {
          assert (rows() == M.rows() && columns() == M.columns());
          LOOP (operator() (i,j) /= M (i,j));
          return *this;
        }

        //! exponentiate each element of the matrix by \a power
        Matrix& pow (ValueType power) throw () {
          LOOP (operator() (i,j) = Math::pow (operator() (i,j), power));
          return *this;
        }

        //! square each element of the matrix
        Matrix& sqrt () throw () {
          LOOP (operator() (i,j) = Math::sqrt (operator() (i,j)));
          return *this;
        }

        //! check whether Matrix is a view of other data
        /*! If a matrix is a view, it will not be capable of any form of data
         * re-allocation. */
        bool is_view () const {
          return !owner;
        }

        //! return a view of the matrix
        View view () throw () {
          return View (ptr(), rows(), columns(), row_stride());
        }

        //! set current Matrix to be a view of another
        Matrix& view (const Matrix& V) throw () {
          if (block) {
            assert (owner);
            GSLBlock<ValueType>::free (block);
          }
          Matrix<ValueType>::size1 = V.size1;
          Matrix<ValueType>::size2 = V.size2;
          Matrix<ValueType>::tda = V.tda;
          Matrix<ValueType>::set (V.data);
          block = NULL;
          owner = 0;
          return *this;
        }


        //! return a Matrix::View corresponding to a submatrix of the matrix
        View sub (size_t from_row, size_t to_row, size_t from_column, size_t to_column) throw () {
          assert (from_row <= to_row && to_row <= rows());
          assert (from_column <= to_column && to_column <= columns());
          return View (ptr() + from_row*tda + from_column,
              to_row-from_row, to_column-from_column, tda);
        }

        //! return a Matrix::View corresponding to a submatrix of the matrix
        const View sub (size_t from_row, size_t to_row, size_t from_column, size_t to_column) const throw () {
          assert (from_row <= to_row && to_row <= rows());
          assert (from_column <= to_column && to_column <= columns());
          return View (const_cast<ValueType*> (ptr() + from_row*tda + from_column),
              to_row-from_row, to_column-from_column, tda);
        }

        //! return a Vector::View corresponding to a row of the matrix
        VectorView row (size_t index = 0) throw () {
          assert (index < rows());
          return VectorView (ptr() +index*tda, size2, 1);
        }
        //! return a Vector::View corresponding to a row of the matrix
        const VectorView row (size_t index = 0) const throw () {
          assert (index < rows());
          return VectorView (const_cast<ValueType*> (ptr()) +index*tda, size2, 1);
        }

        //! return a Vector::View corresponding to a column of the matrix
        VectorView column (size_t index = 0) throw () {
          assert (index < columns());
          return VectorView (ptr() +index, size1, tda);
        }

        //! return a Vector::View corresponding to a column of the matrix
        const VectorView column (size_t index = 0) const throw () {
          assert (index < columns());
          return VectorView (const_cast<ValueType*> (ptr()) +index, size1, tda);
        }

        //! return a Vector::View corresponding to the diagonal of the matrix
        VectorView diagonal () throw () {
          assert (rows() > 0 && columns() > 0);
          return VectorView (ptr(), MIN (size1,size2), tda+1);
        }

        //! return a Vector::View corresponding to the diagonal of the matrix
        const VectorView diagonal () const throw () {
          assert (rows() > 0 && columns() > 0);
          return VectorView (const_cast<ValueType*> (ptr()), MIN (size1,size2), tda+1);
        }

        //! return a Vector::View corresponding to a diagonal of the matrix
        /** \param offset the diagonal to obtain. If \a offset > 0, return the corresponding upper diagonal.
          If \a offset < 0, return the corresponding lower diagonal. */
        VectorView diagonal (int offset) throw () {
          assert (rows() > 0 && columns() > 0);
          if (offset == 0) return diagonal();
          if (offset < 0)
            return VectorView (ptr()-tda*offset,
                MIN (size1+offset,size2+offset), tda+1);
          return VectorView (ptr() +offset,
              MIN (size1-offset,size2-offset), tda+1);
        }

        //! return a Vector::View corresponding to a diagonal of the matrix
        /** \param offset the diagonal to obtain. If \a offset > 0, return the corresponding upper diagonal.
          If \a offset < 0, return the corresponding lower diagonal. */
        const VectorView diagonal (int offset) const throw () {
          assert (rows() > 0 && columns() > 0);
          if (offset == 0) return (diagonal());
          if (offset < 0)
            return VectorView (const_cast<ValueType*> (ptr()-tda*offset),
                MIN (size1+offset,size2+offset), tda+1);
          return VectorView (const_cast<ValueType*> (ptr() +offset),
              MIN (size1-offset,size2-offset), tda+1);
        }

        //! swap two rows of the matrix
        void swap_rows (size_t n, size_t m) {
          for (size_t i = 0; i < size2; i++)
            std::swap (operator() (n,i), operator() (m,i));
        }

        //! swap two columns of the matrix
        void swap_columns (size_t n, size_t m) {
          for (size_t i = 0; i < size1; i++)
            std::swap (operator() (i,n), operator() (i,m));
        }


        //! write the matrix \a M to \a stream as text
        friend std::ostream& operator<< (std::ostream& stream, const Matrix& M) {
          for (size_t i = 0; i < M.rows(); i++) {
            for (size_t j = 0; j < M.columns(); j++)
              stream << str(M(i,j), 10) << " ";
            stream << "\n";
          }
          return stream;
        }

        //! read the matrix data from \a stream and assign to the matrix \a M
        friend std::istream& operator>> (std::istream& stream, Matrix& M) {
          std::vector< RefPtr< std::vector<ValueType> > > V;
          std::string sbuf, entry;

          while (getline (stream, sbuf)) {
            sbuf = strip (sbuf.substr (0, sbuf.find_first_of ('#')));
            if (sbuf.empty()) 
              continue;

            V.push_back (RefPtr< std::vector<ValueType> > (new std::vector<ValueType>));

            std::istringstream line (sbuf);
            while (line >> entry) 
              V.back()->push_back (to<ValueType> (entry));
            if (line.bad())
              throw Exception (strerror (errno));

            if (V.size() > 1)
              if (V.back()->size() != V[0]->size())
                throw Exception ("uneven rows in matrix");
          }
          if (stream.bad()) 
            throw Exception (strerror (errno));

          if (!V.size())
            throw Exception ("no data in file");

          M.allocate (V.size(), V[0]->size());

          for (size_t i = 0; i < M.rows(); i++) {
            const std::vector<ValueType>& W (*V[i]);
            for (size_t j = 0; j < M.columns(); j++)
              M(i,j) = W[j];
          }

          return stream;
        }

      protected:
        using GSLMatrix<ValueType>::size1;
        using GSLMatrix<ValueType>::size2;
        using GSLMatrix<ValueType>::tda;
        using GSLMatrix<ValueType>::block;
        using GSLMatrix<ValueType>::owner;
        using GSLMatrix<ValueType>::data;
        using GSLMatrix<ValueType>::set;

        void initialize (size_t nrows, size_t ncolumns) {
          if (nrows && ncolumns) {
            block = GSLBlock<ValueType>::alloc (nrows * ncolumns);
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





    //! A class to reference existing Matrix data
    /*! This class is used purely to access and modify the elements of
     * existing Matrices. It cannot perform allocation/deallocation or
     * resizing operations. It is designed to be returned by members
     * functions of the Matrix classes to allow convenient access to
     * specific portions of the data (i.e. a subset of a Matrix).
     */
    template <class ValueType>
      class Matrix<ValueType>::View : public Matrix<ValueType>
      {
        public:
          View (const View& M) : Matrix<ValueType> (M) { }

          Matrix<ValueType>& operator= (ValueType value) throw () {
            return Matrix<ValueType>::operator= (value);
          }
          Matrix<ValueType>& operator= (const Matrix<ValueType>& M) throw () {
            return Matrix<ValueType>::operator= (M);
          }
          template <typename U> Matrix<ValueType>& operator= (const Matrix<U>& M) throw () {
            return Matrix<ValueType>::operator= (M);
          }

        private:
          View () {
            assert (0);
          }
          View (const Matrix<ValueType>& M) {
            assert (0);
          }
          template <typename U> View (const Matrix<U>& M) {
            assert (0);
          }
          View (ValueType* data, size_t nrows, size_t ncolumns, size_t row_skip) throw () {
            Matrix<ValueType>::size1 = nrows;
            Matrix<ValueType>::size2 = ncolumns;
            Matrix<ValueType>::tda = row_skip;
            Matrix<ValueType>::set (data);
            Matrix<ValueType>::block = NULL;
            Matrix<ValueType>::owner = 0;
          }

          friend class Matrix<ValueType>;
      };


    //! \cond skip

    namespace
    {
      // double definitions:

      inline void gemm (CBLAS_TRANSPOSE op_A, CBLAS_TRANSPOSE op_B, double alpha, const Matrix<double>& A, const Matrix<double>& B, double beta, Matrix<double>& C)
      {
        gsl_blas_dgemm (op_A, op_B, alpha, A.gsl(), B.gsl(), beta, C.gsl());
      }

      inline void gemv (CBLAS_TRANSPOSE op_A, double alpha, const Matrix<double>& A, const Vector<double>& x, double beta, Vector<double>& y)
      {
        gsl_blas_dgemv (op_A, alpha, A.gsl(), x.gsl(), beta, y.gsl());
      }

      inline void symm (CBLAS_SIDE side, CBLAS_UPLO uplo, double alpha, const Matrix<double>& A, const Matrix<double>& B, double beta, Matrix<double>& C)
      {
        gsl_blas_dsymm (side, uplo, alpha, A.gsl(), B.gsl(), beta, C.gsl());
      }

      inline void trsv (CBLAS_UPLO uplo, CBLAS_TRANSPOSE op_A, CBLAS_DIAG diag, const Matrix<double>& A, Vector<double>& x)
      {
        gsl_blas_dtrsv (uplo, op_A, diag, A.gsl(), x.gsl());
      }

      inline void ger (double alpha, const Vector<double>& x, const Vector<double>& y, Matrix<double>& A)
      {
        gsl_blas_dger (alpha, x.gsl(), y.gsl(), A.gsl());
      }

      inline void syr (CBLAS_UPLO uplo, double alpha, const Vector<double>& x, Matrix<double>& A)
      {
        gsl_blas_dsyr (uplo, alpha, x.gsl(), A.gsl());
      }

      inline void syrk (CBLAS_UPLO uplo, CBLAS_TRANSPOSE op_A, double alpha, const Matrix<double>& A, double beta, Matrix<double>& C)
      {
        gsl_blas_dsyrk (uplo, op_A, alpha, A.gsl(), beta, C.gsl());
      }

      // float definitions:


      inline void gemm (CBLAS_TRANSPOSE op_A, CBLAS_TRANSPOSE op_B, float alpha, const Matrix<float>& A, const Matrix<float>& B, float beta, Matrix<float>& C)
      {
        gsl_blas_sgemm (op_A, op_B, alpha, A.gsl(), B.gsl(), beta, C.gsl());
      }

      inline void gemv (CBLAS_TRANSPOSE op_A, float alpha, const Matrix<float>& A, const Vector<float>& x, float beta, Vector<float>& y)
      {
        gsl_blas_sgemv (op_A, alpha, A.gsl(), x.gsl(), beta, y.gsl());
      }

      inline void symm (CBLAS_SIDE side, CBLAS_UPLO uplo, float alpha, const Matrix<float>& A, const Matrix<float>& B, float beta, Matrix<float>& C)
      {
        gsl_blas_ssymm (side, uplo, alpha, A.gsl(), B.gsl(), beta, C.gsl());
      }

      inline void trsv (CBLAS_UPLO uplo, CBLAS_TRANSPOSE op_A, CBLAS_DIAG diag, const Matrix<float>& A, Vector<float>& x)
      {
        gsl_blas_strsv (uplo, op_A, diag, A.gsl(), x.gsl());
      }

      inline void ger (float alpha, const Vector<float>& x, const Vector<float>& y, Matrix<float>& A)
      {
        gsl_blas_sger (alpha, x.gsl(), y.gsl(), A.gsl());
      }

      inline void syr (CBLAS_UPLO uplo, float alpha, const Vector<float>& x, Matrix<float>& A)
      {
        gsl_blas_ssyr (uplo, alpha, x.gsl(), A.gsl());
      }

      inline void syrk (CBLAS_UPLO uplo, CBLAS_TRANSPOSE op_A, float alpha, const Matrix<float>& A, float beta, Matrix<float>& C)
      {
        gsl_blas_ssyrk (uplo, op_A, alpha, A.gsl(), beta, C.gsl());
      }


#ifdef __math_complex_h__

      // cdouble definitions:

      inline void gemm (CBLAS_TRANSPOSE op_A, CBLAS_TRANSPOSE op_B, cdouble alpha, const Matrix<cdouble>& A, const Matrix<cdouble>& B, cdouble beta, Matrix<cdouble>& C)
      {
        gsl_blas_zgemm (op_A, op_B, gsl(alpha), A.gsl(), B.gsl(), gsl(beta), C.gsl());
      }

      inline void gemv (CBLAS_TRANSPOSE op_A, cdouble alpha, const Matrix<cdouble>& A, const Vector<cdouble>& x, cdouble beta, Vector<cdouble>& y)
      {
        gsl_blas_zgemv (op_A, gsl(alpha), A.gsl(), x.gsl(), gsl(beta), y.gsl());
      }

      inline void symm (CBLAS_SIDE side, CBLAS_UPLO uplo, cdouble alpha, const Matrix<cdouble>& A, const Matrix<cdouble>& B, cdouble beta, Matrix<cdouble>& C)
      {
        gsl_blas_zsymm (side, uplo, gsl(alpha), A.gsl(), B.gsl(), gsl(beta), C.gsl());
      }

      inline void trsv (CBLAS_UPLO uplo, CBLAS_TRANSPOSE op_A, CBLAS_DIAG diag, const Matrix<cdouble>& A, Vector<cdouble>& x)
      {
        gsl_blas_ztrsv (uplo, op_A, diag, A.gsl(), x.gsl());
      }

      inline void syrk (CBLAS_UPLO uplo, CBLAS_TRANSPOSE op_A, cdouble alpha, const Matrix<cdouble>& A, cdouble beta, Matrix<cdouble>& C)
      {
        gsl_blas_zherk (uplo, op_A, alpha.real(), A.gsl(), beta.real(), C.gsl());
      }

      // float definitions:

      inline void gemm (CBLAS_TRANSPOSE op_A, CBLAS_TRANSPOSE op_B, cfloat alpha, const Matrix<cfloat>& A, const Matrix<cfloat>& B, cfloat beta, Matrix<cfloat>& C)
      {
        gsl_blas_cgemm (op_A, op_B, gsl(alpha), A.gsl(), B.gsl(), gsl(beta), C.gsl());
      }

      inline void gemv (CBLAS_TRANSPOSE op_A, cfloat alpha, const Matrix<cfloat>& A, const Vector<cfloat>& x, cfloat beta, Vector<cfloat>& y)
      {
        gsl_blas_cgemv (op_A, gsl(alpha), A.gsl(), x.gsl(), gsl(beta), y.gsl());
      }

      inline void symm (CBLAS_SIDE side, CBLAS_UPLO uplo, cfloat alpha, const Matrix<cfloat>& A, const Matrix<cfloat>& B, cfloat beta, Matrix<cfloat>& C)
      {
        gsl_blas_csymm (side, uplo, gsl(alpha), A.gsl(), B.gsl(), gsl(beta), C.gsl());
      }

      inline void trsv (CBLAS_UPLO uplo, CBLAS_TRANSPOSE op_A, CBLAS_DIAG diag, const Matrix<cfloat>& A, Vector<cfloat>& x)
      {
        gsl_blas_ctrsv (uplo, op_A, diag, A.gsl(), x.gsl());
      }

      inline void syrk (CBLAS_UPLO uplo, CBLAS_TRANSPOSE op_A, cfloat alpha, const Matrix<cfloat>& A, cfloat beta, Matrix<cfloat>& C)
      {
        gsl_blas_cherk (uplo, op_A, alpha.real(), A.gsl(), beta.real(), C.gsl());
      }

#endif

    }
    //! \endcond






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
    template <typename ValueType> 
      inline Vector<ValueType>& mult (Vector<ValueType>& y, ValueType beta, ValueType alpha, CBLAS_TRANSPOSE op_A, const Matrix<ValueType>& A, const Vector<ValueType>& x)
      {
        gemv (op_A, alpha, A, x, beta, y);
        return y;
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
    template <typename ValueType> 
      inline Vector<ValueType>& mult (Vector<ValueType>& y, ValueType alpha, CBLAS_TRANSPOSE op_A, const Matrix<ValueType>& A, const Vector<ValueType>& x)
      {
        y.allocate (op_A == CblasNoTrans ? A.rows() : A.columns());
        mult (y, ValueType (0.0), alpha, op_A, A, x);
        return y;
      }

    //! Computes the matrix-vector product \a y = \a A \a x
    /** \param y the target vector
     * \param A a Matrix
     * \param x a Vector
     * \return a reference to the target vector \a y
     */
    template <typename ValueType> 
      inline Vector<ValueType>& mult (Vector<ValueType>& y, const Matrix<ValueType>& A, const Vector<ValueType>& x)
      {
        return mult (y, ValueType (1.0), CblasNoTrans, A, x);
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
    template <typename ValueType> 
      inline Matrix<ValueType>& mult (Matrix<ValueType>& C, ValueType beta, ValueType alpha, CBLAS_TRANSPOSE op_A, const Matrix<ValueType>& A, CBLAS_TRANSPOSE op_B, const Matrix<ValueType>& B)
      {
        gemm (op_A, op_B, alpha, A, B, beta, C);
        return C;
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
    template <typename ValueType> 
      inline Matrix<ValueType>& mult (Matrix<ValueType>& C, ValueType alpha, CBLAS_TRANSPOSE op_A, const Matrix<ValueType>& A, CBLAS_TRANSPOSE op_B, const Matrix<ValueType>& B)
      {
        C.allocate (op_A == CblasNoTrans ? A.rows() : A.columns(), op_B == CblasNoTrans ? B.columns() : B.rows());
        mult (C, ValueType (0.0), alpha, op_A, A, op_B, B);
        return C;
      }

    //! computes the simplified general matrix-matrix multiplication \a C = \a A \a B
    /** \param C the target matrix
     * \param A a Matrix
     * \param B a Matrix
     * \return a reference to the target matrix \a C
     */
    template <typename ValueType> 
      inline Matrix<ValueType>& mult (Matrix<ValueType>& C, const Matrix<ValueType>& A, const Matrix<ValueType>& B)
      {
        return mult (C, ValueType (1.0), CblasNoTrans, A, CblasNoTrans, B);
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
    template <typename ValueType> 
      inline Matrix<ValueType>& mult (Matrix<ValueType>& C, CBLAS_SIDE side, ValueType beta, ValueType alpha, CBLAS_UPLO uplo, const Matrix<ValueType>& A, const Matrix<ValueType>& B)
      {
        symm (side, uplo, alpha, A, B, beta, C);
        return C;
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
    template <typename ValueType> 
      inline Matrix<ValueType>& mult (Matrix<ValueType>& C, CBLAS_SIDE side, ValueType alpha, CBLAS_UPLO uplo, const Matrix<ValueType>& A, const Matrix<ValueType>& B)
      {
        C.allocate (A);
        symm (side, uplo, alpha, A, B, ValueType (0.0), C);
        return C;
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
    template <typename ValueType> 
      inline Vector<ValueType>& solve_triangular (Vector<ValueType>& x, const Matrix<ValueType>& A, CBLAS_UPLO uplo = CblasUpper, CBLAS_TRANSPOSE op_A = CblasNoTrans, CBLAS_DIAG diag = CblasNonUnit)
      {
        trsv (uplo, op_A, diag, A, x);
        return x;
      }

    //! rank-1 update: \a A = \a alpha \a x \a y^ValueType + \a A
    /** \param A the target matrix.
     * \param x a Vector
     * \param y a Vector
     * \param alpha used to scale the product
     * \return a reference to the target matrix \a A
     */
    template <typename ValueType> 
      inline Matrix<ValueType>& rank1_update (Matrix<ValueType>& A, const Vector<ValueType>& x, const Vector<ValueType>& y, ValueType alpha = 1.0)
      {
        ger (alpha, x, y, A);
        return A;
      }

    //! symmetric rank-1 update: \a A = \a alpha \a x \a x^ValueType + \a A, for symmetric \a A
    /** \param A the symmetric target matrix.
     * \param x a Vector
     * \param alpha used to scale the product
     * \param uplo determines which part of \a A will be used:
     *    - CblasUpper: upper triangle and diagonal of A
     *    - CblasLower: lower triangle and diagonal of A
     * \return a reference to the target matrix \a A
     */
    template <typename ValueType> 
      inline Matrix<ValueType>& sym_rank1_update (Matrix<ValueType>& A, const Vector<ValueType>& x, ValueType alpha = 1.0, CBLAS_UPLO uplo = CblasUpper)
      {
        syr (uplo, alpha, x, A);
        return A;
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
    template <typename ValueType> 
      inline Matrix<ValueType>& rankN_update (Matrix<ValueType>& C, const Matrix<ValueType>& A, CBLAS_TRANSPOSE op_A = CblasNoTrans, CBLAS_UPLO uplo = CblasUpper, ValueType alpha = 1.0, ValueType beta = 0.0)
      {
        syrk (uplo, op_A, alpha, A, beta, C);
        return C;
      }

    //! compute transpose \a A = \a B^ValueType
    /** \param A the target matrix.
     * \param B a Matrix to be transposed
     * \return a reference to the target matrix \a A
     * \note this version will perform the appropriate allocation for \a A
     */
    template <typename ValueType> 
      inline Matrix<ValueType>& transpose (Matrix<ValueType>& A, const Matrix<ValueType>& B)
      {
        A.allocate (B.columns(), B.rows());
        for (size_t i = 0; i < B.rows(); i++)
          for (size_t j = 0; j < B.columns(); j++)
            A (j,i) = B (i,j);
        return A;
      }



    //! compute transpose \a A = \a B^ValueType
    /** \param B a Matrix to be transposed
     * \return the transposed matrix 
     */
    template <typename ValueType> 
      inline Matrix<ValueType> transpose (const Matrix<ValueType>& B)
      {
        Math::Matrix<ValueType> A;
        return transpose (A, B);
      }
    /** @} */


    //! compute determinant of \a A
    /** \param A input matrix
     * \return the determinant
     */
    template <typename ValueType>
      inline ValueType determinant (const Matrix<ValueType>& A)
      {
        if (A.rows() != A.columns())
          throw Exception ("determinant is only defined for square matrices");
        int sign = 0;
        ValueType det = 0.0;
        gsl_permutation* p = gsl_permutation_calloc (A.rows());
        Matrix<double> temp (A);
        int* signum = &sign;
        gsl_linalg_LU_decomp (temp.gsl(), p, signum);
        det = gsl_linalg_LU_det (temp.gsl(), *signum);
        gsl_permutation_free (p);
        return det;
      }
    /** @} */



  }
}

#undef LOOP

#endif

