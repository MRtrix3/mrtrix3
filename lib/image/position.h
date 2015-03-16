/*
    Copyright 2009 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 07/01/10.

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

#ifndef __image_position_h__
#define __image_position_h__

namespace MR
{
  namespace Image
  {

    /*! \brief a class to simplify the implementation of DataSet classes with
     * non-trivial access to their position.
     *
     * This class provides a means of returning a modifiable voxel position
     * from a class, when the process of modifying the position cannot be done
     * by simply returning a non-const reference. This is best illustrated with
     * an example.
     *
     * Consider a DataSet class that keeps track of the offset to the voxel of
     * interest, by updating the offset every time the position along any axis
     * is modified (this is actually how most built-in DataSet instances
     * operate). This would require an additional operation to be performed
     * on top of the assignment itself. In other words,
     * \code
     * vox[1] = 12;
     * \endcode
     * cannot be done simply by returning a reference to the corresponding
     * position along axis 1, since the offset would be left unmodified, and
     * would therefore no longer point to the correct voxel. This can be solved
     * by returning a DataSet::Position instead, as illustated below.
     *
     * The following class implements the desired interface, but the offset
     * is not updated when required:
     * \code
     * class MyDataSet
     * {
     *   public:
     *     MyDataSet (ssize_t xdim, ssize_t ydim, ssize_t zdim) :
     *       offset (0)
     *     {
     *       // dataset dimensions:
     *       N[0] = xdim; N[1] = ydim; N[2] = zdim;
     *       // voxel positions along each axis:
     *       X[0] = X[1] = X[2] = 0;
     *       // strides to compute correct offset;
     *       S[0] = 1; S[1] = N[0]; S[2] = N[0]*N[1];
     *       // allocate data:
     *       data = new float [N[0]*N[1]*N[2]);
     *     }
     *     ~MyDataSet () { delete [] data; }
     *
     *     typedef float value_type;
     *
     *     size_t     ndim () const            { return 3; }
     *     ssize_t    dim (size_t axis) const  { return N[axis]; }
     *
     *     // PROBLEM: can't modify the offset when the position is modified!
     *     ssize_t&   operator[] (size_t axis) { return X[axis]; }
     *
     *     float&  value () { return (data[offset]); }
     *
     *   private:
     *     float*  data
     *     ssize_t N[3], X[3], S[3], offset;
     * };
     * \endcode
     * The DataSet::Position class provides a solution to this problem. To use
     * it, the DataSet class must implement the get_pos(), set_pos(), and
     * move_pos() methods. While these can be declared public, it is probably
     * cleaner to make them private or protected, and to declare the
     * DataSet::Position class as a friend.
     * \code
     * class MyDataSet
     * {
     *   public:
     *     MyDataSet (ssize_t xdim, ssize_t ydim, ssize_t zdim) :
     *       offset (0)
     *     {
     *       // dataset dimensions:
     *       N[0] = xdim; N[1] = ydim; N[2] = zdim;
     *       // voxel positions along each axis:
     *       X[0] = X[1] = X[2] = 0;
     *       // strides to compute correct offset;
     *       S[0] = 1; S[1] = N[0]; S[2] = N[0]*N[1];
     *       // allocate data:
     *       data = new float [N[0]*N[1]*N[2]);
     *     }
     *     ~MyDataSet () { delete [] data; }
     *
     *     typedef float value_type;
     *
     *     size_t     ndim () const            { return 3; }
     *     ssize_t    dim (size_t axis) const  { return N[axis]; }
     *
     *     // FIX: return a DataSet::Position<MyDataSet> class:
     *     DataSet::Position<MyDataSet> operator[] (size_t axis) {
     *       return DataSet::Position<MyDataSet> (*this, axis);
     *     }
     *
     *     float&  value () { return data[offset]; }
     *
     *   private:
     *     float*  data
     *     ssize_t N[3], X[3], S[3], offset;
     *
     *     // this function returns the voxel position along the specified axis,
     *     // in a non-modifiable way:
     *     ssize_t get_pos (size_t axis) const { return X[axis]; }
     *
     *     // this function sets the voxel position along the specified axis:
     *     void set_pos (size_t axis, ssize_t pos) const {
     *       offset += S[axis] * (pos - X[axis]);
     *       X[axis] = pos;
     *     }
     *
     *     // this function moves the voxel position by the specified increment
     *     // along the speficied axis:
     *     void move_pos (size_t axis, ssize_t increment) {
     *       offset += S[axis] * increment;
     *       X[axis] += increment;
     *     }
     *
     *     friend class DataSet::Position<MyDataSet>;
     * };
     * \endcode
     * In the example above, a DataSet::Position instance is returned by the
     * operator[]() function, and can be manipulated using standard operators. For
     * instance, the following code fragment is allowed, and will update the
     * offset each time as expected:
     * \code
     * // create an instance of MyDataSet:
     * MyDataSet data (100, 100, 100);
     *
     * // setting the position of the voxel will cause
     * // the set_pos() function to be invoked each time:
     * // ensuring the offset is up to date:
     * data[0] = 10;
     *
     * // setting the position also causes the get_pos() function to
     * // be invoked, so that the new position can be returned.
     * // This allows chaining of the assignments, e.g.:
     * data[1] = data[2] = 20;
     *
     * // incrementing the position will cause the move_pos()
     * // function to be invoked (and also the get_pos() function
     * // so that the new position can be returned):
     * data[0] += 10;
     *
     * // reading the position will cause the get_pos()
     * // function to be invoked:
     * ssize_t xpos = data[0];
     *
     * // this implies that the voxel position can be used
     * // in simple looping constructs:
     * float sum = 0.0
     * for (data[1] = 0; data[1] < data.dim(1); ++data[1])
     *   sum += data.value();
     * \endcode
     *
     * \section performance Performance
     * The use of this class or of the related DataSet::Value class does not
     * impose any measurable performance penalty if the code is
     * compiled for release (i.e. with optimisations turned on and debugging
     * symbols turned off). This is the default setting for the configure
     * script, unless the -debug option is supplied.
     */
    template <class Set> class Position
    {
      public:
        Position (Set& parent, size_t corresponding_axis) : S (parent), axis (corresponding_axis) {
          assert (axis < S.ndim());
        }
        operator ssize_t () const          {
          return S.get_pos (axis);
        }
        ssize_t operator++ ()              {
          S.move_pos (axis,1);
          return S.get_pos (axis);
        }
        ssize_t operator-- ()              {
          S.move_pos (axis,-1);
          return S.get_pos (axis);
        }
        ssize_t operator++ (int)   {
          const ssize_t p = S.get_pos (axis);
          S.move_pos (axis,1);
          return p;
        }
        ssize_t operator-- (int)   {
          const ssize_t p = S.get_pos (axis);
          S.move_pos (axis,-1);
          return p;
        }
        ssize_t operator+= (ssize_t increment) {
          S.move_pos (axis, increment);
          return S.get_pos (axis);
        }
        ssize_t operator-= (ssize_t increment) {
          S.move_pos (axis, -increment);
          return S.get_pos (axis);
        }
        ssize_t operator= (ssize_t position)   {
          S.set_pos (axis, position);
          return position;
        }
        ssize_t operator= (const Position& position) {
          S.set_pos (axis, ssize_t (position));
          return ssize_t (position);
        }
        friend std::ostream& operator<< (std::ostream& stream, const Position& p) {
          stream << ssize_t (p);
          return stream;
        }
      protected:
        Set& S;
        size_t axis;
    };

  }
}

#endif

