/*
    Copyright 2010 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 26/09/10.

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

#ifndef __dataset_kernel_h__
#define __dataset_kernel_h__

#include "ptr.h"
#include "progressbar.h"
#include "thread/exec.h"
#include "thread/queue.h"
#include "dataset/misc.h"
#include "dataset/loop.h"


namespace MR {
  namespace DataSet {

    namespace Kernel {

      template <typename T>
        class Item
        {
          public:
            std::vector<RefPtr<T,true> > slice;
            std::vector<ssize_t> pos;
        };



      //! \cond skip
      namespace {
        template <typename T>
          class DataPrivate
          {
            public:
              DataPrivate (const size_t axes_ordering[3], ssize_t j_skip) :
                axes (axes_ordering), jskip (j_skip) { }

              const size_t* axes;
              const ssize_t jskip;
              ssize_t from[3], to[3], offset[3];
              const Item<T>* item;

              T operator() (ssize_t i, ssize_t j, ssize_t k) const
              {
                const T* slice = item->slice[k+offset[2]];
                ssize_t index = jskip * (j+offset[1]) + i+offset[0];
                return (slice[index]);
              }
          };
      }
      //! \endcond


      template <typename T>
        class Data
        {
          public:
            explicit Data (const DataPrivate<T>& roi) : R (roi) { }

            size_t count () const { return ((to(0)-from(0)) * (to(1)-from(1)) * (to(2)-from(2))); }
            ssize_t from (size_t i) const { return (R.from[i]); }
            ssize_t to   (size_t i) const { return (R.to  [i]); }
            T operator() (ssize_t i, ssize_t j, ssize_t k) const { return (R (i,j,k)); }

          private:
            const DataPrivate<T>& R;
        };



      template <class Input,class Functor>
        class Loader
        {
          public:
            typedef typename Input::value_type value_type;

            Loader (
                Input& input,
                const Functor& func,
                const std::vector<size_t>& axes,
                const std::string& progress_message) :
              src (input),
              x (axes[0]), y (axes[1]), z(axes[2]),
              slice (0),
              nslices (func.extent(z)),
              slice_offset ((nslices+1)/2),
              data (nslices),
              loop (get_axes (axes), progress_message),
              slice_axes (2) { 
                slice_axes[0] = x;
                slice_axes[1] = y;
                loop.start (src);
              }

            bool operator() (Item<value_type>& item) {
              if (loop.ok()) {
                // get data:
                while (slice < src[z] + slice_offset)
                  get_slice (slice);

                // set item:
                item.slice.resize (nslices);
                for (ssize_t i = 0; i < nslices; ++i)
                  item.slice[i] = data[i];

                // set item position:
                item.pos.resize (src.ndim());
                for (size_t n = 0; n < src.ndim(); ++n)
                  item.pos[n] = src[n];

                loop.next (src);
                return true;
              }
              return false;
            }

          private:
            Input&  src;
            const size_t x, y, z;
            ssize_t slice;
            const ssize_t nslices;
            const ssize_t slice_offset;
            std::vector<RefPtr<value_type,true> > data;
            DataSet::LoopInOrder loop;
            std::vector<size_t> slice_axes;

            std::vector<size_t> get_axes (const std::vector<size_t>& axes) const {
              std::vector<size_t> a (src.ndim()-1);
              a[0] = y;
              a[1] = z;
              for (size_t n = 3; n < src.ndim(); ++n)
                a[n-1] = n;
              return a;
            }

            void get_slice (ssize_t& slice)
            {
              for (ssize_t i = 0; i < nslices-1; ++i)
                data[i] = data[i+1];
              RefPtr<value_type,true> a (slice < src.dim(z) ? new value_type [src.dim(x)*src.dim(y)] : NULL);
              data[nslices-1] = a;
              if (a) {
                const ssize_t pos[] = { src[0], src[1], src[2] };
                DataSet::LoopInOrder loop (slice_axes);
                src[z] = slice;
                value_type* p = a;
                for (loop.start (src); loop.ok(); loop.next (src)) {
                  *p = src.value();
                  ++p;
                }
                src[0] = pos[0];
                src[1] = pos[1];
                src[2] = pos[2];
              }
              ++slice;
            }
        };





      template <class Input, class Output, class Functor>
        class Processor
        {
          public:
            typedef typename Input::value_type value_type;

            Processor (Output& output, Functor functor, const size_t axes_ordering[3]) :
              dest (output),
              axes (axes_ordering), 
              func (functor),
              kernel (axes, dest.dim(axes[0])) { 
                extent[0] = (func.extent(axes[0])-1)/2;
                extent[1] = (func.extent(axes[1])-1)/2;
                extent[2] = (func.extent(axes[2])-1)/2;
                kernel.offset[2] = extent[2];
              }

            bool operator() (const Item<value_type>& item)
            {
              for (size_t i = 0; i < dest.ndim(); ++i)
                dest[i] = item.pos[i];

              kernel.offset[1] = item.pos[axes[1]];
              kernel.item = &item;

              kernel.from[1] = get_from (dest[axes[1]], extent[1]);
              kernel.to[1] = get_to (dest[axes[1]], extent[1], dest.dim(axes[1]));
              kernel.from[2] = get_from (dest[axes[2]], extent[2]);
              kernel.to[2] = get_to (dest[axes[2]], extent[2], dest.dim(axes[2]));

              for (dest[axes[0]] = 0; dest[axes[0]] < dest.dim(axes[0]); ++dest[axes[0]]) {
                kernel.offset[0] = dest[axes[0]];
                kernel.from[0] = get_from (dest[axes[0]], extent[0]);
                kernel.to[0] = get_to (dest[axes[0]], extent[0], dest.dim(axes[0]));

                dest.value() = func (Data<value_type> (kernel));
              }

              return true;
            }

          private:
            Output dest;
            const size_t* axes;
            Functor func;
            DataPrivate<value_type> kernel;
            ssize_t extent[3];

            ssize_t get_from (ssize_t pos, ssize_t offset) const
            {
              return (std::max (offset-pos, ssize_t(0))-offset);
            }

            ssize_t get_to (ssize_t pos, ssize_t offset, ssize_t max) const
            {
              return (offset+1 - std::max (offset - max+1 + pos, ssize_t(0)));
            }
        };






      template <class Input, class Output, class Functor>
        inline void run (Output& output, Input& input, Functor functor, const std::string& progress_message)
        {
          std::vector<size_t> ax = DataSet::Stride::order (input, 0, 3);
          const size_t axes[] = { ax[0], ax[1], ax[2] };
          functor.prepare (input, axes[0], axes[1], axes[2]);

          Loader<Input,Functor> loader (input, functor, ax, progress_message);
          Processor<Input,Output,Functor> processor (output, functor, axes);

          Thread::run_queue (loader, 1, Item<typename Input::value_type>(), processor, 0);
        }

    }

  }
}


#endif
