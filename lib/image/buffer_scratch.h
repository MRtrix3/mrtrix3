#ifndef __image_buffer_scratch_h__
#define __image_buffer_scratch_h__

#include "debug.h"
#include "image/info.h"
#include "image/stride.h"
#include "image/voxel.h"
#include "image/adapter/info.h"

namespace MR
{
  namespace Image
  {


    template <typename ValueType>
      class BufferScratch : public ConstInfo
    {
      public:
        template <class Template>
          BufferScratch (const Template& info) :
            ConstInfo (info),
            data_ (new value_type [Image::voxel_count (*this)]) {
              Info::datatype_ = DataType::from<value_type>();
              zero();
            }

        template <class Template>
          BufferScratch (const Template& info, const std::string& label) :
            ConstInfo (info),
            data_ (new value_type [Image::voxel_count (*this)]) {
              zero();
              datatype_ = DataType::from<value_type>();
              name_ = label;
            }

        typedef ValueType value_type;
        typedef Image::Voxel<BufferScratch> voxel_type;

        void zero () {
          memset (data_, 0, Image::voxel_count (*this) * sizeof(ValueType));
        }

        value_type get_value (size_t index) const {
          return data_[index];
        }

        void set_value (size_t index, value_type val) {
          data_[index] = val;
        }

        value_type* address (size_t index) const {
          return &data_[index];
        }

        friend std::ostream& operator<< (std::ostream& stream, const BufferScratch& V) {
          stream << "scratch image data \"" << V.name() << "\": " + str (Image::voxel_count (V))
            + " voxels in " + V.datatype().specifier() + " format, stored at address " + str ((void*) V.data_);
          return stream;
        }

      protected:
        Ptr<value_type,true> data_;

        template <class Set>
          BufferScratch& operator= (const Set& H) { assert (0); return *this; }

        BufferScratch (const BufferScratch& that) { assert (0); }
    };


    template <> inline bool BufferScratch<bool>::get_value (size_t index) const {
      return get<bool> (data_, index);
    }

    template <> inline void BufferScratch<bool>::set_value (size_t index, bool val) {
      put<bool> (val, data_, index);
    }



  }
}

#endif




