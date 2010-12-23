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

#ifndef __thread_next_h__
#define __thread_next_h__

#include "thread/mutex.h"
#include "dataset/loop.h"

namespace MR
{
  namespace Thread
  {

    /** \addtogroup Thread
      @{ */

    /** \defgroup threadloop Multi-threaded looping helper class
      @{ */

    template <class Set, class Loop = DataSet::Loop> class NextInMask
    {
      public:
        //! Constructor
        /*! Construct an object to fetch the next coordinates at which the mask
         * is greater than 0.5, using the Loop supplied, in a thread-safe
         * manner. */
        NextInMask (Loop& loop, const Set& mask) :
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
            if (mask_.value () > 0.5) {
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
            if (mask_.value () > 0.5) {
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
            if (mask_.value () > 0.5) {
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

        NextInMask (const NextInMask& next) { assert (0); }
        NextInMask& operator= (const NextInMask& next) { assert (0); return *this; }
    };






    //! a thread-safe class to fetch the next voxel from a DataSet
    /*! Here is an example:
    * \code
    * class Processor {
    *   public:
    *     Processor (Thread::Next<>& nextvoxel, Image::Header& data) :
    *       next (nextvoxel),
    *       voxel (data) { }
    *
    *     // multi-threaded processing takes place here:
    *     void execute () {
    *       while (next (voxel)) {
    *         // perform processing here, e.g.:
    *         float val = voxel.value();
    *         ...
    *       }
    *     }
    *
    *   private:
    *     Thread::Next<>& next;
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
    * // create a Thread::Next object to loop in a thread-safe manner:
    * Thread::Next<DataSet::Loop> next (loop, header);
    *
    * // create an instance of the processor,
    * // passing the Thread::Next object by reference:
    * Processor processor (next, header);
    *
    * // launch threads:
    * Thread::Array<Processor> processor_array (processor);
    * Thread::Exec threads (processor_array);
    * \endcode
    */
    template <class Loop = DataSet::Loop> class Next
    {
      public:
        //! Constructor
        /*! Construct an object to fetch the next coordinates to process, using
         * the Loop supplied, in a thread-safe manner. The \a set supplied is
         * used purely to provide the dimensions of the data - it will not be
         * accessed in any way outside of the constructor. */
        template <class Set>
        Next (Loop& loop, const Set& set) :
          loop_ (loop), dim_ (set.ndim()), pos_ (set.ndim()) {
          for (size_t i = 0; i < dim_.size(); ++i)
            dim_[i] = set.dim (i);
          loop_.start (*this);
        }

        //! fetch the next coordinates to process
        /*! return false if there are no more voxels to process */
        template <class Container>
        bool operator () (Container& pos) {
          Mutex::Lock lock (mutex_);
          if (loop_.ok()) {
            loop_.next (*this);
            loop_.set_position (*this, pos);
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
            loop_.next (*this);
            loop_.set_position (*this, pos1, pos2);
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
            loop_.next (*this);
            loop_.set_position (*this, pos1, pos2, pos3);
            return true;
          }
          return false;
        }


        //! \cond skip
        size_t ndim () const {
          return dim_.size();
        }
        ssize_t dim (size_t axis) const {
          return dim_[axis];
        }

        ssize_t& operator[] (size_t axis) {
          return pos_[axis];
        }
        const ssize_t& operator[] (size_t axis) const {
          return pos_[axis];
        }
        //! \endcond

      private:
        Loop& loop_;
        std::vector<ssize_t> dim_, pos_;
        Mutex mutex_;

        Next (const Next& next) { assert (0); }
        Next& operator= (const Next& next) { assert (0); return *this; }
    };



    //! @}
    //! @}
  }
}

#endif




