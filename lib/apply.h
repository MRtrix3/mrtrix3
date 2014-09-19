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

#ifndef __apply_h__
#define __apply_h__

#include <tuple>
#include <type_traits>

namespace MR {
  namespace {

    template <int ...>
      struct IntegerTemplateSequence { };

    template <int N, int... S>
      struct GenIntegerTemplateSequence : GenIntegerTemplateSequence<N-1, N-1, S...> { };

    template <int ...S>
      struct GenIntegerTemplateSequence<0, S...> {
        typedef IntegerTemplateSequence<S...> type;
      };

    template<class F, typename... Args, int ...S>
      inline auto __unpack_seq_impl (F& f, std::tuple<Args...>& t, IntegerTemplateSequence<S...>) 
      -> decltype (f (std::get<S>(t) ...))
      {
        return f (std::get<S>(t) ...);
      }

    template <class F, typename... Args>
      inline auto __unpack_impl (F& f, std::tuple<Args...>& t) 
      -> decltype (__unpack_seq_impl (f, t, typename GenIntegerTemplateSequence<sizeof...(Args)>::type()))
      {
        return __unpack_seq_impl (f, t, typename GenIntegerTemplateSequence<sizeof...(Args)>::type());
      }



    template <class F, size_t I = 0, typename... Args, typename std::enable_if<I == sizeof...(Args), int>::type = 0>
      inline void __apply_impl (F& f, std::tuple<Args...>& t) { }

    template <class F, size_t I = 0, typename... Args, typename std::enable_if<I < sizeof...(Args), int>::type = 0>
      inline void __apply_impl (F& f, std::tuple<Args...>& t) {
        f (std::get<I> (t));
        __apply_impl<F, I+1, Args...> (f, t);
      }
  }

  template <class F, typename... Args>
    inline void apply (F& f, std::tuple<Args...>& t) { __apply_impl (f, t); }
  template <class F, typename... Args>
    inline void apply (F&& f, std::tuple<Args...>& t) { __apply_impl (f, t); }
  template <class F, typename... Args>
    inline void apply (F& f, std::tuple<Args...>&& t) { __apply_impl (f, t); }
  template <class F, typename... Args>
    inline void apply (F&& f, std::tuple<Args...>&& t) { __apply_impl (f, t); }


  template <class F, typename... Args>
    inline auto unpack (F& f, std::tuple<Args...>& t) -> decltype (__unpack_impl (f, t)) { return __unpack_impl (f, t); } 
  template <class F, typename... Args>
    inline auto unpack (F&& f, std::tuple<Args...>& t) -> decltype (__unpack_impl (f, t)) { return __unpack_impl (f, t); } 
  template <class F, typename... Args>
    inline auto unpack (F& f, std::tuple<Args...>&& t) -> decltype (__unpack_impl (f, t)) { return __unpack_impl (f, t); } 
  template <class F, typename... Args>
    inline auto unpack (F&& f, std::tuple<Args...>&& t) -> decltype (__unpack_impl (f, t)) { return __unpack_impl (f, t); } 
}

#endif

