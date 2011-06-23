/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 16/08/09.

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

#error - this file is for documentation purposes only!
#error - It should NOT be included in other code files.

namespace MR
{

  /** \defgroup DataSet The DataSet abstract class
   * \brief A collection of template functions to operate on GenericDataSet objects
   *
   * MRtrix defines an abstract GenericDataSet class, describing the interface
   * that a number of MRtrix algorithms expect to operate on. It does not
   * correspond to a real class, and only serves to document the expected
   * behaviour for classes that represent image datasets.
   *
   * Classes that are designed to represent a data set should implement at
   * least a subset of the member functions documented for the GenericDataSet
   * class. Such classes should NOT derive from this class, but rather provide
   * their own implementations. There is also no requirement to reproduce the
   * function definitions exactly, as long as the class can be used with the
   * same syntax in practice. DataSet algorithms are defined using the C++
   * template framework, and hence any function call is interpreted at
   * compile-time (and potentially optimised away), rather than being issued at
   * run-time. This is perhaps better illustrated using the example below.
   *
   * The following example defines a simple class to store a 3D image:
   *
   * \code
   * class Image {
   *   public:
   *     Image (int xdim, int ydim, int zdim) {
   *       N[0] = xdim; N[1] = ydim; N[2] = zdim;
   *       X[0] = X[1] = X[2] = 0;
   *       data = new float [N[0]*N[1]*N[2]);
   *     }
   *     ~Image () { delete [] data; }
   *
   *     typedef float value_type;
   *
   *     int     ndim () const           { return (3); }
   *     int     dim (int axis) const    { return (N[axis]); }
   *     int&    operator[] (int axis)   { return (X[axis]); }
   *     float&  value ()                { return (data[X[0]+N[0]*(X[1]+N[1]*X[2])]); }
   *
   *   private:
   *     float*  data
   *     int     N[3];
   *     int     X[3];
   * };
   * \endcode
   *
   * This class does not implement all the functions listed for the
   * GenericDataSet class, and some of the functions it does implement do not
   * match the equivalent GenericDataSet definitions. However, in practice this
   * class can be used with identical syntax. For example, this template
   * function scales the data by a user-defined factor:
   *
   * \code
   * template <class Set> void scale (Set& data, float factor)
   * {
   *   for (data[2] = 0; data[2] < data.dim(2); ++data[2])
   *     for (data[1] = 0; data[1] < data.dim(1); ++data[1])
   *       for (data[0] = 0; data[0] < data.dim(0); ++data[0])
   *         data.value() *= factor;
   * }
   * \endcode
   *
   * This template function might be used like this:
   *
   * \code
   * Image my_image (128, 128, 32); // create an instance of a 128 x 128 x 32 image
   * ...
   * ... // populate my_image with data
   * ...
   * scale (my_image, 10.0); // scale my_image by a factor of 10
   * \endcode
   *
   * As you can see, the \a %Image class implements all the functionality
   * required for the \a scale() function to compile and run. There is also
   * plenty of scope for the compiler to optimise this particular function,
   * since all member functions of \c my_image are declared inline. Note that
   * this does not mean that this class can be used with all of the other
   * template functions, some of which might rely on some of the other member
   * functions having been defined.
   *
   * \par Why define this abstract class?
   *
   * Different image classes may not be suited to all uses. For example, the
   * Image::Voxel class provides access to the data for an image file, but
   * incurs an overhead for each read/write access. A simpler class such as the
   * \a %Image class above can provide much more efficient access to the data.
   * There will therefore be cases where it might be beneficial to copy the
   * data from an Image::Voxel class into a more efficient data structure.
   * In order to write algorithms that can operate on all of these different
   * classes, MRtrix uses the C++ template framework, leaving it up to the
   * compiler to ensure that the classes defined are compatible with the
   * particular template function they are used with, and that the algorithm
   * implemented in the function is fully optimised for that particular class.
   *
   * \par Why not use an abstract base class and inheritance?
   *
   * Defining an abstract class implies that all functions are declared
   * virtual. This means that every operation on a derived class will incur a
   * function call overhead, which will in many cases have a significant
   * adverse impact on performance. This also restricts the amount of optimisation that the
   * compiler might otherwise be able to perform. Using inheritance would have
   * the benefit of allowing run-time polymorphism (i.e. the same function can
   * be used with any derived class at runtime); however, in practice run-time
   * polymorphism is rarely needed in MRtrix applications. Finally, if such an
   * interface were required, it would be trivial to define such an abstract class and
   * use it with the template functions provided by MRtrix.
   *
   * \sa the DataSet::Position and DataSet::Value template classes are designed
   * to simplify the process of returning a modifiable object for non-trivial
   * DataSet classes.
   */


  //! \addtogroup DataSet
  // @{

  /*! \brief The abstract generic DataSet interface
   *
   * This class is an abstract prototype describing the interface that a
   * number of MRtrix algorithms expect to operate on. For more details, see
   * \ref DataSet.
   *
   * \note The DataSet class itself should \b not be used or included in MRtrix
   * programs. Any attempt at including the relevant header will result in
   * compile-time errors.
   */
  class GenericDataSet
  {
    public:
      const std::string& name () const; //!< a human-readable identifier, useful for error reporting
      size_t  ndim () const; //!< the number of dimensions of the image
      int     dim (size_t axis) const; //!< the number of voxels along the specified dimension

      //! the type of data returned by the value() functions
      /*! DataSets can use a different data type to store the voxel intensities
       * than what is provided by the value() interface. For instance, it is
       * not possible to know at compile-time what type of data may be
       * contained in an input data set supplied on the command-line. The
       * Image::Voxel class (a realisation of a DataSet) provides a uniform
       * interface to the data, by translating between the datatype stored on
       * disc and the datatype requested by the application on the fly at
       * runtime. Most instances of a DataSet will probably use a \c float as
       * their \a value_type, but other types could be used in special
       * circumstances. */
      typedef float value_type;

      //! the size of the voxel along the specified dimension
      /*! The first 3 dimensions are always assumed to correspond to the \e x,
       * \e y & \e z spatial dimensions, for which the voxel size has an
       * unambiguous meaning, and should be specified in units of millimeters.
       * For the higher dimensions, the interpretation of the voxel size is
       * undefined, and may assume different meaning for different
       * applications. It may for example correspond to time in a fMRI series,
       * in which case it should be specified in seconds. Other applications
       * such as %DWI may interpret the fourth dimension as the diffusion
       * volume direction, and leave the voxel size undefined. */
      float   vox (size_t axis) const;

      const Math::Matrix<float>& transform () const; //!< the 4x4 transformation matrix of the image.

      //! return the strides used to navigate the data in memory
      /*! This return the number of voxel values to skip to reach the adjacent voxel
       * along the specified axis. */
      const ssize_t& stride (size_t axis) const;

      void    reset (); //!< reset the current position to zero

      const ssize_t& operator[] (size_t axis) const; //!< return the current position along dimension \a axis
      ssize_t& operator[] (size_t axis); //!< modify the current position along dimension \a axis

      const value_type&   value () const; //!< return the value of the voxel at the current position
      value_type&   value ();         //!< modify the value of the voxel at the current position
  };

  // @}

}

