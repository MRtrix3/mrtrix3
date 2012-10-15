/*
    Copyright 2009 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 20/10/09.

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

#ifndef __image_foreach_h__
#define __image_foreach_h__

#include "debug.h"
#include "image/loop.h"
#include "image/iterator.h"
#include "thread/mutex.h"
#include "thread/exec.h"

namespace MR
{
  namespace Image
  {

    //! \cond skip
    namespace {

      template <class Functor, class VoxelType1>
        class __Foreach_Functor1 {
          public:
            __Foreach_Functor1 (const int noutputs, Functor& functor, VoxelType1& vox1) :
              noutputs (noutputs), func (functor), vox1 (vox1) { } 
            void operator () (const Iterator& pos) {
              voxel_assign (vox1, pos);
              typename VoxelType1::value_type val1 = vox1.value();
              func (val1);
              if (noutputs) 
                vox1.value() = val1;
            }
          protected:
            const int noutputs;
            Functor func;
            VoxelType1 vox1;
        };


      template <class Functor, class VoxelType1, class VoxelType2>
        class __Foreach_Functor2 {
          public:
            __Foreach_Functor2 (const int noutputs, Functor& functor, VoxelType1& vox1, VoxelType2& vox2) :
              noutputs (noutputs), func (functor), vox1 (vox1), vox2 (vox2) { } 
            void operator () (const Iterator& pos) {
              voxel_assign (vox1, pos);
              voxel_assign (vox2, pos);
              typename VoxelType1::value_type val1 = vox1.value();
              typename VoxelType2::value_type val2 = vox2.value();
              func (val1, val2);
              if (noutputs) {
                vox1.value() = val1;
                if (noutputs > 1)
                  vox2.value() = val2;
              }
            }
          protected:
            const int noutputs;
            Functor func;
            VoxelType1 vox1;
            VoxelType2 vox2;
        };


      template <class Functor, class VoxelType1, class VoxelType2, class VoxelType3>
        class __Foreach_Functor3 {
          public:
            __Foreach_Functor3 (const int noutputs, Functor& functor, VoxelType1& vox1, VoxelType2& vox2, VoxelType3& vox3) :
              noutputs (noutputs), func (functor), vox1 (vox1), vox2 (vox2), vox3 (vox3) { } 
            void operator () (const Iterator& pos) {
              voxel_assign (vox1, pos);
              voxel_assign (vox2, pos);
              voxel_assign (vox3, pos);
              typename VoxelType1::value_type val1 = vox1.value();
              typename VoxelType2::value_type val2 = vox2.value();
              typename VoxelType3::value_type val3 = vox3.value();
              func (val1, val2, val3);
              if (noutputs) {
                vox1.value() = val1;
                if (noutputs > 1) {
                  vox2.value() = val2;
                  if (noutputs > 2)
                    vox3.value() = val3;
                }
              }
            }
          protected:
            const int noutputs;
            Functor func;
            VoxelType1 vox1;
            VoxelType2 vox2;
            VoxelType3 vox3;
        };

    }
    //! \endcond

    /*! \addtogroup loop 
     * \{ */

    /** \defgroup foreach Multi-threading per-voxel operations
      * \brief a framework to perform multi-threading operations on a per-voxel basis
      * 
      * A set of functions to allow easy coding of multi-threaded per-voxel
      * operations by passing a simple function / functor that operates on
      * per-voxel values, which avoids the need to code a full Functor kernel
      * for use with the Image::ThreadedLoop infrastructure.
      *
      * The Image::foreach() functions require a function or class as an
      * argument that defines the operation to be performed. When passing a
      * class, the operator() method will be invoked. The number of arguments
      * of this function should match the number of VoxelType objects supplied
      * to Image::foreach(). Any number of the arguments to this function can
      * be specified as outputs; in this case, the outputs should appear first
      * in the list of arguments, and be passed by reference.
      *
      * The Image::foreach() functions themselves can then be used, as follows:
      * - \a loop: an Image::ThreadedLoop instance, already set up as desired.
      * This may be useful when the operation is to be applied only over a
      * sub-volume, or by traversing the data in a specific order.
      * - \a progress_message: a string to be displayed in the progressbar
      * - \a noutputs: the number of arguments to be used as outputs of the
      * function; the values as modified by the function or functor will be
      * written to the corresponding VoxelType objects for each voxel.
      * - \a functor: the function or functor defining the operation itself.
      * - \a VoxelTypeN: the VoxelType objects whose voxel values are to be
      * used as inputs/outputs; their number and order should match that of
      * the function or functor.
      *
      * For example, the following performs an in-place add of \a vox1 and \a vox2,
      * storing the results in \a vox1:
      * \code
      * void myadd (float& in_out, float in2) { in_out += in2; }
      *
      * ...
      *
      * Image::foreach ("adding...", 1, myadd, vox1, vox2);
      * \endcode
      * 
      * This performs the same addition of \a vox1 and \a vox2, this time
      * storing the results in \a vox_out, with no progress display: 
      * \code
      * void myadd (float& out, float in1, float in2) { out = in1 + in2; }
      *
      * ...
      *
      * Image::foreach (1, myadd, vox_out, vox1, vox2);
      * \endcode 
      * 
      * This computes the root-mean-square of \a vox, with no explicit
      * per-voxel output:
      * \code 
      * class RMS {
      *   public:
      *     // we pass a reference to the same double to all threads. 
      *     // Each thread accumulates its own sum-of-squares, and 
      *     // updates the overal sum-of-squares in the destructor, which is
      *     // guaranteed to be invoked after all threads have re-joined.
      *     RMS (double& grand_SoS) : SoS (0.0), grand_SoS (grand_SoS) { }
      *     ~RMS () { grand_SoS += SoS; }
      *
      *     // accumulate the thread-local sum_of_squares:
      *     void operator() (float in) { SoS += Math::pow2 (in); } 
      *
      *   protected:
      *     double SoS;
      *     double& grand_SoS;
      * };
      * 
      * ...
      *
      * double SoS = 0.0;
      * Image::foreach ("computing RMS of \"" + vox.name() + "\"...", 0, * RMS(SoS), vox);
      * double rms = Math::sqrt (SoS / Image::voxel_count (vox));
      * \endcode
      *
      * This takes the third image volume of VoxelType \a vox, and replaces
      * each voxel value with its own log:
      * \code
      * void mylog (float& val) { val = Math::log (val); }
      *
      * ...
      *
      * // set vox position to the third volume:
      * vox[3] = 2; 
      *
      * // Create an Image::ThreadedLoop to iterate over the first 3 axes only:
      * Image::ThreadedLoop loop ("computing log of third volume...", vox, 1, 0, 3);
      * Image::foreach (loop, 1, mylog, vox);
      * \endcode
      * */
     //! \{ 


    template <class Functor, class VoxelType1> 
      inline void foreach (Image::ThreadedLoop& loop, int noutputs, Functor functor, VoxelType1 vox1)
      {
        loop.run (__Foreach_Functor1<Functor, VoxelType1> (noutputs, functor, vox1), "foreach thread");
      }

    template <class Functor, class VoxelType1, class VoxelType2> 
      inline void foreach (Image::ThreadedLoop& loop, int noutputs, Functor functor, VoxelType1 vox1, VoxelType2& vox2)
      {
        loop.run (__Foreach_Functor2<Functor, VoxelType1, VoxelType2> (noutputs, functor, vox1, vox2));
      }

    template <class Functor, class VoxelType1, class VoxelType2, class VoxelType3> 
      inline void foreach (Image::ThreadedLoop& loop, int noutputs, Functor functor, VoxelType1 vox1, VoxelType2& vox2, VoxelType3& vox3)
      {
        loop.run (__Foreach_Functor2<Functor, VoxelType1, VoxelType2> (noutputs, functor, vox1, vox2, vox3));
      }




    template <class Functor, class VoxelType1> 
      inline void foreach (int noutputs, Functor functor, VoxelType1 vox1)
      {
        Image::ThreadedLoop (vox1).run (__Foreach_Functor1<Functor, VoxelType1> (noutputs, functor, vox1), "foreach thread");
      }

    template <class Functor, class VoxelType1, class VoxelType2> 
      inline void foreach (int noutputs, Functor functor, VoxelType1 vox1, VoxelType2& vox2)
      {
        Image::ThreadedLoop (vox1).run (__Foreach_Functor2<Functor, VoxelType1, VoxelType2> (noutputs, functor, vox1, vox2), "foreach thread");
      }

    template <class Functor, class VoxelType1, class VoxelType2, class VoxelType3> 
      inline void foreach (int noutputs, Functor functor, VoxelType1 vox1, VoxelType2& vox2, VoxelType3& vox3)
      {
        Image::ThreadedLoop (vox1).run (__Foreach_Functor3<Functor, VoxelType1, VoxelType2, VoxelType3> (noutputs, functor, vox1, vox2, vox3), "foreach thread");
      }




    template <class Functor, class VoxelType1> 
      inline void foreach (const std::string& progress_message, int noutputs, Functor functor, VoxelType1 vox1)
      {
        Image::ThreadedLoop (progress_message, vox1).run (__Foreach_Functor1<Functor, VoxelType1> (noutputs, functor, vox1), "foreach thread");
      }

    template <class Functor, class VoxelType1, class VoxelType2> 
      inline void foreach (const std::string& progress_message, int noutputs, Functor functor, VoxelType1 vox1, VoxelType2& vox2)
      {
        Image::ThreadedLoop (progress_message, vox1).run (__Foreach_Functor2<Functor, VoxelType1, VoxelType2> (noutputs, functor, vox1, vox2), "foreach thread");
      }

    template <class Functor, class VoxelType1, class VoxelType2, class VoxelType3> 
      inline void foreach (const std::string& progress_message, int noutputs, Functor functor, VoxelType1 vox1, VoxelType2& vox2, VoxelType3& vox3)
      {
        Image::ThreadedLoop (progress_message, vox1).run (__Foreach_Functor3<Functor, VoxelType1, VoxelType2, VoxelType3> (noutputs, functor, vox1, vox2, vox3), "foreach thread");
      }

    //! \}
    //! \}

  }
}

#endif



