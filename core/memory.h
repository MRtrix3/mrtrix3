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

#ifndef __memory_h__
#define __memory_h__

#include <memory>

#define NOMEMALIGN

/** \defgroup Memory Memory management
 * \brief Classes & functions to ease memory management. */

namespace MR
{

  template<class T, class Deleter = std::default_delete<T>>
    class copy_ptr : public std::unique_ptr<T, Deleter>
  { NOMEMALIGN
    public:
      constexpr copy_ptr () noexcept : std::unique_ptr<T,Deleter>() { }
      constexpr copy_ptr (std::nullptr_t) noexcept : std::unique_ptr<T,Deleter>() { }
      explicit copy_ptr (T* p) noexcept : std::unique_ptr<T,Deleter> (p) { }
      copy_ptr (const copy_ptr& u) : std::unique_ptr<T,Deleter>(u ? new T (*u) : nullptr) { }
      copy_ptr (copy_ptr&& u) noexcept : std::unique_ptr<T,Deleter>(std::move(u)) { }
      template< class U, class E >
        copy_ptr (copy_ptr<U, E>&& u) noexcept : std::unique_ptr<T,Deleter>(std::move(u)) { }

      copy_ptr& operator=(const copy_ptr& u) { this->reset (u ? new T (*u) : nullptr); return *this; }
  };

  struct compare_ptr_contents { NOMEMALIGN
    template <class X>
      bool operator() (const X& a, const X& b) const { return *a < *b; }
    template <class X>
      bool operator() (const std::shared_ptr<X>& a, const std::shared_ptr<X>& b) const { return *a < *b; }
  };

}

#endif

