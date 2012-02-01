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
#include "image/adapter/data.h"

namespace MR
{
  namespace Image
  {


    template <typename T> class DataPreload : public Data<T>
    {
      public:
        DataPreload (const std::string& image_name) :
          Data<value_type> (image_name),
          data_ (NULL) {
            init();
          }

        DataPreload (const std::string& image_name, Header& original_header) :
          Data<value_type> (image_name),
          data_ (NULL) {
            original_header = *this;
            init();
          }

        DataPreload (const std::string& image_name, const Image::Stride::List& desired_strides) :
          Data<value_type> (image_name),
          data_ (NULL) {
            init (desired_strides);
          }

        DataPreload (const std::string& image_name, const Image::Stride::List& desired_strides, Header& original_header) :
          Data<value_type> (image_name),
          data_ (NULL) {
            original_header = *this;
            init (desired_strides);
          }

        ~DataPreload () {
          if (!handler)
            delete [] data_;
        }

        typedef T value_type;
        typedef Image::Voxel<DataPreload> voxel_type;

        value_type get_value (size_t index) const {
          return data_[index];
        }

        void set_value (size_t index, value_type val) const {
          data_[index] = val;
        }

        friend std::ostream& operator<< (std::ostream& stream, const DataPreload& V) {
          stream << "preloaded data for image \"" << V.name() << "\": " + str (Image::voxel_count (V))
            + " voxels in " + V.datatype().specifier() + " format, stored at address " + str ((void*) V.data_);
          return stream;
        }

        using Data<value_type>::name;
        using Data<value_type>::datatype;

      protected:
        value_type* data_;
        using Data<value_type>::handler;

        template <class Set> DataPreload& operator= (const Set& H) { assert (0); return *this; }

        class DataWithStride : public Adapter::Data<DataPreload<value_type> > {
          public:
            DataWithStride (DataPreload<value_type>& data, const Stride::List& new_strides) : 
              Adapter::Data<DataPreload<value_type> > (data),
              stride_ (new_strides) { }
            int stride (size_t axis) const { 
              return stride_[axis];
            }
          private:
            Stride::List stride_;
        };

        void init (const Stride::List& desired_strides) {
          Stride::List new_strides = Image::Stride::get_nearest_match (static_cast<ConstHeader&> (*this), desired_strides);

          if (new_strides == Image::Stride::get (static_cast<ConstHeader&> (*this))) {
            init();
            return;
          }

          DataWithStride new_data (*this, new_strides);
          Voxel<DataWithStride> dest_vox (new_data);

          do_load (dest_vox);
          Stride::set (static_cast<Header&> (*this), new_strides);
        }


        void init () {
          assert (handler);
          assert (handler->nsegments());

          if (handler->nsegments() == 1 && datatype() == DataType::from<value_type>()) {
            info ("data in \"" + name() + "\" already in required format - mapping as-is");
            data_ = reinterpret_cast<value_type*> (handler->segment (0));
            return;
          }

          voxel_type dest (*this);
          do_load (dest);
        }

        template <class VoxType> void do_load (VoxType& destination) {
          info ("data for image \"" + name() + "\" will be loaded into memory");
          data_ = new value_type [Image::voxel_count (*this)];
          Data<value_type>& filedata (*this);
          typename Data<value_type>::voxel_type src (filedata);
          Image::copy_with_progress_message ("loading data for image \"" + name() + "\"...", destination, src);

          Info::datatype_ = DataType::from<value_type>();
          handler = NULL;
        }

    };




  }
}

#endif




