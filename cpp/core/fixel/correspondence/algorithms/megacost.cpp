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

#include "fixel/correspondence/algorithms/megacost.h"

#include <cmath>
#include <cstdio>
#include <limits>

#include "math/math.h"

// Note: DP2Cost (fixel/correspondence/dp2cost.h) is made available transitively via
//   combinatorial.h (included by megacost.h); that header lacks an include guard, so it
//   must not be included a second time here.

namespace MR::Fixel::Correspondence::Algorithms {

namespace {

// Local fast lookup for the tan(acos(.)) angular penalisation used by the
//   "ismrm2018" and "rs2023" cost functions. Distinct from (but equivalent to)
//   the per-instantiation Combinatorial<>::dp2cost; kept file-local so that the
//   parameterised cost evaluations are self-contained and depend on no statics.
const DP2Cost dp2cost;

// Shared source-mass transport evaluation, mirroring Transport::transport_core but
//   driven entirely by the configuration's own parameters (no statics).
float transport_core(const std::vector<Correspondence::Fixel> &s,
                     const std::vector<Correspondence::Fixel> &rs,
                     const std::vector<Correspondence::Fixel> &t,
                     const std::vector<std::vector<index_type>> &inv_mapping,
                     const Eigen::Array<int8_t, Eigen::Dynamic, 1> &origins_per_remapped_fixel,
                     const AngularKernel kernel,
                     const float cap,
                     const float gamma) {
  float result = 0.0f;
  for (index_type s_index = 0; s_index != s.size(); ++s_index) {
    const float d_s = s[s_index].density();
    const std::size_t n_objectives = inv_mapping[s_index].size();
    if (n_objectives == 0) {
      result += d_s * cap;
    } else {
      const float mass = d_s / static_cast<float>(n_objectives);
      for (const index_type t_index : inv_mapping[s_index])
        result += mass * angular_cost(s[s_index].absdot(t[t_index]), kernel);
      if (n_objectives > 1)
        result += gamma * d_s * static_cast<float>(n_objectives - 1);
    }
  }
  for (index_type t_index = 0; t_index != rs.size(); ++t_index) {
    const int n_origins = static_cast<int>(origins_per_remapped_fixel[t_index]);
    if (n_origins > 1)
      result += gamma * rs[t_index].density() * static_cast<float>(n_origins - 1);
  }
  return result;
}

} // namespace

float CostConfig::evaluate(const std::vector<Correspondence::Fixel> &s,
                           const std::vector<Correspondence::Fixel> &rs,
                           const std::vector<Correspondence::Fixel> &t,
                           const std::vector<std::vector<index_type>> &inv_mapping,
                           const Eigen::Array<int8_t, Eigen::Dynamic, 1> &origins_per_remapped_fixel) const {
  assert(rs.size() == t.size());
  assert(s.size() == inv_mapping.size());

  switch (family) {

  case Family::ISMRM2018: {
    float result = 0.0f;
    for (index_type index = 0; index != rs.size(); ++index) {
      if (rs[index].density())
        result += Math::pow2(t[index].density() - rs[index].density()) * dp2cost(t[index].absdot(rs[index]));
      else
        result += Math::pow2(t[index].density());
    }
    for (index_type index = 0; index != s.size(); ++index) {
      if (inv_mapping[index].empty())
        result += Math::pow2(s[index].density());
    }
    return result;
  }

  case Family::POT: {
    float result = 0.0f;
    for (index_type t_index = 0; t_index != rs.size(); ++t_index) {
      const float d_t = t[t_index].density();
      const float d_rs = rs[t_index].density();
      const float matched_mass = std::min(d_t, d_rs);
      if (matched_mass > 0.0f) {
        const float sq_dp = Math::pow2(t[t_index].dot(rs[t_index]));
        if (sq_dp > 0.0f)
          result += 2.0f * matched_mass * (1.0f - sq_dp) / sq_dp;
        else
          return std::numeric_limits<float>::infinity();
      }
      result += std::fabs(d_t - d_rs);
      const int n_origins = static_cast<int>(origins_per_remapped_fixel[t_index]);
      if (n_origins > 1)
        result += gamma * d_t * static_cast<float>(n_origins - 1);
    }
    for (index_type s_index = 0; s_index != s.size(); ++s_index) {
      const float d_s = s[s_index].density();
      const std::size_t n_objectives = inv_mapping[s_index].size();
      if (n_objectives == 0)
        result += d_s;
      else if (n_objectives > 1)
        result += gamma * d_s * static_cast<float>(n_objectives - 1);
    }
    return result;
  }

  case Family::RS2023: {
    float result = 0.0f;
    for (index_type t_index = 0; t_index != rs.size(); ++t_index) {
      result += t[t_index].density() * (rs[t_index].density() ? dp2cost(t[t_index].absdot(rs[t_index])) : 1.0f);
      result += alpha * Math::pow2(t[t_index].density() - rs[t_index].density());
      result += beta * Math::pow2(static_cast<float>(static_cast<int>(origins_per_remapped_fixel[t_index]) - 1));
    }
    for (index_type s_index = 0; s_index != s.size(); ++s_index) {
      if (inv_mapping[s_index].empty()) {
        result += s[s_index].density();
        result += alpha * Math::pow2(s[s_index].density());
      }
      result += beta * Math::pow2(static_cast<float>(static_cast<int>(inv_mapping[s_index].size()) - 1));
    }
    return result;
  }

  case Family::TRANSPORT:
    return transport_core(s, rs, t, inv_mapping, origins_per_remapped_fixel, kernel, cap, gamma);

  case Family::TRANSPORTDISP: {
    std::vector<std::vector<index_type>> origins(t.size());
    for (index_type s_index = 0; s_index != s.size(); ++s_index) {
      for (const index_type t_index : inv_mapping[s_index])
        origins[t_index].push_back(s_index);
    }
    float result = 0.0f;
    for (index_type t_index = 0; t_index != rs.size(); ++t_index) {
      const float d_rs = rs[t_index].density();
      if (d_rs <= 0.0f)
        continue;
      const float align = angular_cost(rs[t_index].absdot(t[t_index]), kernel);
      dir_t resultant = dir_t::Zero();
      for (const index_type s_index : origins[t_index]) {
        const float mass = s[s_index].density() / static_cast<float>(inv_mapping[s_index].size());
        const float sign = (t[t_index].dot(s[s_index]) < 0.0f) ? -1.0f : 1.0f;
        resultant += mass * sign * s[s_index].dir();
      }
      const float R = resultant.norm() / d_rs;
      result += d_rs * (align + lambda * (1.0f - R));
      if (origins[t_index].size() > 1)
        result += gamma * d_rs * static_cast<float>(origins[t_index].size() - 1);
    }
    for (index_type s_index = 0; s_index != s.size(); ++s_index) {
      const float d_s = s[s_index].density();
      const std::size_t n_objectives = inv_mapping[s_index].size();
      if (n_objectives == 0)
        result += d_s * cap;
      else if (n_objectives > 1)
        result += gamma * d_s * static_cast<float>(n_objectives - 1);
    }
    return result;
  }

  case Family::AGREEMENT: {
    const float sigma2 = Math::pow2(sigma);
    float result = 0.0f;
    for (index_type t_index = 0; t_index != rs.size(); ++t_index) {
      const float d_t = t[t_index].density();
      const float d_rs = rs[t_index].density();
      if (d_rs > 0.0f) {
        const float dd = d_t - d_rs;
        const float a = angular_cost(rs[t_index].absdot(t[t_index]), kernel);
        result += sigma2 * a * (1.0f - std::exp(-Math::pow2(dd) / sigma2));
      } else {
        result += Math::pow2(d_t);
      }
      const int n_origins = static_cast<int>(origins_per_remapped_fixel[t_index]);
      if (n_origins > 1)
        result += beta * Math::pow2(static_cast<float>(n_origins - 1));
    }
    for (index_type s_index = 0; s_index != s.size(); ++s_index) {
      const std::size_t n_objectives = inv_mapping[s_index].size();
      if (n_objectives == 0)
        result += Math::pow2(s[s_index].density());
      else if (n_objectives > 1)
        result += beta * Math::pow2(static_cast<float>(n_objectives - 1));
    }
    return result;
  }

  case Family::TRANSPORTGUARD: {
    float result = transport_core(s, rs, t, inv_mapping, origins_per_remapped_fixel, kernel, cap, gamma);
    for (index_type t_index = 0; t_index != rs.size(); ++t_index) {
      const float excess = rs[t_index].density() - rho * t[t_index].density();
      if (excess > 0.0f)
        result += mu * Math::pow2(excess);
    }
    return result;
  }
  }

  assert(0);
  return std::numeric_limits<float>::infinity();
}

namespace {

std::string fnum(const float value) {
  char buffer[32];
  std::snprintf(buffer, sizeof(buffer), "%g", value);
  std::string result(buffer);
  for (char &c : result) {
    if (c == '.')
      c = 'p';
    else if (c == '-')
      c = 'm';
  }
  return result;
}

std::string kernel_name(const AngularKernel kernel) { return kernel == AngularKernel::TAN2 ? "tan2" : "tan"; }

float angle_to_cap(const float angle_degrees, const AngularKernel kernel) {
  const float cos_theta_star = static_cast<float>(std::cos(angle_degrees * Math::pi / 180.0));
  return angular_cost(cos_theta_star, kernel);
}

// -----------------------------------------------------------------------------
// The complete set of cost functions and internal parameter values to evaluate.
//
// Parameter ranges below were chosen to span the plausible useful range of each
//   knob around its production default while keeping the total configuration
//   count modest. Memory: the per-configuration output is one CSR mapping over
//   all target fixels. Worst case (750,000 voxels x 5 target fixels, with up to
//   max_origins source fixels each) is ~270 MB per configuration; the 100 GB
//   ceiling therefore permits a few hundred configurations. The sweep below is
//   ~85 configurations (well under the ceiling, and far smaller in practice as
//   real fixel datasets are much sparser).
// -----------------------------------------------------------------------------
std::vector<CostConfig> build_configs() {
  std::vector<CostConfig> c;

  // -- ismrm2018: no internal parameters -------------------------------------
  {
    CostConfig cfg;
    cfg.family = CostConfig::Family::ISMRM2018;
    cfg.name = "ismrm2018";
    c.push_back(cfg);
  }

  // -- pot: complexity gamma --------------------------------------------------
  for (const float gamma : {0.0f, 0.1f, 0.25f, 0.5f, 0.75f, 1.0f, 1.5f, 2.0f}) {
    CostConfig cfg;
    cfg.family = CostConfig::Family::POT;
    cfg.gamma = gamma;
    cfg.name = "pot_g" + fnum(gamma);
    c.push_back(cfg);
  }

  // -- rs2023: density-difference weight alpha, parsimony weight beta ---------
  for (const float alpha : {0.1f, 0.25f, 0.5f, 1.0f, 2.0f}) {
    for (const float beta : {0.0f, 0.1f, 0.25f, 0.5f}) {
      CostConfig cfg;
      cfg.family = CostConfig::Family::RS2023;
      cfg.alpha = alpha;
      cfg.beta = beta;
      cfg.name = "rs2023_a" + fnum(alpha) + "_b" + fnum(beta);
      c.push_back(cfg);
    }
  }

  // -- transport: kernel, threshold angle theta*, complexity gamma -----------
  for (const AngularKernel kernel : {AngularKernel::TAN, AngularKernel::TAN2}) {
    for (const float angle : {30.0f, 45.0f, 60.0f}) {
      for (const float gamma : {0.0f, 0.25f, 0.5f, 1.0f}) {
        CostConfig cfg;
        cfg.family = CostConfig::Family::TRANSPORT;
        cfg.kernel = kernel;
        cfg.angle = angle;
        cfg.cap = angle_to_cap(angle, kernel);
        cfg.gamma = gamma;
        cfg.name = "transport_" + kernel_name(kernel) + "_ang" + fnum(angle) + "_g" + fnum(gamma);
        c.push_back(cfg);
      }
    }
  }

  // -- transportdisp: + dispersion weight lambda -----------------------------
  for (const float gamma : {0.25f, 0.5f}) {
    for (const float lambda : {0.25f, 0.5f, 1.0f, 2.0f}) {
      CostConfig cfg;
      cfg.family = CostConfig::Family::TRANSPORTDISP;
      cfg.kernel = AngularKernel::TAN2;
      cfg.angle = 45.0f;
      cfg.cap = angle_to_cap(45.0f, AngularKernel::TAN2);
      cfg.gamma = gamma;
      cfg.lambda = lambda;
      cfg.name = "transportdisp_tan2_ang45_g" + fnum(gamma) + "_l" + fnum(lambda);
      c.push_back(cfg);
    }
  }

  // -- agreement: contrast-protection scale sigma, parsimony weight beta -----
  for (const float sigma : {0.25f, 0.5f, 1.0f, 2.0f, 4.0f}) {
    for (const float beta : {0.0f, 0.1f, 0.5f}) {
      CostConfig cfg;
      cfg.family = CostConfig::Family::AGREEMENT;
      cfg.kernel = AngularKernel::TAN2;
      cfg.sigma = sigma;
      cfg.beta = beta;
      cfg.name = "agreement_tan2_s" + fnum(sigma) + "_b" + fnum(beta);
      c.push_back(cfg);
    }
  }

  // -- transportguard: + over-explanation weight mu, density ratio rho -------
  for (const float mu : {0.5f, 1.0f, 2.0f}) {
    for (const float rho : {1.5f, 2.0f, 3.0f}) {
      CostConfig cfg;
      cfg.family = CostConfig::Family::TRANSPORTGUARD;
      cfg.kernel = AngularKernel::TAN2;
      cfg.angle = 45.0f;
      cfg.cap = angle_to_cap(45.0f, AngularKernel::TAN2);
      cfg.gamma = 0.5f;
      cfg.mu = mu;
      cfg.rho = rho;
      cfg.name = "transportguard_tan2_ang45_g0p5_mu" + fnum(mu) + "_rho" + fnum(rho);
      c.push_back(cfg);
    }
  }

  return c;
}

} // namespace

const std::vector<CostConfig> &MegaCost::configs() {
  static const std::vector<CostConfig> instance = build_configs();
  return instance;
}

thread_local MegaCost::Scratch MegaCost::scratch;

void MegaCost::Scratch::reset(const size_t nconfig, const size_t nsource) {
  best_cost.assign(nconfig, std::numeric_limits<float>::infinity());
  // Reuse existing allocations across voxels where possible: resize the outer
  //   structures and clear (rather than reallocate) the inner per-source vectors.
  best_inv.resize(nconfig);
  for (size_t k = 0; k != nconfig; ++k) {
    best_inv[k].resize(nsource);
    for (auto &per_source : best_inv[k])
      per_source.clear();
  }
}

float MegaCost::calculate(const std::vector<Correspondence::Fixel> &s,
                          const std::vector<Correspondence::Fixel> &rs,
                          const std::vector<Correspondence::Fixel> &t,
                          const std::vector<std::vector<index_type>> &inv_mapping,
                          const Eigen::Array<int8_t, Eigen::Dynamic, 1> &origins_per_remapped_fixel) {
  const std::vector<CostConfig> &cfgs = configs();
  float reference_cost = 0.0f;
  for (size_t k = 0; k != cfgs.size(); ++k) {
    const float cost = cfgs[k].evaluate(s, rs, t, inv_mapping, origins_per_remapped_fixel);
    if (k == 0)
      reference_cost = cost;
    if (cost < scratch.best_cost[k]) {
      scratch.best_cost[k] = cost;
      // Record this candidate's inverse mapping (voxel-local) for this configuration;
      //   the forward mapping is reconstructed from it after enumeration completes.
      std::vector<std::vector<index_type>> &dst = scratch.best_inv[k];
      dst.resize(inv_mapping.size());
      for (size_t i = 0; i != inv_mapping.size(); ++i)
        dst[i] = inv_mapping[i];
    }
  }
  // Combinatorial<>::operator() uses this scalar only for its own (here unused)
  //   single-best tracking and the optional per-voxel cost image; returning the
  //   first configuration's cost keeps both coherent.
  return reference_cost;
}

const std::vector<std::vector<std::vector<index_type>>> &MegaCost::evaluate_all(
    const voxel_t &v, const std::vector<Correspondence::Fixel> &s, const std::vector<Correspondence::Fixel> &t) const {
  assert(!t.empty());
  scratch.reset(configs().size(), s.size());
  // Reuse the combinatorial enumeration verbatim; each candidate is routed through
  //   calculate(), which updates scratch.best_inv per configuration.
  (*this)(v, s, t);
  return scratch.best_inv;
}

} // namespace MR::Fixel::Correspondence::Algorithms
