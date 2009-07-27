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

#error the header file "dataset.h" is for documentation purposes only, and should not be included

namespace MR {

  //! This abstract generic DataSet interface
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
   * practice. Algorithms designed to operate on DataSets are defined using the
   * C++ template framework, and hence any function calls are interpreted at
   * compile-time, rather than being issued at run-time.
   * \note This class and its corresponding header should not be used or 
   * included in MRtrix programs. Inclusion will result in compile-time errors. */
  class DataSet {
    public:
      size_t  ndim () const; //!< the number of dimensions of the image
      int     dim (size_t axis) const; //!< the number of voxels along the specified dimension
      //! the size of the voxel along the specified dimension
      /*! The first 3 dimensions are always assumed to correspond to the \e x,
       * \e y & \e z spatial dimensions, for which the voxel size has an
       * unambiguous meaning, and should be specified in millimeters.
       * For the higher dimensions, the interpretation of the voxel size is
       * undefined, and may assume different meaning for different
       * applications. It may for example correspond to time in a fMRI series,
       * in which case it should be specified in seconds. Other applications
       * such as DWI may interpret the fourth dimension as the diffusion 
       * volume direction, and leave the voxel size undefined. */
      float   vox (size_t axis) const;
      DataType datatype () const; //!< the type of the underlying image data.
      const Math::Matrix<float>& transform () const; //!< the 4x4 transformation matrix of the image.

      const ssize_t operator[] (const size_t axis) const; //!< return the current position along dimension \a axis
      ssize_t&      operator[] (const size_t axis);       //!< manipulate the current position along dimension \a axis

      const float   value () const; //<! return the value of the voxel at the current position
      float&        value ();       //<! manipulate the value of the voxel at the current position

      bool  is_complex () const; //<! return whether the underlying data are complex

      const float   real () const; //<! return the real value of the voxel at the current position (for complex data)
      float&        real ();       //<! manipulate the real value of the voxel at the current position (for complex data)

      const float   imag () const; //<! return the imaginary value of the voxel at the current position (for complex data)
      float&        imag ();       //<! manipulate the imaginary value of the voxel at the current position (for complex data)

      const cfloat  Z () const; //<! return the complex value of the voxel at the current position (for complex data)
      cfloat&       Z ();       //<! manipulate the complex value of the voxel at the current position (for complex data)
  };

}
