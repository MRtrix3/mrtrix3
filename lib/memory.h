/*
    Copyright 2015 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 08/04/2015

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

#ifndef __memory_h__
#define __memory_h__

#include <memory>

/** \defgroup Memory Memory management
 * \brief Classes & functions to ease memory management. */

namespace MR
{

  template<class T, class Deleter = std::default_delete<T>> 
    class copy_ptr : public std::unique_ptr<T, Deleter>
  {
    public:
      constexpr copy_ptr () noexcept : std::unique_ptr<T,Deleter>() { }
      constexpr copy_ptr (std::nullptr_t) noexcept : std::unique_ptr<T,Deleter>() { }
      explicit copy_ptr (T* p) noexcept : std::unique_ptr<T,Deleter> (p) { }
      copy_ptr (const copy_ptr& u) : std::unique_ptr<T,Deleter>(new T (*u)) { }
      copy_ptr (copy_ptr&& u) noexcept : std::unique_ptr<T,Deleter>(std::move(u)) { }
      template< class U, class E >
        copy_ptr (copy_ptr<U, E>&& u) noexcept : std::unique_ptr<T,Deleter>(std::move(u)) { }

      copy_ptr& operator=(const copy_ptr& u) { this->reset (new T (*u)); return *this; }
  };

}

#endif

