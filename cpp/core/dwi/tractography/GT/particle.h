/* Copyright (c) 2008-2026 the MRtrix3 contributors.
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

#pragma once

#include "spinlock.h"
#include "types.h"

namespace MR::DWI::Tractography::GT {

using Point_t = Eigen::Vector3f;

/**
 * A particle is a segment of a track and consists of a position and a direction.
 */
class Particle {
public:
  // Particle length
  static float L;

  // Constructors and destructor --------------------------------------------------

  Particle() {
    predecessor = nullptr;
    successor = nullptr;
    visited = false;
    alive = false;
  }

  Particle(const Point_t &p, const Point_t &d) { init(p, d); }

  ~Particle() { finalize(); }

  inline void init(const Point_t &p, const Point_t &d) {
    const std::lock_guard<SpinLock> lock(spinlock);
    pos = p;
    dir = d;
    predecessor = nullptr;
    successor = nullptr;
    visited = false;
    alive = true;
  }

  inline void finalize() {
    const std::lock_guard<SpinLock> lock(spinlock);
    if (predecessor)
      removePredecessor_nolock();
    if (successor)
      removeSuccessor_nolock();
    alive = false;
  }

  // disable copy and assignment
  Particle(const Particle &) = delete;
  Particle &operator=(const Particle &) = delete;

  // disable move constructor and assignment
  Particle(Particle &&) = delete;
  Particle &operator=(Particle &&) = delete;

  // Getters and setters ----------------------------------------------------------

  Point_t getPosition() const {
    const std::lock_guard<SpinLock> lock(spinlock);
    return pos;
  }

  void setPosition(const Point_t &p) {
    const std::lock_guard<SpinLock> lock(spinlock);
    pos = p;
  }

  Point_t getDirection() const {
    const std::lock_guard<SpinLock> lock(spinlock);
    return dir;
  }

  void setDirection(const Point_t &d) {
    const std::lock_guard<SpinLock> lock(spinlock);
    dir = d;
    dir.normalize();
  }

  Point_t getEndPoint(const int a) const {
    const std::lock_guard<SpinLock> lock(spinlock);
    return (pos + a * L * dir);
  }

  bool hasPredecessor() const {
    const std::lock_guard<SpinLock> lock(spinlock);
    return (predecessor != nullptr);
  }

  Particle *getPredecessor() const {
    const std::lock_guard<SpinLock> lock(spinlock);
    return predecessor;
  }

  void connectPredecessor(Particle *p1, const int a1) {
    const std::lock_guard<SpinLock> lock(spinlock);
    assert(p1 != nullptr);
    setPredecessor_nolock(p1);
    if (a1 == 1)
      p1->setSuccessor(this);
    if (a1 == -1)
      p1->setPredecessor(this);
  }

  void setPredecessor(Particle *p1) {
    const std::lock_guard<SpinLock> lock(spinlock);
    setPredecessor_nolock(p1);
  }

  void setSuccessor(Particle *p1) {
    const std::lock_guard<SpinLock> lock(spinlock);
    setSuccessor_nolock(p1);
  }

  void removePredecessor() {
    const std::lock_guard<SpinLock> lock(spinlock);
    removePredecessor_nolock();
  }

  void removeSuccessor() {
    const std::lock_guard<SpinLock> lock(spinlock);
    removeSuccessor_nolock();
  }

  bool hasSuccessor() const {
    const std::lock_guard<SpinLock> lock(spinlock);
    return (successor != nullptr);
  }

  Particle *getSuccessor() const {
    const std::lock_guard<SpinLock> lock(spinlock);
    return successor;
  }

  void connectSuccessor(Particle *p1, const int a1) {
    assert(p1 != nullptr);
    const std::lock_guard<SpinLock> lock(spinlock);
    setSuccessor_nolock(p1);
    if (a1 == 1)
      p1->setSuccessor(this);
    if (a1 == -1)
      p1->setPredecessor(this);
  }

  bool isVisited() const {
    const std::lock_guard<SpinLock> lock(spinlock);
    return visited;
  }

  void setVisited(const bool v) {
    const std::lock_guard<SpinLock> lock(spinlock);
    visited = v;
  }

  bool isAlive() const {
    const std::lock_guard<SpinLock> lock(spinlock);
    return alive;
  }

protected:
  mutable SpinLock spinlock;
  Point_t pos, dir;
  Particle *predecessor;
  Particle *successor;
  bool visited;
  bool alive;

  void setPredecessor_nolock(Particle *p1) {
    if (predecessor == p1)
      return;
    if (predecessor)
      removePredecessor_nolock();
    predecessor = p1;
  }

  void setSuccessor_nolock(Particle *p1) {
    if (successor == p1)
      return;
    if (successor)
      removeSuccessor_nolock();
    successor = p1;
  }

  void removePredecessor_nolock() {
    assert(predecessor != nullptr);
    assert(predecessor != this);
    assert(predecessor->predecessor == this || predecessor->successor == this);
    const std::lock_guard<SpinLock> lock(predecessor->spinlock);
    if (predecessor->predecessor == this)
      predecessor->predecessor = nullptr;
    if (predecessor->successor == this)
      predecessor->successor = nullptr;
    predecessor = nullptr;
  }

  void removeSuccessor_nolock() {
    assert(successor != nullptr);
    assert(successor != this);
    assert(successor->predecessor == this || successor->successor == this);
    const std::lock_guard<SpinLock> lock(successor->spinlock);
    if (successor->predecessor == this)
      successor->predecessor = nullptr;
    if (successor->successor == this)
      successor->successor = nullptr;
    successor = nullptr;
  }
};

/**
 * Small data structure that refers to one end of a particle.
 * It is used to represent candidate neighbours of a given particle,
 * and to represent a pending fibre track.
 */
struct ParticleEnd {
  Particle *par = nullptr;
  int alpha = 0;
  float e_conn = 0.0;
  double p_suc = 1.0;
};

} // namespace MR::DWI::Tractography::GT
