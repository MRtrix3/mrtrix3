/* Copyright (c) 2008-2023 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#ifndef __apply_h__
#define __apply_h__

#include <tuple>
#include <type_traits>

#include "types.h" // For FORCE_INLINE

namespace MR {
namespace {

template <size_t N> struct Apply {
  template <typename F, typename T> static FORCE_INLINE void apply(F &&f, T &&t) {
    Apply<N - 1>::apply(::std::forward<F>(f), ::std::forward<T>(t));
    ::std::forward<F>(f)(::std::get<N>(::std::forward<T>(t)));
  }
};

template <> struct Apply<0> {
  template <typename F, typename T> static FORCE_INLINE void apply(F &&f, T &&t) {
    ::std::forward<F>(f)(::std::get<0>(::std::forward<T>(t)));
  }
};

} // namespace

//! invoke \c f(x) for each entry in \c t
template <class F, class T> FORCE_INLINE void apply(F &&f, T &&t) {
  Apply<::std::tuple_size<typename ::std::decay<T>::type>::value - 1>::apply(::std::forward<F>(f),
                                                                             ::std::forward<T>(t));
}

} // namespace MR

#endif
