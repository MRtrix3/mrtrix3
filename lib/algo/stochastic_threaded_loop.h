/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see www.mrtrix.org
 *
 */

#ifndef __algo_stochastic_threaded_loop_h__
#define __algo_stochastic_threaded_loop_h__

#include "debug.h"
#include "algo/loop.h"
#include "algo/iterator.h"
#include "thread.h"
#include "math/rng.h"

namespace MR
{

  /* slower than ThreadedLoop for any voxel_density */



  namespace {


    template <int N, class Functor, class... ImageType>
      struct StochasticThreadedLoopRunInner
      { MEMALIGN(StochasticThreadedLoopRunInner<N,Functor,ImageType...>)
        const vector<size_t>& outer_axes;
        decltype (Loop (outer_axes)) loop;
        typename std::remove_reference<Functor>::type func;
        double density;
        Math::RNG::Uniform<double> rng;
        std::tuple<ImageType...> vox;

        StochasticThreadedLoopRunInner (const vector<size_t>& outer_axes, const vector<size_t>& inner_axes,
            const Functor& functor, const double voxel_density, ImageType&... voxels) :
          outer_axes (outer_axes),
          loop (Loop (inner_axes)),
          func (functor), 
          density (voxel_density), 
          rng (Math::RNG::Uniform<double>()),
          vox (voxels...) { }

        void operator() (const Iterator& pos) {
          assign_pos_of (pos, outer_axes).to (vox);
          for (auto i = unpack (loop, vox); i; ++i) {
            if (rng() >= density){
              //DEBUG (str(pos) + " ...skipped inner");
              continue;
            }
            // DEBUG (str(pos) + " ...used inner");
            unpack (func, vox);
          }
        }
      };


    template <class Functor, class... ImageType>
      struct StochasticThreadedLoopRunInner<0,Functor,ImageType...>
      { MEMALIGN(StochasticThreadedLoopRunInner<0, Functor,ImageType...>)
        const vector<size_t>& outer_axes;
        decltype (Loop (outer_axes)) loop;
        typename std::remove_reference<Functor>::type func;
        double density;
        Math::RNG::Uniform<double> rng;


        StochasticThreadedLoopRunInner (const vector<size_t>& outer_axes, const vector<size_t>& inner_axes,
            const Functor& functor, const double voxel_density, ImageType&... voxels) :
          outer_axes (outer_axes),
          loop (Loop (inner_axes)),
          func (functor),
          density (voxel_density),
          rng (Math::RNG::Uniform<double>()) {  }

        void operator() (Iterator& pos) {
          for (auto i = loop (pos); i; ++i){
            if (rng() >= density){
              // DEBUG (str(pos) + " ...skipped inner");
              continue;
            }
            // DEBUG (str(pos) + " ...used inner");
            func (pos);
          }
        }
      };


    template <class OuterLoopType>
      struct StochasticThreadedLoopRunOuter { MEMALIGN(StochasticThreadedLoopRunOuter<OuterLoopType>)
        Iterator iterator;
        OuterLoopType outer_loop;
        vector<size_t> inner_axes;

        //! invoke \a functor (const Iterator& pos) per voxel <em> in the outer axes only</em>
        template <class Functor> 
          void run_outer (Functor&& functor, const double voxel_density)
          {
            if (Thread::number_of_threads() == 0) {
              for (auto i = outer_loop (iterator); i; ++i){ 
                // std::cerr << "outer: " << str(iterator) << " " << voxel_density << std::endl;
                functor (iterator);
              }
              return;
            }

            struct Shared { MEMALIGN(Shared)
              Iterator& iterator;
              decltype (outer_loop (iterator)) loop;
              std::mutex mutex;
              FORCE_INLINE bool next (Iterator& pos) {
                std::lock_guard<std::mutex> lock (mutex);
                if (loop) {
                  assign_pos_of (iterator, loop.axes).to (pos);
                  ++loop;
                  return true;
                }
                else return false;
              }
            } shared = { iterator, outer_loop (iterator) };

            struct PerThread { MEMALIGN(PerThread)
              Shared& shared;
              typename std::remove_reference<Functor>::type func;
              void execute () {
                Iterator pos = shared.iterator;
                while (shared.next (pos))
                  func (pos);
              }
            } loop_thread = { shared, functor };

            auto t = Thread::run (Thread::multi (loop_thread), "loop threads");
            t.wait();
          }



        //! invoke \a functor (const Iterator& pos) per voxel <em> in the outer axes only</em>
        template <class Functor, class... ImageType>
          void run (Functor&& functor, const double voxel_density, ImageType&&... vox)
          {
            StochasticThreadedLoopRunInner< 
              sizeof...(ImageType),
              typename std::remove_reference<Functor>::type, 
              typename std::remove_reference<ImageType>::type...
                > loop_thread (outer_loop.axes, inner_axes, functor, voxel_density, vox...);
            run_outer (loop_thread, voxel_density);
          }

      };
  }







  template <class HeaderType>
    inline StochasticThreadedLoopRunOuter<decltype(Loop(vector<size_t>()))> StochasticThreadedLoop (
        const HeaderType& source,
        const vector<size_t>& outer_axes,
        const vector<size_t>& inner_axes) {
      return { source, Loop (outer_axes), inner_axes };
    }


  template <class HeaderType>
    inline StochasticThreadedLoopRunOuter<decltype(Loop(vector<size_t>()))> StochasticThreadedLoop (
        const HeaderType& source,
        const vector<size_t>& axes,
        size_t num_inner_axes = 1) {
      return { source, Loop (get_outer_axes (axes, num_inner_axes)), get_inner_axes (axes, num_inner_axes) }; 
    }

  template <class HeaderType>
    inline StochasticThreadedLoopRunOuter<decltype(Loop(vector<size_t>()))> StochasticThreadedLoop (
        const HeaderType& source,
        size_t from_axis = 0,
        size_t to_axis = std::numeric_limits<size_t>::max(),
        size_t num_inner_axes = 1) {
      return { source, 
        Loop (get_outer_axes (source, num_inner_axes, from_axis, to_axis)), 
        get_inner_axes (source, num_inner_axes, from_axis, to_axis) };
      }

  template <class HeaderType>
    inline StochasticThreadedLoopRunOuter<decltype(Loop("", vector<size_t>()))> StochasticThreadedLoop (
        const std::string& progress_message,
        const HeaderType& source,
        const vector<size_t>& outer_axes,
        const vector<size_t>& inner_axes) {
      return { source, Loop (progress_message, outer_axes), inner_axes };
    }

  template <class HeaderType>
    inline StochasticThreadedLoopRunOuter<decltype(Loop("", vector<size_t>()))> StochasticThreadedLoop (
        const std::string& progress_message,
        const HeaderType& source,
        const vector<size_t>& axes,
        size_t num_inner_axes = 1) {
      return { source, 
        Loop (progress_message, get_outer_axes (axes, num_inner_axes)),
        get_inner_axes (axes, num_inner_axes) };
      }

  template <class HeaderType>
    inline StochasticThreadedLoopRunOuter<decltype(Loop("", vector<size_t>()))> StochasticThreadedLoop (
        const std::string& progress_message,
        const HeaderType& source,
        size_t from_axis = 0,
        size_t to_axis = std::numeric_limits<size_t>::max(), 
        size_t num_inner_axes = 1) {
      return { source,
        Loop (progress_message, get_outer_axes (source, num_inner_axes, from_axis, to_axis)),
        get_inner_axes (source, num_inner_axes, from_axis, to_axis) };
      }


  /*! \} */

}

#endif



