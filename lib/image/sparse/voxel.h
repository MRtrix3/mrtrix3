/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 01/08/13.

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

#ifndef __image_sparse_voxel_h__
#define __image_sparse_voxel_h__

#include <typeinfo>

#include "image/buffer.h"
#include "image/buffer_sparse.h"
#include "image/voxel.h"
#include "image/handler/base.h"
#include "image/sparse/value.h"

namespace MR
{
  namespace Image
  {
    namespace Sparse
    {



    template <class SparseDataType>
      class Voxel : public Image::Voxel< Image::Buffer<uint64_t> > {
        public:
          Voxel (BufferSparse<SparseDataType>& array) :
              Image::Voxel< Image::Buffer<uint64_t> > (array),
              handler_ (array.__get_handler ()) { }


          typedef uint64_t value_type;
          typedef SparseDataType sparse_data_type;
          typedef Voxel voxel_type;



          Sparse::Value<Voxel> value() {
            return Sparse::Value<Voxel> (*this);
          }


          friend std::ostream& operator<< (std::ostream& stream, const Voxel& V) {
            stream << "sparse voxel for image \"" << V.name() << "\", position [ ";
            for (size_t n = 0; n < V.ndim(); ++n)
              stream << V[n] << " ";
            stream << "], value = " << V.value();
            return stream;
          }



        protected:
          std::shared_ptr<Handler::Base> handler_;

          Handler::Sparse& get_handler()
          {
            Handler::Base* ptr (handler_.get());
            return *(dynamic_cast<Handler::Sparse*> (ptr));
          }

          value_type value() const { return Image::Voxel< Image::Buffer<uint64_t> >::value(); }

          friend class Sparse::Value<Voxel>;
      };


    }
  }
}

#endif




