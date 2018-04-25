/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#ifndef __algo_random_threaded_loop_h__
#define __algo_random_threaded_loop_h__

#include "debug.h"
#include "exception.h"
#include "algo/loop.h"
#include "algo/iterator.h"
#include "thread.h"
#include "math/rng.h"
#include <algorithm>  // std::shuffle
#include <random>
// #include "algo/random_loop.h"

namespace MR
{

  namespace {


    template <int N, class Functor, class... ImageType>
      struct RandomThreadedLoopRunInner
      { MEMALIGN(RandomThreadedLoopRunInner<N,Functor,ImageType...>)
        const vector<size_t>& outer_axes;
        decltype (Loop (outer_axes)) loop;
        typename std::remove_reference<Functor>::type func;
        double density;
        Math::RNG::Uniform<double> rng;
        const vector<size_t> dims;
        std::tuple<ImageType...> vox;

        RandomThreadedLoopRunInner (const vector<size_t>& outer_axes, const vector<size_t>& inner_axes,
            const Functor& functor, const double voxel_density, const vector<size_t> dimensions, ImageType&... voxels) :
          outer_axes (outer_axes),
          loop (Loop (inner_axes)),
          func (functor),
          density (voxel_density),
          dims(dimensions),
          vox (voxels...) { }

        void operator() (const Iterator& pos) {
          assign_pos_of (pos, outer_axes).to (vox);
          for (auto i = unpack (loop, vox); i; ++i) {
            // if (rng() >= density){
            //   DEBUG (str(pos) + " ...skipped inner");
            //   continue;
            // }
            // DEBUG (str(pos) + " ...used inner");
            unpack (func, vox);
          }
        }
      };


    template <class Functor, class... ImageType>
      struct RandomThreadedLoopRunInner<0,Functor,ImageType...>
      { MEMALIGN(RandomThreadedLoopRunInner<0,Functor,ImageType...>)
        const vector<size_t>& outer_axes;
        const vector<size_t> inner;
        decltype (Loop (outer_axes)) loop;
        // Random_loop<Iterator, std::default_random_engine>  random_loop;
        typename std::remove_reference<Functor>::type func;
        double density;
        size_t max_cnt;
        size_t cnt;
        // Math::RNG::Uniform<double> rng;
        std::default_random_engine random_engine;
        vector<size_t> idx;
        vector<size_t>::iterator it;
        vector<size_t>::iterator stop;
        const vector<size_t> dims;
        // ImageType& image;
        // RandomEngine& engine;
        // size_t max_cnt;
        // bool status;
        // size_t cnt;


        RandomThreadedLoopRunInner (const vector<size_t>& outer_axes, const vector<size_t>& inner_axes,
            const Functor& functor, const double voxel_density, const vector<size_t>& dimensions, ImageType&... voxels) :
          outer_axes (outer_axes),
          inner (inner_axes),
          loop (Loop (inner_axes)),
          func (functor),
          density (voxel_density),
          dims (dimensions) {
            assert (inner_axes.size() == 1);
            // VAR(inner_axes);
            // VAR(outer_axes);
            Math::RNG rng;
            typename std::default_random_engine::result_type seed = rng.get_seed();
            random_engine = std::default_random_engine{static_cast<std::default_random_engine::result_type>(seed)};
            idx = vector<size_t>(dims[inner_axes[0]]);
            std::iota (std::begin(idx), std::end(idx), 0);
          }

        void operator() (Iterator& pos) {
          std::shuffle (std::begin(idx), std::end(idx), random_engine);
          cnt = 0;
          it = std::begin(idx);
          stop = std::end(idx);
          max_cnt = //size_t (density * dims[inner[0]]);
          (size_t) std::ceil (density * dims[inner[0]]);
          // VAR(max_cnt);
          for (size_t i = 0; i < max_cnt; ++i){
            pos.index(inner[0]) = *it;
            it++;
          // for (auto i = loop (pos); i; ++i){
            // if (rng() >= density){
            //   DEBUG (str(pos) + " ...skipped inner.");
            //   continue;
            // }
            // DEBUG (str(pos) + " ...used inner.");
            // VAR(pos);
            // VAR(cnt);
            func (pos);
            ++cnt;
          }
          pos.index(inner[0]) = dims[inner[0]];
        }

      };


    template <class OuterLoopType>
      struct RandomThreadedLoopRunOuter { MEMALIGN(RandomThreadedLoopRunOuter<OuterLoopType>)
        Iterator iterator;
        OuterLoopType outer_loop;
        vector<size_t> inner_axes;

        //! invoke \a functor (const Iterator& pos) per voxel <em> in the outer axes only</em>
        template <class Functor>
          void run_outer (Functor&& functor, const double voxel_density, const vector<size_t>& dimensions)
          {
            if (Thread::number_of_threads() == 0) {
              for (auto i = outer_loop (iterator); i; ++i){
                // std::cerr << "outer: " << str(iterator) << " " << voxel_density << " " << dimensions << std::endl;
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

            Thread::run (Thread::multi (loop_thread), "loop threads");
          }



        //! invoke \a functor (const Iterator& pos) per voxel <em> in the outer axes only</em>
        template <class Functor, class... ImageType>
          void run (Functor&& functor, const double voxel_density, vector<size_t> dimensions, ImageType&&... vox)
          {
            RandomThreadedLoopRunInner<
              sizeof...(ImageType),
              typename std::remove_reference<Functor>::type,
              typename std::remove_reference<ImageType>::type...
                > loop_thread (outer_loop.axes, inner_axes, functor, voxel_density, dimensions, vox...);
            run_outer (loop_thread, voxel_density, dimensions);
            check_app_exit_code();
          }

      };
  }







  template <class HeaderType>
    inline RandomThreadedLoopRunOuter<decltype(Loop(vector<size_t>()))> RandomThreadedLoop (
        const HeaderType& source,
        const vector<size_t>& outer_axes,
        const vector<size_t>& inner_axes) {
      return { source, Loop (outer_axes), inner_axes };
    }


  template <class HeaderType>
    inline RandomThreadedLoopRunOuter<decltype(Loop(vector<size_t>()))> RandomThreadedLoop (
        const HeaderType& source,
        const vector<size_t>& axes,
        size_t num_inner_axes = 1) {
      return { source, Loop (get_outer_axes (axes, num_inner_axes)), get_inner_axes (axes, num_inner_axes) };
    }

  template <class HeaderType>
    inline RandomThreadedLoopRunOuter<decltype(Loop(vector<size_t>()))> RandomThreadedLoop (
        const HeaderType& source,
        size_t from_axis = 0,
        size_t to_axis = std::numeric_limits<size_t>::max(),
        size_t num_inner_axes = 1) {
      return { source,
        Loop (get_outer_axes (source, num_inner_axes, from_axis, to_axis)),
        get_inner_axes (source, num_inner_axes, from_axis, to_axis) };
      }

  template <class HeaderType>
    inline RandomThreadedLoopRunOuter<decltype(Loop("", vector<size_t>()))> RandomThreadedLoop (
        const std::string& progress_message,
        const HeaderType& source,
        const vector<size_t>& outer_axes,
        const vector<size_t>& inner_axes) {
      return { source, Loop (progress_message, outer_axes), inner_axes };
    }

  template <class HeaderType>
    inline RandomThreadedLoopRunOuter<decltype(Loop("", vector<size_t>()))> RandomThreadedLoop (
        const std::string& progress_message,
        const HeaderType& source,
        const vector<size_t>& axes,
        size_t num_inner_axes = 1) {
      return { source,
        Loop (progress_message, get_outer_axes (axes, num_inner_axes)),
        get_inner_axes (axes, num_inner_axes) };
      }

  template <class HeaderType>
    inline RandomThreadedLoopRunOuter<decltype(Loop("", vector<size_t>()))> RandomThreadedLoop (
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



