/* Copyright (c) 2008-2024 the MRtrix3 contributors.
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

#ifndef MUTEXPROTECTED_H
#define MUTEXPROTECTED_H

#include <mutex>

// This class is a wrapper around an object that provides a mutex-protected
// interface to the object. The object is constructed in place, and the
// constructor forwards its arguments to the constructor of the object.
// The lock() method returns a Guard object that provides access to the
// object. The Guard object locks the mutex in its constructor and unlocks
// the mutex in its destructor. This ensures that the mutex is always
// unlocked when the Guard object goes out of scope, even if an exception
// is thrown.
// Usage example:
// MutexProtected<std::vector<int>> protectedVector;
// {
//   auto guard = protectedVector.lock();
//   guard->push_back(42);
// }

template <typename Object> class MutexProtected {
public:
  // Constructor forwards arguments to the constructor of the object
  template <typename... Args> MutexProtected(Args &&...args) : m_object{std::forward<Args>(args)...} {}

  // Guard is not copy-constructible or copy-assignable
  // NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members)
  class Guard {
  public:
    Guard(MutexProtected &protectedObject) : m_protectedObject(protectedObject), m_lock(protectedObject.m_mutex) {}
    Guard(const Guard &) = delete;
    Guard &operator=(const Guard &) = delete;
    Guard(Guard &&) noexcept = default;
    Guard &operator=(Guard &&) noexcept = default;
    ~Guard() = default;

    Object &operator*() { return m_protectedObject.m_object; }
    Object *operator->() { return &m_protectedObject.m_object; }

  private:
    MutexProtected &m_protectedObject;
    std::lock_guard<std::mutex> m_lock;
  };
  // NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)

  Guard lock() { return Guard(*this); }

private:
  std::mutex m_mutex;
  Object m_object;
};
#endif // MUTEXPROTECTED_H
