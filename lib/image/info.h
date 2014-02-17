#ifndef __image_info_h__
#define __image_info_h__

#include "debug.h"
#include "types.h"
#include "datatype.h"
#include "image/axis.h"
#include "math/matrix.h"

namespace MR
{
  namespace Image
  {

    

    class Info 
    {
      public:

        Info () { }

        //! copy constructor
        template <class InfoType> 
          Info (const InfoType& H) :
            name_ (H.name()),
            datatype_ (H.datatype()),
            transform_ (H.transform()),
            axes_ (H.ndim()) {
              for (size_t n = 0; n < ndim(); ++n) {
                dim(n) = H.dim (n);
                vox(n) = H.vox (n);
                stride(n) = H.stride (n);
              }
            }

        //! assignment operator
        template <class InfoType> 
          Info& operator= (const InfoType& H) {
            name_ = H.name();
            datatype_ = H.datatype();
            transform_ = H.transform();
            set_ndim (H.ndim());
            for (size_t n = 0; n < ndim(); ++n) {
              dim(n) = H.dim (n);
              vox(n) = H.vox (n);
              stride(n) = H.stride (n);
            }
            return *this;
          }

        const Info& info () const {
          return *this;
        }
        Info& info () {
          return *this;
        }

        const std::string& name () const {
          return name_;
        }
        std::string& name () {
          return name_;
        }

        const DataType& datatype () const {
          return datatype_;
        }
        DataType& datatype () {
          return datatype_;
        }

        size_t ndim () const {
          return axes_.size();
        }
        void set_ndim (size_t new_ndim) {
          axes_.resize (new_ndim);
        }

        const int& dim (size_t axis) const {
          return axes_[axis].dim;
        }
        int& dim (size_t axis) {
          return axes_[axis].dim;
        }

        const float& vox (size_t axis) const {
          return axes_[axis].vox;
        }
        float& vox (size_t axis) {
          return axes_[axis].vox;
        }

        const ssize_t& stride (size_t axis) const {
          return axes_[axis].stride;
        }
        ssize_t& stride (size_t axis) {
          return axes_[axis].stride;
        }

        const Math::Matrix<float>& transform () const {
          return transform_;
        }
        Math::Matrix<float>& transform () {
          return transform_;
        }

        void clear () {
          name_.clear();
          axes_.clear();
          datatype_ = DataType();
          transform_.clear();
        }

        void sanitise () {
          DEBUG ("sanitising image information...");
          sanitise_voxel_sizes ();
          sanitise_transform ();
          sanitise_strides ();
        }

        friend std::ostream& operator<< (std::ostream& stream, const Info& A)
        {
          stream << A.name() << ": dim: [ ";
          for (size_t n = 0; n < A.ndim(); ++n) stream << A.dim(n) << " ";
          stream << "], vox: [";
          for (size_t n = 0; n < A.ndim(); ++n) stream << A.vox(n) << " ";
          stream << "], stride: [";
          for (size_t n = 0; n < A.ndim(); ++n) stream << A.stride(n) << " ";
          stream << "], datatype: " << A.datatype().specifier();
          return stream;
        }

      protected:
        std::string          name_;
        DataType             datatype_;
        Math::Matrix<float>  transform_;
        std::vector<Axis>    axes_;

        void sanitise_voxel_sizes ();
        void sanitise_transform ();
        void sanitise_strides ();
    };







    class ConstInfo : public Info
    {
      public:

        ConstInfo () { }

        //! copy constructor
        template <class InfoType> 
          ConstInfo (const InfoType& H) : 
            Info (H) { }

        const Info& info () const {
          return *this;
        }
        const std::string& name () const {
          return name_;
        }
        const DataType& datatype () const {
          return datatype_;
        }
        size_t ndim () const {
          return axes_.size();
        }
        int dim (size_t axis) const {
          return axes_[axis].dim;
        }
        float vox (size_t axis) const {
          return axes_[axis].vox;
        }
        ssize_t stride (size_t axis) const {
          return axes_[axis].stride;
        }
        const Math::Matrix<float>& transform () const {
          return transform_;
        }
      private: 
        template <class InfoType> 
          ConstInfo& operator= (const InfoType& H) { assert (0); return *this; }
        using Info::set_ndim;
        using Info::clear;
        using Info::sanitise;
    };

  }
  //! @}
}

#endif


