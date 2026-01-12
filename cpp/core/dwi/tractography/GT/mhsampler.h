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

#include "header.h"
#include "image.h"
#include "transform.h"

#include "math/rng.h"

#include "dwi/tractography/GT/energy.h"
#include "dwi/tractography/GT/gt.h"
#include "dwi/tractography/GT/particle.h"
#include "dwi/tractography/GT/particlegrid.h"
#include "dwi/tractography/GT/spatiallock.h"

namespace MR::DWI::Tractography::GT {

/**
 * @brief The MHSampler class
 */
class MHSampler {
public:
  MHSampler(const Header &dwiheader, Properties &p, Stats &s, ParticleGrid &pgrid, EnergyComputer *e, Image<bool> &m)
      : props(p),
        stats(s),
        pGrid(pgrid),
        E(e),
        T(dwiheader),
        dims{static_cast<size_t>(dwiheader.size(0)),
             static_cast<size_t>(dwiheader.size(1)),
             static_cast<size_t>(dwiheader.size(2))},
        mask(m),
        lock(std::make_shared<SpatialLock<float>>(std::max(5.0F * Particle::L, float(2.0F * pGrid.spacing())))),
        sigpos(Particle::L / 8.),
        sigdir(0.2) {
    DEBUG("Initialise Metropolis Hastings sampler.");
  }

  MHSampler(const MHSampler &other)
      : props(other.props),
        stats(other.stats),
        pGrid(other.pGrid),
        E(other.E->clone()),
        T(other.T),
        dims(other.dims),
        mask(other.mask),
        lock(other.lock),
        rng_uniform(),
        rng_normal(),
        sigpos(other.sigpos),
        sigdir(other.sigdir) {
    DEBUG("Copy Metropolis Hastings sampler.");
  }

  ~MHSampler() { delete E; }

  void execute();

  void next();

  void birth();
  void death();
  void randshift();
  void optshift();
  void connect();

protected:
  Properties &props;
  Stats &stats;
  ParticleGrid &pGrid;
  EnergyComputer
      *E; // Polymorphic copy requires call to EnergyComputer::clone(), hence references or smart pointers won't do.

  Transform T;
  std::vector<size_t> dims;
  Image<bool> mask;

  std::shared_ptr<SpatialLock<float>> lock;
  Math::RNG::Uniform<float> rng_uniform;
  Math::RNG::Normal<float> rng_normal;
  float sigpos, sigdir;

  Point_t getRandPosInMask();

  bool inMask(const Point_t p);

  Point_t getRandDir();

  void moveRandom(const Particle *par, Point_t &pos, Point_t &dir);

  bool moveOptimal(const Particle *par, Point_t &pos, Point_t &dir) const;

  inline double calcShiftProb(const Particle *par, const Point_t &pos, const Point_t &dir) const {
    Point_t Dpos = par->getPosition() - pos;
    Point_t Ddir = par->getDirection() - dir;
    return gaussian_pdf(Dpos, sigpos) * gaussian_pdf(Ddir, sigdir);
  }

  inline double gaussian_pdf(const Point_t &x, double sigma) const {
    return std::exp(-x.squaredNorm() / (2 * sigma)) / std::sqrt(2 * Math::pi * sigma * sigma);
  }
};

} // namespace MR::DWI::Tractography::GT
