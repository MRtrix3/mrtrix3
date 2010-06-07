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

#ifndef __dataset_value_h__
#define __dataset_value_h__

namespace MR {
  namespace DataSet {

    //! \addtogroup DataSet
    // @{

    /*! \brief a class to simplify the implementation of DataSet classes with
     * non-trivial access to their data.
     *
     * This class provides a means of returning a modifiable voxel value from a
     * class, when the process of modifying a value cannot be done by simply
     * returning a non-const reference. This is best illustrated with an
     * example. 
     *
     * Consider a DataSet class that perform a validity check on each voxel
     * value as it is stored. For example, it might enforce a policy that
     * values should be clamped to a particular range. Such a class might look
     * like the following:
     * \code
     * class MyDataSet
     * {
     *   public:
     *     MyDataSet (int xdim, int ydim, int zdim)
     *     {
     *       N[0] = xdim; N[1] = ydim; N[2] = zdim;
     *       X[0] = X[1] = X[2] = 0;
     *       data = new float [N[0]*N[1]*N[2]);
     *     }
     *     ~MyDataSet () { delete [] data; }
     * 
     *     typedef float value_type;
     * 
     *     int     ndim () const           { return (3); }
     *     int     dim (int axis) const    { return (N[axis]); }
     *     int&    operator[] (int axis)   { return (X[axis]); }
     *
     *     // PROBLEM: can't check that the value is valid when it is modified!
     *     float&  value () { return (data[X[0]+N[0]*(X[1]+N[1]*X[2])]); }
     * 
     *   private:
     *     float*  data
     *     int     N[3];
     *     int     X[3];
     * };
     * \endcode
     * While it is possible in the above example to modify the voxel values,
     * since they are returned by reference, it is also impossible to implement
     * clamping of the voxel values as they are modified. The DataSet::Value
     * class provides a solution to this problem. 
     * 
     * To use the DataSet::Value interface, the DataSet class must implement a
     * get_value() and a set_value() method. While these can be declared
     * public, it is probably cleaner to make them private or protected, and to
     * declare the DataSet::Value class as a friend. 
     * \code
     * class MyDataSet
     * {
     *   public:
     *     MyDataSet (int xdim, int ydim, int zdim)
     *     {
     *       N[0] = xdim; N[1] = ydim; N[2] = zdim;
     *       X[0] = X[1] = X[2] = 0;
     *       data = new float [N[0]*N[1]*N[2]);
     *     }
     *     ~MyDataSet () { delete [] data; }
     * 
     *     typedef float value_type;
     * 
     *     int     ndim () const           { return (3); }
     *     int     dim (int axis) const    { return (N[axis]); }
     *     int&    operator[] (int axis)   { return (X[axis]); }
     *
     *     // FIX: return a DataSet::Value<MyDataSet> class:
     *     DataSet::Value<MyDataSet>  value () { return (DataSet::Value<MyDataSet> (*this); }
     * 
     *   private:
     *     float*  data
     *     int     N[3];
     *     int     X[3];
     *
     *     // this function returns the voxel value, in a non-modifiable way:
     *     value_type get_value () const { return (data[X[0]+N[0]*(X[1]+N[1]*X[2])]); }
     *
     *     // this function ensures that the value supplied is clamped
     *     // between 0 and 1 before updating the voxel value:
     *     void set_value (value_type val) {
     *       if (val < 0.0) val = 0.0;
     *       if (val > 1.0) val = 1.0;
     *       data[X[0]+N[0]*(X[1]+N[1]*X[2])] = val;
     *     }
     *
     *     friend class DataSet::Value<MyDataSet>;
     * };
     * \endcode
     * In the example above, a DataSet::Value instance is returned by the
     * value() function, and can be manipulated using standard operators. For
     * instance, the following code fragment is allowed, and will function as
     * expected with clamping of the voxel values when any voxel value is
     * modified:
     * \code
     * // create an instance of MyDataSet:
     * MyDataSet data (100, 100, 100);
     *
     * // set the position of the voxel:
     * data[0] = X;
     * data[1] = Y;
     * data[2] = Z;
     * 
     * // set the voxel value, clamped to the range [0.0 1.0]:
     * data.value() = 2.3;
     * 
     * // other operators can also be used, for example:
     * data.value() += 10.0;
     * data.value() /= 5.0;
     * \endcode
     *
     * \section performance Performance
     * The use of this class or of the related DataSet::Position class does not
     * impose any measurable performance penalty if the code is
     * compiled for release (i.e. with optimisations turned on and debugging
     * symbols turned off). This is the default setting for the configure
     * script, unless the -debug option is supplied.
     */
    template <class Set> class Value {
      public:
        typedef typename Set::value_type value_type;

        Value (Set& parent) : S (parent) { }
        operator value_type () const { return (S.get_value()); }
        value_type operator= (value_type value) { S.set_value (value); return (value); }
        value_type operator= (const Value& V) { value_type value = V.S.get_value(); S.set_value (value); return (value); }
        value_type operator+= (value_type value) { value += S.get_value(); S.set_value (value); return (value); }
        value_type operator-= (value_type value) { value = S.get_value() - value; S.set_value (value); return (value); }
        value_type operator*= (value_type value) { value *= S.get_value(); S.set_value (value); return (value); }
        value_type operator/= (value_type value) { value = S.get_value() / value; S.set_value (value); return (value); }
      private:
        Set& S;
    };

    //! @}
  }
}

#endif

