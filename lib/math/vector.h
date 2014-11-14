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

#ifndef __math_vector_h__
#define __math_vector_h__

#include <gsl/gsl_vector.h>
#include <gsl/gsl_vector_float.h>

#include "mrtrix.h"
#include "debug.h"
#include "file/ofstream.h"
#include "math/math.h"

#ifdef __math_complex_h__
#include <gsl/gsl_vector_complex_double.h>
#include <gsl/gsl_vector_complex_float.h>
#endif

#define LOOP(op) for (size_t i = 0; i < size(); i++) { op; }

namespace MR
{
  namespace Math
  {

    //! \cond skip

    template <typename ValueType> class GSLVector;

    template <> class GSLVector <float> : public gsl_vector_float
    {
      public:
        void set (float* p) {
          data = p;
        }
    };
    template <> class GSLVector <double> : public gsl_vector
    {
      public:
        void set (double* p) {
          data = p;
        }
    };

    template <typename ValueType> class GSLBlock;

    template <> class GSLBlock <float> : public gsl_block_float
    {
      public:
        static gsl_block_float* alloc (size_t n) {
          return gsl_block_float_alloc (n);
        }
        static void free (gsl_block_float* p) {
          gsl_block_float_free (p);
        }
    };

    template <> class GSLBlock <double> : public gsl_block
    {
      public:
        static gsl_block* alloc (size_t n) {
          return gsl_block_alloc (n);
        }
        static void free (gsl_block* p) {
          gsl_block_free (p);
        }
    };



#ifdef __math_complex_h__

    template <> class GSLVector <cfloat> : public gsl_vector_complex_float
    {
      public:
        void set (cfloat* p) {
          data = (float*) p;
        }
    };
    template <> class GSLVector <cdouble> : public gsl_vector_complex
    {
      public:
        void set (cdouble* p) {
          data = (double*) p;
        }
    };


    template <> class GSLBlock <cfloat> : public gsl_block_complex_float
    {
      public:
        static gsl_block_complex_float* alloc (size_t n) {
          return (gsl_block_complex_float_alloc (n));
        }
        static void free (gsl_block_complex_float* p) {
          gsl_block_complex_float_free (p);
        }
    };
    template <> class GSLBlock <cdouble> : public gsl_block_complex
    {
      public:
        static gsl_block_complex* alloc (size_t n) {
          return (gsl_block_complex_alloc (n));
        }
        static void free (gsl_block_complex* p) {
          gsl_block_complex_free (p);
        }
    };
#endif



    template <typename U> class Matrix;

    //! \endcond




    /** @defgroup linalg Linear Algebra
      Provides classes and function to perform basic linear algebra operations.
      The main classes are the Vector and Matrix class, and their associated views.

      @{ */

    //! provides access to data as a vector
    /*! This class is a thin wrapper around the GSL vector classes, and can be
     * passed to existing GSL functions by pointer using the gsl() member
     * functions.
     *
     * Here are some examples:
     * \code
     * using namespace Math;
     *
     * Vector<ValueType> V (10);
     * // set all elements to zero:
     * V = 0.0;
     * // V is now [ 0 0 0 0 0 0 0 0 0 0 ]
     *
     * // set 4th & 5th elements to 1:
     * V.sub (3,5) = 1.0;
     * // V is now [ 0 0 0 1 1 0 0 0 0 0 ]
     *
     * // add 2 to elements 5 through to 8:
     * V.sub (4,8) += 2.0;
     * // V is now [ 0 0 0 1 3 2 2 2 0 0 ]
     *
     * // get view of subvector from 6th to last element:
     * Vector<ValueType> U = V.sub (5,10);
     * // U is [ 2 2 2 0 0 ]
     *
     * // add 5 to U:
     * U += 5.0;
     * // U is now [ 7 7 7 5 5 ]
     * // V is now [ 0 0 0 1 3 7 7 7 5 5 ]
     *
     * // add subvector of elements 7->9 to subvector of elements 3->5:
     * V.sub (2,5) += V.sub(6,9);
     * // V is now [ 0 0 7 8 8 7 7 7 5 5 ]
     * \endcode
     */
    template <typename ValueType> class Vector : protected GSLVector<ValueType>
    {
      public:
        template <typename U> friend class Vector;
        typedef ValueType value_type;

        class View;


        //! construct empty vector
        Vector () throw () {
          GSLVector<ValueType>::size = GSLVector<ValueType>::stride = 0;
          data = NULL;
          block = NULL;
          owner = 1;
        }

        //! construct from View
        Vector (const View& V) {
          GSLVector<ValueType>::size = V.size();
          GSLVector<ValueType>::stride = V.stride();
          data = V.data;
          block = NULL;
          owner = 0;
        }

        //! copy constructor
        Vector (const Vector& V) {
          initialize (V.size());
          LOOP (operator[] (i) = V[i]);
        }

        //! copy constructor
        template <typename U> Vector (const Vector<U>& V) {
          initialize (V.size());
          LOOP (operator[] (i) = V[i]);
        }

        //! construct vector of size \a nelements
        /** \note the elements of the vector are left uninitialised. */
        Vector (size_t nelements) {
          initialize (nelements);
        }

        //! construct from existing data array
        Vector (ValueType* vector_data, size_t nelements, size_t skip = 1) throw () {
          GSLVector<ValueType>::size = nelements;
          GSLVector<ValueType>::stride = skip;
          GSLVector<ValueType>::set (vector_data);
          block = NULL;
          owner = 0;
        }

        //! construct a vector by reading from the text file \a filename
        Vector (const std::string& file) {
          GSLVector<ValueType>::size = GSLVector<ValueType>::stride = 0;
          data = NULL;
          block = NULL;
          owner = 1;
          load (file);
        }

        //! destructor
        ~Vector () {
          if (block) {
            assert (owner);
            GSLBlock<ValueType>::free (block);
          }
        }

        //! deallocate the vector data
        Vector& clear () {
          if (block) {
            assert (owner);
            GSLBlock<ValueType>::free (block);
          }
          GSLVector<ValueType>::size = 0;
          GSLVector<ValueType>::stride = 1;
          data = NULL;
          block = NULL;
          owner = 1;
          return *this;
        }

        //! allocate the vector to have the same size as \a V
        Vector& allocate (const Vector& V) {
          return (allocate (V.size()));
        }

        //! allocate the vector to have the same size as \a V
        template <typename U> Vector& allocate (const Vector<U>& V) {
          return (allocate (V.size()));
        }

        //! allocate the vector to have size \a nelements
        Vector& allocate (size_t nelements) {
          if (nelements == size()) return *this;
          if (!owner) {
            FAIL ("attempt to allocate a view of a Vector!");
            abort();
          }
          if (block) {
            if (block->size < nelements) {
              GSLBlock<ValueType>::free (block);
              block = NULL;
            }
          }
          if (!block && nelements) {
            block = GSLBlock<ValueType>::alloc (nelements);
            if (!block)
              throw Exception ("Failed to allocate memory for Vector data");
          }
          GSLVector<ValueType>::size = nelements;
          GSLVector<ValueType>::stride = 1;
          owner = 1;
          data = block ? block->data : NULL;
          return *this;
        }

        //! resize the vector to have size \a nelements, preserving existing data
        /*! The \c fill_value argument determines what value the elements of
         * the Vector will be set to in case the size requested exceeds the
         * current size. */
        Vector& resize (size_t nelements, value_type fill_value = 0.0) {
          if (!owner) {
            FAIL ("attempt to resize a view of a Vector!");
            abort();
          }
          if (nelements == size())
            return *this;
          if (nelements < size()) {
            GSLVector<ValueType>::size = nelements;
            return *this;
          }
          if (!block || nelements*stride() > (block ? block->size : 0)) {
            Vector V (nelements);
            V.sub (0, size()) = *this;
            V.sub (size(), V.size()) = fill_value;
            swap (V);
            return *this;
          }
          GSLVector<ValueType>::size = nelements;
          return *this;
        }

        //! read vector data from the text file \a filename
        Vector& load (const std::string& filename) {
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
          File::OFStream out (filename);
          out << *this;
        }


        //! used to obtain a pointer to the underlying GSL structure
        GSLVector<ValueType>* gsl () {
          return this;
        }
        //! used to obtain a pointer to the underlying GSL structure
        const GSLVector<ValueType>* gsl () const {
          return this;
        }

        //! true if vector points to existing data
        bool is_set () const throw () {
          return ptr();
        }
        //! returns number of elements of vector
        size_t size () const throw ()  {
          return GSLVector<ValueType>::size;
        }

        //! returns a reference to the element at \a i
        ValueType& operator[] (size_t i) throw ()          {
          return ptr() [i*stride()];
        }

        //! returns a reference to the element at \a i
        const ValueType& operator[] (size_t i) const throw ()    {
          return ptr() [i*stride()];
        }

        //! return a pointer to the underlying data
        ValueType* ptr () throw () {
          return (ValueType*) (data);
        }

        //! return a pointer to the underlying data
        const ValueType* ptr () const throw () {
          return (const ValueType*) (data);
        }

        //! return the stride of the vector
        size_t stride () const throw () {
          return GSLVector<ValueType>::stride;
        }

        //! assign the specified \a value to all elements of the vector
        Vector& operator= (ValueType value) throw () {
          LOOP (operator[] (i) = value);
          return *this;
        }

        //! assign the values in \a V to the corresponding elements of the vector
        Vector& operator= (const Vector& V) {
          allocate (V);
          LOOP (operator[] (i) = V[i]);
          return *this;
        }

        //! assign the values in \a V to the corresponding elements of the vector
        template <typename U> Vector& operator= (const Vector<U>& V) {
          allocate (V);
          LOOP (operator[] (i) = V[i]);
          return *this;
        }


        //! compare the value to the corresponding elements of the vector
        bool operator!= (ValueType value) const throw () {
          LOOP (if (operator[] (i) != value) return true);
          return false;
        }
        //! compare the values in \a V to the corresponding elements of the vector
        bool operator!= (const Vector& V) const throw () {
          if (size() != V.size()) return true;
          LOOP (if (operator[] (i) != V[i]) return true);
          return false;
        }
        //! compare the values in \a V to the corresponding elements of the vector
        template <typename U> bool operator!= (const Vector<U>& V) const throw () {
          if (size() != V.size()) return true;
          LOOP (if (operator[] (i) != V[i]) return true);
          return false;
        }

        //! compare the value to the corresponding elements of the vector
        bool operator== (ValueType value) const throw () { return !(*this != value); }
        //! compare the values in \a V to the corresponding elements of the vector
        bool operator== (const Vector& V) const throw () { return !(*this != V); }
        //! compare the values in \a V to the corresponding elements of the vector
        template <typename U> bool operator== (const Vector<U>& V) const { return !(*this != V); }


        //! set all elements of vector to zero
        Vector& zero () throw () {
          LOOP (operator[] (i) = 0.0);
          return *this;
        }

        //! swap contents with \a V without copying
        void swap (Vector& V) throw () {
          char c [sizeof (Vector)];
          memcpy (&c, this, sizeof (Vector));
          memcpy (this, &V, sizeof (Vector));
          memcpy (&V, &c, sizeof (Vector));
        }

        //! add \a value to all elements of the vector
        Vector& operator+= (ValueType value) throw () {
          LOOP (operator[] (i) += value);
          return *this;
        }
        //! subtract \a value from all elements of the vector
        Vector& operator-= (ValueType value) throw () {
          LOOP (operator[] (i) -= value);
          return *this;
        }
        //! multiply all elements of the vector by \a value
        Vector& operator*= (ValueType value) throw () {
          LOOP (operator[] (i) *= value);
          return *this;
        }
        //! divide all elements of the vector by \a value
        Vector& operator/= (ValueType value) throw () {
          LOOP (operator[] (i) /= value);
          return *this;
        }

        //! add each element of \a V to the corresponding element of the vector
        Vector& operator+= (const Vector& V) throw () {
          LOOP (operator[] (i) += V[i]);
          return *this;
        }
        //! subtract each element of \a V from the corresponding element of the vector
        Vector& operator-= (const Vector& V) throw () {
          LOOP (operator[] (i) -= V[i]);
          return *this;
        }
        //! multiply each element of \a V by the corresponding element of the vector
        Vector& operator*= (const Vector& V) throw () {
          LOOP (operator[] (i) *= V[i]);
          return *this;
        }
        //! divide each element of \a V by the corresponding element of the vector
        Vector& operator/= (const Vector& V) throw () {
          LOOP (operator[] (i) /= V[i]);
          return *this;
        }


        //! check whether Vector is a view of other data
        /*! If a vector is a view, it will not be capable of any form of data
         * re-allocation. */
        bool is_view () const {
          return !owner;
        }

        //! return a view of the vector
        View view () throw () {
          return View (ptr(), size(), stride());
        }

        //! set current Vector to be a view of another
        Vector& view (const Vector& V) throw () {
          if (block) {
            assert (owner);
            GSLBlock<ValueType>::free (block);
          }
          GSLVector<ValueType>::size = V.size();
          GSLVector<ValueType>::stride = V.stride();
          data = V.data;
          block = NULL;
          owner = 0;
          return *this;
        }

        //! return a subvector of the vector
        View sub (size_t from, size_t to) throw () {
          assert (from <= to && to <= size());
          return View (ptr() + from*stride(), to-from, stride());
        }

        //! return a subvector of the vector
        const View sub (size_t from, size_t to) const throw () {
          assert (from <= to && to <= size());
          return View (const_cast<ValueType*> (ptr()) + from*stride(), to-from, stride());
        }

        //! return a subvector of the vector
        View sub (size_t from, size_t to, size_t skip) throw () {
          assert (from <= to && to <= size());
          return View (ptr() + from*stride(), ceil<size_t> ( (to-from) /float (skip)), stride() *skip);
        }

        //! return a subvector of the vector
        const View sub (size_t from, size_t to, size_t skip) const throw () {
          assert (from <= to && to <= size());
          return View (ptr() + from*stride(), ceil<size_t> ( (to-from) /float (skip)), stride() *skip);
        }

        //! write the vector \a V to \a stream as text
        friend std::ostream& operator<< (std::ostream& stream, const Vector& V) {
          for (size_t i = 0; i < V.size() - 1; i++)
            stream << str(V[i], 10) << " ";
          stream << str(V[V.size() - 1], 10);
          return stream;
        }

        //! read the vector data from \a stream and assign to the vector \a V
        friend std::istream& operator>> (std::istream& stream, Vector& V) {
          std::vector<ValueType> vec;
          std::string entry;
          while (stream >> entry) 
            vec.push_back (to<ValueType> (entry));
          if (stream.bad()) 
            throw Exception (strerror (errno));

          V.allocate (vec.size());
          for (size_t n = 0; n < V.size(); n++)
            V[n] = vec[n];
          return stream;
        }



        const ValueType* begin () const { return ptr(); }
        ValueType* begin () { return ptr(); }

        const ValueType* end () const { return ptr()+size(); }
        ValueType* end () { return ptr()+size(); }


      protected:
        using GSLVector<ValueType>::data;
        using GSLVector<ValueType>::block;
        using GSLVector<ValueType>::owner;

        void initialize (size_t nelements) {
          if (nelements) {
            block = GSLBlock<ValueType>::alloc (nelements);
            if (!block)
              throw Exception ("Failed to allocate memory for Vector data");
            GSLVector<ValueType>::size = nelements;
            GSLVector<ValueType>::stride = 1;
            data = block->data;
            owner = 1;
          } 
          else {
            GSLVector<ValueType>::size = 0;
            GSLVector<ValueType>::stride = 1;
            data = NULL;
            block = NULL;
            owner = 1;
          }
        }
    };







    //! A class to reference existing Vector data
    /*! This class is used purely to access and modify the elements of
     * existing Vectors. It cannot perform allocation/deallocation or
     * resizing operations. It is designed to be returned by members
     * functions of the Vector and Matrix classes to allow convenient
     * access to specific portions of the data (e.g. a row of a Matrix,
     * etc.).
     */
    template <class ValueType>
      class Vector<ValueType>::View : public Vector<ValueType>
      {
        public:
          View (const View& V) : Vector<ValueType> (V) { }

          Vector<ValueType>& operator= (ValueType value) throw () {
            return Vector<ValueType>::operator= (value);
          }
          Vector<ValueType>& operator= (const Vector<ValueType>& V) {
            return Vector<ValueType>::operator= (V);
          }
          template <typename U> Vector<ValueType>& operator= (const Vector<U>& V) {
            return Vector<ValueType>::operator= (V);
          }

        private:
          View () {
            assert (0);
          }
          View (const Vector<ValueType>& V) {
            assert (0);
          }
          template <typename U> View (const Vector<U>& V) {
            assert (0);
          }

          View (ValueType* vector_data, size_t nelements, size_t skip = 1) throw () {
            GSLVector<ValueType>::size = nelements;
            GSLVector<ValueType>::stride = skip;
            GSLVector<ValueType>::set (vector_data);
            GSLVector<ValueType>::block = NULL;
            GSLVector<ValueType>::owner = 0;
          }

          friend class Vector<ValueType>;
          friend class Matrix<ValueType>;
      };

    /** @defgroup vector Vector functions
      @{ */

    //! compute the squared 2-norm of a vector
    template <typename ValueType> inline ValueType norm2 (const ValueType* V, size_t size = 3, size_t stride = 1)
    {
      ValueType n = 0.0;
      for (size_t i = 0; i < size; i++) n += pow2 (V[i*stride]);
      return n;
    }

    //! compute the squared 2-norm of a vector
    template <typename ValueType> inline ValueType norm2 (const Vector<ValueType>& V)
    {
      return norm2 (V.ptr(), V.size(), V.stride());
    }

    //! compute the 2-norm of a vector
    template <typename ValueType> inline ValueType norm (const ValueType* V, size_t size = 3, size_t stride = 1)
    {
      return sqrt (norm2 (V, size, stride));
    }

    //! compute the 2-norm of a vector
    template <typename ValueType> inline ValueType norm (const Vector<ValueType>& V)
    {
      return norm (V.ptr(), V.size(), V.stride());
    }

    //! compute the squared 2-norm of the difference between two vectors
    template <typename ValueType> inline ValueType norm_diff2 (const ValueType* x, const ValueType* y, size_t size = 3, size_t x_stride = 1, size_t y_stride = 1)
    {
      ValueType n = 0.0;
      for (size_t i = 0; i < size; i++) n += pow2 (x[i*x_stride] - y[i*y_stride]);
      return n;
    }

    //! compute the squared 2-norm of the difference between two vectors
    template <typename ValueType> inline ValueType norm_diff2 (const Vector<ValueType>& x, const Vector<ValueType>& y)
    {
      return norm_diff2 (x.ptr(), y.ptr(), x.size(), x.stride(), y.stride());
    }

    //! compute the sum of the elements of a vector
    template <typename ValueType> inline ValueType sum (const ValueType* V, size_t size = 3, size_t stride = 1)
    {
      ValueType n = 0.0;
      for (size_t i = 0; i < size; i++)
        n += V[i*stride];
      return n;
    }

    //! compute the sum of the elements of a vector
    template <typename ValueType> inline ValueType sum (const Vector<ValueType>& V)
    {
      return sum (V.ptr(), V.size(), V.stride());
    }

    //! compute the mean of the elements of a vector
    template <typename ValueType> inline ValueType mean (const ValueType* V, size_t size = 3, size_t stride = 1)
    {
      return sum(V, size, stride)/size;
    }

    //! compute the mean of the elements of a vector
    template <typename ValueType> inline ValueType mean (const Vector<ValueType>& V)
    {
      return mean (V.ptr(), V.size(), V.stride());
    }

    //! normalise a vector to have unit 2-norm
    template <typename ValueType> inline void normalise (ValueType* V, size_t size = 3, size_t stride = 1)
    {
      ValueType n = norm (V, size, stride);
      for (size_t i = 0; i < size; i++)
        V[i*stride] /= n;
    }

    //! normalise a vector to have unit 2-norm
    template <typename ValueType> inline Vector<ValueType>& normalise (Vector<ValueType>& V)
    {
      normalise (V.ptr(), V.size(), V.stride());
      return V;
    }

    //! compute the dot product between two vectors
    template <typename ValueType> inline ValueType dot (const ValueType* x, const ValueType* y, size_t size = 3, size_t x_stride = 1, size_t y_stride = 1)
    {
      ValueType retval = 0.0;
      for (size_t i = 0; i < size; i++)
        retval += x[i*x_stride] * y[i*y_stride];
      return retval;
    }

    //! compute the dot product between two vectors
    template <typename ValueType> inline ValueType dot (const Vector<ValueType>& x, const Vector<ValueType>& y)
    {
      return dot (x.ptr(), y.ptr(), x.size(), x.stride(), y.stride());
    }

    //! compute the cross product between two vectors
    template <typename ValueType> inline void cross (ValueType* c, const ValueType* x, const ValueType* y, 
        size_t c_stride = 1, size_t x_stride = 1, size_t y_stride = 1)
    {
      c[0] = x[x_stride]*y[2*y_stride] - x[2*x_stride]*y[y_stride];
      c[c_stride] = x[2*x_stride]*y[0] - x[0]*y[2*y_stride];
      c[2*c_stride] = x[0]*y[y_stride] - x[x_stride]*y[0];
    }

    //! compute the cross product between two vectors
    template <typename ValueType> inline Vector<ValueType>& cross (Vector<ValueType>& c, const Vector<ValueType>& x, const Vector<ValueType>& y)
    {
      cross (c.ptr(), x.ptr(), y.ptr(), c.stride(), x.stride(), y.stride());
      return c;
    }


    //! find the maximum value of any elements within a vector
    template <typename ValueType> inline ValueType max (const Vector<ValueType>& V, size_t& i)
    {
      ValueType val (V[0]);
      i = 0;
      for (size_t j = 0; j < V.size(); j++) {
        if (val < V[j]) {
          val = V[j];
          i = j;
        }
      }
      return val;
    }

    //! find the minimum value of any elements within a vector
    template <typename ValueType> inline ValueType min (const Vector<ValueType>& V, size_t& i)
    {
      ValueType val (V[0]);
      i = 0;
      for (size_t j = 0; j < V.size(); j++) {
        if (val > V[j]) {
          val = V[j];
          i = j;
        }
      }
      return val;
    }


    //! find the maximum absolute value of any elements within a vector
    template <typename ValueType> inline ValueType absmax (const Vector<ValueType>& V, size_t& i)
    {
      ValueType val (std::abs (V[0]));
      i = 0;
      for (size_t j = 0; j < V.size(); j++) {
        if (val < std::abs (V[j])) {
          val = std::abs (V[j]);
          i = j;
        }
      }
      return val;
    }

    //! find the maximum value of any elements within a vector
    template <typename ValueType> inline ValueType max (const Vector<ValueType>& V)
    {
      size_t i;
      return max (V, i);
    }

    //! find the minimum value of any elements within a vector
    template <typename ValueType> inline ValueType min (const Vector<ValueType>& V)
    {
      size_t i;
      return min (V, i);
    }


    //! find the maximum absolute value of any elements within a vector
    template <typename ValueType> inline ValueType absmax (const Vector<ValueType>& V)
    {
      size_t i;
      return absmax (V, i);
    }

    /** @} */

    /** @} */
  }
}

#undef LOOP

#endif
