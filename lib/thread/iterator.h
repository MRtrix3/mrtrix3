/*
    Copyright 2009 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 16/10/09.

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

#ifndef __thread_iterator_h__
#define __thread_iterator_h__

#include "thread/mutex.h"
#include "dataset/loop.h"
#include "dataset/iterator.h"

namespace MR
{
  namespace Thread
  {

    /** \addtogroup Thread
      @{ */

    /** \defgroup threadloop Multi-threaded iterator class
      @{ */

    //! a thread-safe class to iterate over voxels in a mask
    /*! Here is an example:
    * \code
    * class Processor {
    *   public:
    *     Processor (Thread::MaskIterator<>& iterator, Image::Header& data) :
    *       iterate (iterator),
    *       voxel (data) { }
    *
    *     // multi-threaded processing takes place here:
    *     void execute () {
    *       while (iterate (voxel)) {
    *         // perform processing here, e.g.:
    *         float val = voxel.value();
    *         ...
    *       }
    *     }
    *
    *   private:
    *     Thread::Iterator<>& iterate;
    *     Image::Voxel<float> voxel;
    * };
    *
    *
    * // in invoking function:
    *
    * // the DataSet to be processed:
    * Image::Header header ("datafile.mif");
    *
    * // the binary mask within which processing is to happen:
    * Image::Header mask_header ("mask.mif");
    * Image::Voxel<bool> mask (mask_header);
    *
    * // create a loop to iterate over the dataset:
    * DataSet::Loop loop ("processing message...");
    *
    * // create a Thread::MaskIterator object to loop in a thread-safe manner:
    * Thread::MaskIterator<DataSet::Loop> iterate (loop, mask);
    *
    * // create an instance of the processor,
    * // passing the Thread::Iterator object by reference:
    * Processor processor (iterate, header);
    *
    * // launch threads:
    * Thread::Array<Processor> processor_array (processor);
    * Thread::Exec threads (processor_array);
    * \endcode
    *
    * \note the Thread::MaskIterator object must be \em shared amongst the
    * threads. In other words, ensure your functor objects keep a \em reference
    * to the Thread::MaskIterator object, not a copy of it.
    */
    template <class Set, class Loop = DataSet::Loop> class MaskIterator
    {
      public:
        //! Constructor
        /*! Construct an object to fetch the next coordinates at which the mask
         * is true, using the Loop supplied, in a thread-safe manner. */
        MaskIterator (Loop& loop, const Set& mask) :
          loop_ (loop), mask_ (mask) {
          reset();
        }

        //! reset the loop to the first voxel
        void reset () {
          loop_.start (mask_);
        }

        //! fetch the next coordinates to process
        /*! return false if there are no more voxels to process */
        template <class Container>
        bool operator () (Container& pos) {
          Mutex::Lock lock (mutex_);
          while (loop_.ok()) {
            loop_.next (mask_);
            if (mask_.value()) {
              loop_.set_position (mask_, pos);
              return true;
            }
          }
          return false;
        }

        //! fetch the next coordinates to process
        /*! return false if there are no more voxels to process */
        template <class Container1, class Container2>
        bool operator () (Container1& pos1, Container2& pos2) {
          Mutex::Lock lock (mutex_);
          while (loop_.ok()) {
            loop_.next (mask_);
            if (mask_.value()) {
              loop_.set_position (mask_, pos1, pos2);
              return true;
            }
          }
          return false;
        }

        //! fetch the next coordinates to process
        /*! return false if there are no more voxels to process */
        template <class Container1, class Container2, class Container3>
        bool operator () (Container1& pos1, Container2& pos2, Container3& pos3) {
          Mutex::Lock lock (mutex_);
          while (loop_.ok()) {
            loop_.next (mask_);
            if (mask_.value()) {
              loop_.set_position (mask_, pos1, pos2, pos3);
              return true;
            }
          }
          return false;
        }

      private:
        Loop& loop_;
        Set& mask_;
        Mutex mutex_;

        MaskIterator (const MaskIterator& iterate) { assert (0); }
        MaskIterator& operator= (const MaskIterator& iterate) { assert (0); return *this; }
    };






    //! a thread-safe class to iterate over voxels
    /*! Here is an example:
    * \code
    * class Processor {
    *   public:
    *     Processor (Thread::Iterator<>& iterator, Image::Header& data) :
    *       iterate (iterator),
    *       voxel (data) { }
    *
    *     // multi-threaded processing takes place here:
    *     void execute () {
    *       while (iterate (voxel)) {
    *         // perform processing here, e.g.:
    *         float val = voxel.value();
    *         ...
    *       }
    *     }
    *
    *   private:
    *     Thread::Iterator<>& iterate;
    *     Image::Voxel<float> voxel;
    * };
    *
    *
    * // in invoking function:
    *
    * // the DataSet to be processed:
    * Image::Header header ("datafile.mif");

    * // create a loop to iterate over the dataset:
    * DataSet::Loop loop ("processing message...");
    *
    * // create a Thread::Iterator object to loop in a thread-safe manner:
    * Thread::Iterator<DataSet::Loop> iterate (loop, header);
    *
    * // create an instance of the processor,
    * // passing the Thread::Iterator object by reference:
    * Processor processor (iterate, header);
    *
    * // launch threads:
    * Thread::Array<Processor> processor_array (processor);
    * Thread::Exec threads (processor_array);
    * \endcode
    *
    * \note the Thread::Iterator object must be \em shared amongst the threads. In
    * other words, ensure your functor objects keep a \em reference to the
    * Thread::Iterator object, not a copy of it.
    */
    template <class Loop = DataSet::Loop> class Iterator
    {
      public:
        //! Constructor
        /*! Construct an object to fetch the next coordinates to process, using
         * the Loop supplied, in a thread-safe manner. The \a set supplied is
         * used purely to provide the dimensions of the data - it will not be
         * accessed in any way outside of the constructor. */
        template <class Set>
        Iterator (Loop& loop, const Set& set) :
          loop_ (loop), counter_ (set) {
          loop_.start (counter_);
        }

        //! fetch the next coordinates to process
        /*! return false if there are no more voxels to process */
        template <class Container>
        bool operator () (Container& pos) {
          Mutex::Lock lock (mutex_);
          if (loop_.ok()) {
            loop_.next (counter_);
            loop_.set_position (counter_, pos);
            return true;
          }
          return false;
        }

        //! fetch the next coordinates to process
        /*! return false if there are no more voxels to process */
        template <class Container1, class Container2>
        bool operator () (Container1& pos1, Container2& pos2) {
          Mutex::Lock lock (mutex_);
          if (loop_.ok()) {
            loop_.next (counter_);
            loop_.set_position (counter_, pos1, pos2);
            return true;
          }
          return false;
        }

        //! fetch the next coordinates to process
        /*! return false if there are no more voxels to process */
        template <class Container1, class Container2, class Container3>
        bool operator () (Container1& pos1, Container2& pos2, Container3& pos3) {
          Mutex::Lock lock (mutex_);
          if (loop_.ok()) {
            loop_.next (counter_);
            loop_.set_position (counter_, pos1, pos2, pos3);
            return true;
          }
          return false;
        }

      private:
        Loop& loop_;
        DataSet::Iterator counter_;
        Mutex mutex_;

        Iterator (const Iterator& iterate) { assert (0); }
        Iterator& operator= (const Iterator& iterate) { assert (0); return *this; }
    };



    //! @}
    //! @}
  }
}

#endif




