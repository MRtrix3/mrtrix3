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

    template<size_t N>
      struct Apply {
        template<typename F, typename T>
          static inline void apply (F && f, T && t)
          {
            Apply<N-1>::apply (::std::forward<F>(f), ::std::forward<T>(t));
            ::std::forward<F>(f) (::std::get<N> (::std::forward<T>(t)));
          }
      };

    template<>
      struct Apply<0> {
        template<typename F, typename T>
          static inline void apply (F && f, T && t)
          {
            ::std::forward<F>(f) (::std::get<0> (::std::forward<T>(t)));
          }
      };




    template<size_t N>
      struct Unpack {
        template<typename F, typename T, typename... A>
          static inline auto unpack (F && f, T && t, A &&... a)
          -> decltype(Unpack<N-1>::unpack (
                ::std::forward<F>(f), ::std::forward<T>(t),
                ::std::get<N-1>(::std::forward<T>(t)), ::std::forward<A>(a)...
                ))
          {
            return Unpack<N-1>::unpack (::std::forward<F>(f), ::std::forward<T>(t),
                ::std::get<N-1>(::std::forward<T>(t)), ::std::forward<A>(a)...
                );
          }
      };

    template<>
      struct Unpack<0> {
        template<typename F, typename T, typename... A>
          static inline auto unpack (F && f, T &&, A &&... a)
          -> decltype(::std::forward<F>(f)(::std::forward<A>(a)...))
          {
            return ::std::forward<F>(f)(::std::forward<A>(a)...);
          }
      };

  }




  //! invoke \c f(x) for each entry in \c t
  template <class F, class T>
    inline void apply (F && f, T && t) 
    {
      Apply<::std::tuple_size<
        typename ::std::decay<T>::type
        >::value-1>::apply (::std::forward<F>(f), ::std::forward<T>(t));
    }

  //! if \c t is a tuple of elements \c a..., invoke \c f(a...)
  template<typename F, typename T>
    inline auto unpack (F && f, T && t)
    -> decltype(Unpack< ::std::tuple_size<
        typename ::std::decay<T>::type
        >::value>::unpack (::std::forward<F>(f), ::std::forward<T>(t)))
    {
      return Unpack< ::std::tuple_size<
        typename ::std::decay<T>::type
        >::value>::unpack (::std::forward<F>(f), ::std::forward<T>(t));
    }
}

#endif

