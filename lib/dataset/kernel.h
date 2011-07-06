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
            Item (size_t ndim, size_t nslices) : slice (nslices), pos (ndim) { }
            std::vector<RefPtr<T,true> > slice;
            std::vector<ssize_t> pos;
        };


      template <typename T>
        class Allocator
        {
          public:
            Allocator (size_t ndim, size_t nslices) : N (ndim), ns (nslices) { }
            Item<T>* alloc () { Item<T>* item = new Item<T> (N, ns); return (item); }
            void reset (Item<T>* item) { }
            void dealloc (Item<T>* item) { delete item; }
          private:
            size_t N, ns;
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
            typedef Thread::Queue<Item<value_type>,Allocator<value_type> > Queue;

            Loader (
                Queue& queue,
                Input& input,
                const Functor& func,
                const std::vector<size_t>& axes,
                const std::string& progress_message) :
              writer (queue),
              src (input),
              x (axes[0]), y (axes[1]), z(axes[2]),
              nslices (func.extent(z)),
              data (nslices),
              prog_message (progress_message) { }

            void execute ()
            {
              std::vector<size_t> axes (src.ndim()-1);
              axes[0] = y;
              axes[1] = z;
              for (size_t n = 3; n < src.ndim(); ++n)
                axes[n-1] = n;

              std::vector<size_t> slice_axes (2);
              slice_axes[0] = x;
              slice_axes[1] = y;

              ssize_t slice = 0;
              typename Queue::Writer::Item item (writer);
              DataSet::LoopInOrder loop (axes, prog_message);

              const ssize_t slice_offset = (nslices+1)/2;

              for (loop.start (src); loop.ok(); loop.next (src)) {
                // get data:
                while (slice < src[z] + slice_offset)
                  get_slice (slice, slice_axes);

                // set item:
                for (ssize_t i = 0; i < nslices; ++i)
                  item->slice[i] = data[i];

                // set item position:
                for (size_t n = 0; n < src.ndim(); ++n)
                  item->pos[n] = src[n];

                // dispatch:
                if (!item.write()) throw Exception ("error writing to work queue");
              }
            }

          private:
            typename Queue::Writer writer;
            Input&  src;
            const size_t x, y, z;
            const ssize_t nslices;
            std::vector<RefPtr<value_type,true> > data;

            const std::string prog_message;

            void get_slice (ssize_t& slice, const std::vector<size_t>& axes)
            {
              for (ssize_t i = 0; i < nslices-1; ++i)
                data[i] = data[i+1];
              RefPtr<value_type,true> a (slice < src.dim(z) ? new value_type [src.dim(x)*src.dim(y)] : NULL);
              data[nslices-1] = a;
              if (a) {
                const ssize_t pos[] = { src[0], src[1], src[2] };
                DataSet::LoopInOrder loop (axes);
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
            typedef typename Loader<Input,Functor>::Queue Queue;

            Processor (Queue& queue, Output& output, Functor functor, const size_t axes_ordering[3]) :
              reader (queue), dest (output), axes (axes_ordering), func (functor) { }

            void execute ()
            {
              typename Queue::Reader::Item item (reader);
              DataPrivate<value_type> kernel (axes, dest.dim(axes[0]));
              const ssize_t extent[] = {
                (func.extent(axes[0])-1)/2,
                (func.extent(axes[1])-1)/2,
                (func.extent(axes[2])-1)/2
              };
              kernel.offset[2] = extent[2];

              while (item.read()) {

                for (size_t i = 0; i < dest.ndim(); ++i)
                  dest[i] = item->pos[i];

                kernel.offset[1] = item->pos[axes[1]];
                kernel.item = &(*item);

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
              }
            }

          private:
            typename Queue::Reader reader;
            Output dest;
            const size_t* axes;
            Functor func;

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

          typename Loader<Input,Functor>::Queue queue
            ("work queue", 100, Allocator<typename Input::value_type> (input.ndim(), functor.extent (axes[2])));

          Loader<Input,Functor> loader (queue, input, functor, ax, progress_message);
          Processor<Input,Output,Functor> processor (queue, output, functor, axes);

          Thread::Array<Processor<Input,Output,Functor> > processor_list (processor);
          Thread::Exec processor_threads (processor_list, "processor");
          loader.execute();
        }

    }

  }
}


#endif
