/*
    Copyright 2009 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/07/09.

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

#error - "dataset.h" is for documentation purposes only!
#error - It should NOT be included in other code files.

#ifdef DOXYGEN_SHOULD_SKIP_THIS /* Doxygen should NOT skip this! */

namespace MR {

  /*! \defgroup Image Image access
   * \brief Classes and functions providing access to image data. */

  //! \addtogroup Image 
  // @{
    

  //! The abstract generic DataSet interface
  /*! This class is an abstract prototype describing the interface that a
   * number of MRtrix algorithms expect to operate on. It does not correspond
   * to a real class, and only serves to document the expected behaviour for
   * classes that represent image datasets.
   *
   * Classes that are designed to represent a data set should implement at
   * least a subset of the member functions described here. Such classes should
   * NOT derive from this class, but rather provide their own implementations. 
   * There is also no requirement to reproduce the function definitions
   * exactly, as long as the class can be used with the same syntax in
   * practice. Algorithms designed to operate on a DataSet are defined using the
   * C++ template framework, and hence any function calls are interpreted at
   * compile-time, rather than being issued at run-time. This is perhaps better
   * illustrated using the example below.
   *
   * The following example defines a simple class to store a 3D image:
   * \code
   * class Image {
   *   public:
   *     Image (int xdim, int ydim, int zdim) { 
   *       nvox[0] = xdim; nvox[1] = ydim; nvox[2] = zdim;
   *       pos[0] = pos[1] = pos[2] = 0;
   *       data = new float [nvox[0]*nvox[1]*nvox[2]);
   *     }
   *    ~Image () { delete [] data; }
   *
   *     int     ndim () const         { return (3); }
   *     int     dim (int axis) const  { return (nvox[axis]); }
   *     int&    operator[] (int axis) { return (pos[axis]); }
   *     float&  value()               { return (data[pos[0]+nvox[0]*(pos[1]+nvox[1]*pos[2])]); }
   *
   *   private:
   *     float*   data
   *     int      nvox[3];
   *     int      pos[3];
   * };
   * \endcode
   * This class does not implement all the functions listed for the generic
   * DataSet class, and some of the functions it does implement do not match
   * the DataSet equivalent definitions. However, in practice this
   * class can be used with identical syntax. For example, this template
   * function scales the data by a user-defined factor:
   * \code
   * template <class DataSet> void scale (DataSet& data, float factor)
   * {
   *   for (data[2] = 0; data[2] < data.dim(2); data[2]++)
   *     for (data[1] = 0; data[1] < data.dim(1); data[1]++)
   *       for (data[0] = 0; data[0] < data.dim(0); data[0]++)
   *         data.value() *= factor;
   * }
   * \endcode
   * This template function might be used like this:
   * \code
   * Image my_image (128, 128, 32); // create an instance of a 128 x 128 x 32 image
   * ...
   * ... // populate my_image with data
   * ...
   * scale (my_image, 10.0); // scale my_image by a factor of 10
   * \endcode
   * As you can see, the \a %Image class implements all the functionality
   * required for the \a scale() function to compile and run. This does not
   * mean that this class can be used with any of the other template functions,
   * some of which might rely on some of the other member functions having been
   * defined.
   *
   * \par Why define this abstract class?
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
   * Defining an abstract class implies that all functions are declared
   * virtual. This means that every operation on derived class will incur a
   * function call overhead, which will in many cases have a significant
   * adverse impact on performance. This also restricts the amount of optimisation that the
   * compiler might otherwise be able to perform. Using inheritance would have
   * the benefit of allowing run-time polymorphism (i.e. the same function can
   * be used with any derived class at runtime); however, in practice run-time
   * polymorphism is rarely needed in MRtrix applications. Finally, if such an
   * interface were required, it would be trivial to define such an abstract class and
   * use it with the template functions provided by MRtrix.
   *
   * \note The DataSet class and its corresponding header should \b not be used or 
   * included in MRtrix programs. Inclusion will result in compile-time errors. */
  class DataSet {
    public:
      const std::string& name () const; //!< a human-readable identifier, useful for error reporting
      size_t  ndim () const; //!< the number of dimensions of the image
      int     dim (size_t axis) const; //!< the number of voxels along the specified dimension

      //! the size of the voxel along the specified dimension
      /*! The first 3 dimensions are always assumed to correspond to the \e x,
       * \e y & \e z spatial dimensions, for which the voxel size has an
       * unambiguous meaning, and should be specified in units of millimeters.
       * For the higher dimensions, the interpretation of the voxel size is
       * undefined, and may assume different meaning for different
       * applications. It may for example correspond to time in a fMRI series,
       * in which case it should be specified in seconds. Other applications
       * such as DWI may interpret the fourth dimension as the diffusion 
       * volume direction, and leave the voxel size undefined. */
      float   vox (size_t axis) const;

      //! provides access to the ordering of the data in memory
      /*! This function should return the \a n th axis whose data points are most
       * contiguous in memory. This is helpful to optimise algorithms that
       * operate on image voxels independently, with no dependence on the order
       * of processing, since the algorithm can then perform the processing in
       * the order that makes best use of the memory subsystem's bandwidth.
       *
       * For example, if a 3D image is stored with all anterior-posterior
       * voxels stored contiguously in memory, and all such lines along the
       * inferior-superior axis are stored contiguously, and finally all such
       * slices along the left-right axis are stored contiguously (corresponding
       * to a stack of sagittal slices), then this function should return 1 for
       * \a n = 0, 2 for \a n = 1, and 0 for \a n = 2. The innermost loop of
       * an algorithm can then be made to loop over the anterior-posterior
       * direction, which is optimal in terms of memory bandwidth.
       *
       * An algorithm might make use of this feature in the following way:
       * \code
       * template <class DataSet> void add (DataSet& data, float offset) 
       * {
       *   size_t I = data.contiguous(0);
       *   size_t J = data.contiguous(1);
       *   size_t K = data.contiguous(2);
       *   for (data[K] = 0; data[K] < data.dim(K); data[K]++)
       *     for (data[J] = 0; data[J] < data.dim(J); data[J]++)
       *       for (data[I] = 0; data[I] < data.dim(I); data[I]++)
       *         data.value() += offset;
       * }
       * \endcode
       *
       * \note this is NOT the order as specified in the MRtrix file format,
       * but its exact inverse. */
      size_t  contiguous (size_t n) const;

      DataType datatype () const; //!< the type of the underlying image data.
      const Math::Matrix<float>& transform () const; //!< the 4x4 transformation matrix of the image.

      const ssize_t operator[] (const size_t axis) const; //!< return the current position along dimension \a axis
      ssize_t&      operator[] (const size_t axis);       //!< manipulate the current position along dimension \a axis

      const float   value () const; //!< return the value of the voxel at the current position
      float&        value ();       //!< manipulate the value of the voxel at the current position

      bool  is_complex () const; //!< return whether the underlying data are complex

      const float   real () const; //!< return the real value of the voxel at the current position (for complex data)
      float&        real ();       //!< manipulate the real value of the voxel at the current position (for complex data)

      const float   imag () const; //!< return the imaginary value of the voxel at the current position (for complex data)
      float&        imag ();       //!< manipulate the imaginary value of the voxel at the current position (for complex data)

      const cfloat  Z () const; //!< return the complex value of the voxel at the current position (for complex data)
      cfloat&       Z ();       //!< manipulate the complex value of the voxel at the current position (for complex data)
  };
  
   //! @}
}

#endif /* DOXYGEN_SHOULD_SKIP_THIS */


