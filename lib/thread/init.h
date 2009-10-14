/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 14/10/09.

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

#ifndef __mrtrix_thread_init_h__
#define __mrtrix_thread_init_h__

#include <pthread.h>
#include "exception.h"

/** \defgroup Thread Multi-threading
 * \brief functions to provide support for multi-threading 
 *
 * These functions and class provide a simple interface for multi-threading in
 * MRtrix applications. Most applications will probably find that the
 * Thread::Queue and Thread::Exec classes are sufficient for their needs. 
 *
 * \note The Thread::init() functin must be called \e before using any of the
 * multi-threading functionality described here.
 */

namespace MR {
  namespace Thread {

    /** \addtogroup Thread 
     * @{ */

    bool initialised ();
    //! Initialise the thread system
    /*! This function must be called before using any of the functionality in
     * the multi-threading system. */
    void init ();

    /*! the number of cores to use for multi-threading, as specified in the
     * variable NumberOfThreads in the MRtrix configuration file */
    size_t number ();
    const pthread_attr_t* default_attributes();

    /** @} */
  }
}

#endif



