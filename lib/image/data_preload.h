/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 23/05/09.

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

#ifndef __image_data_preload_h__
#define __image_data_preload_h__

#include "debug.h"
#include "image/data.h"
#include "image/copy.h"

namespace MR
{
  namespace Image
  {


    //! This class provides access to the voxel intensities of an image.
    /*! This class keeps a reference to an existing Image::Header, and provides
     * access to the corresponding image intensities.
     * \todo Provide specialisations of get/set methods to handle conversions
     * between floating-point and integer types */
    template <typename T> class DataPreload : public DataCommon<T>
    {
      using DataCommon<T>::H;

      public:
        //! construct a DataPreload object to access the data in the Image::Header \p parent
        DataPreload (const Header& parent) :
          DataCommon<T> (parent),
          data (NULL),
          handler (H.get_handler()),
          stride_ (Image::Stride::get (H)) {
            init (handler->nsegments() == 1 &&
              H.datatype() == DataType::from<value_type>());
          }

        //! construct a DataPreload object to access the data in the Image::Header \p parent
        /*! the resulting instance is guaranteed to have the strides specified.
         * Any zero strides will be ignored. */
        DataPreload (const Header& parent, const Image::Stride::List& desired_strides) :
          DataCommon<T> (parent),
          data (NULL),
          handler (H.get_handler()),
          stride_ (Image::Stride::get_nearest_match (H, desired_strides)) {

            init (stride_ == Image::Stride::get (H) &&
                handler->nsegments() == 1 &&
              H.datatype() == DataType::from<value_type>());
          }

        ~DataPreload () {
          if (!handler)
            delete [] data;
        }

        typedef T value_type;
        typedef Image::Voxel<DataPreload> voxel_type;

        DataType datatype () const {
          return DataType::from<value_type>();
        }

        ssize_t stride (size_t axis) const {
          return stride_ [axis];
        }

        value_type get (size_t index) const {
          return data[index];
        }

        void set (size_t index, value_type val) const {
          data[index] = val;
        }

        friend std::ostream& operator<< (std::ostream& stream, const DataPreload& V) {
          stream << "preloaded data for image \"" << V.name() << "\": " + str (Image::voxel_count (V))
            + " voxels in " + V.datatype().specifier() + " format, stored at address " + str (size_t (&(*V.data)));
          return stream;
        }

      private:
        value_type* data;
        Handler::Base* handler;
        const Image::Stride::List stride_;

        void init (bool do_load) {
          assert (handler);
          handler->open();

          if (do_load) {
            info ("data in \"" + H.name() + "\" already in required format - mapping as-is");
            data = reinterpret_cast<value_type*> (handler->segment (0));
            return;
          }

          handler = NULL;
          data = new T [Image::voxel_count(H)];
          info ("data for image \"" + H.name() + "\" will be loaded into memory");
          Data<T> filedata (H);
          typename Data<T>::voxel_type src (filedata);
          voxel_type dest (*this);
          Image::copy_with_progress_message ("loading data for image \"" + H.name() + "\"...", dest, src);
        }

    };




  }
}

#endif




