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

#include <cstddef>
#include <vector>

#include "dwi/tractography/field_registry.h"
#include "dwi/tractography/tractogram_item.h"

namespace MR::DWI::Tractography {

//! \brief Const-shared sidecar context for multi-threaded streamline functors
//!   (§2.5 / §2.7).
/*! The "Shared" object of the processing paradigm (§1.4): a single, immutable
 * instance constructed once per run and shared by const reference with every
 * worker functor in the Reader→queue→worker→queue→Writer pipeline. It identifies
 * which sidecar data are associated with the operation and how they map from the
 * input tractogram to the output tractogram, so that a worker can address fields
 * by O(1) ordinal rather than re-resolving names per streamline.
 *
 * It owns (by const reference) the input and output field registries — distinct
 * ordinal spaces per §2.7 — and precomputes a name+role-based **pass-through
 * map**: for every field present in both registries with a compatible role,
 * datatype and column count M, an (input ordinal → output ordinal) copy entry.
 * The framework applies the map so that fields the command neither creates,
 * modifies nor drops are carried with no per-field code (the fast path for
 * tckconvert / tcktransform). A command that does not change the field set
 * constructs the Shared object with output == input, yielding an identity map.
 *
 * For Stage 10 a single registry (input) is typical; the output registry may
 * alias it (the pass-through-only / lossless-copy case used by tckconvert). The
 * separate-registry design is in place from the outset so the field-set
 * evolution of later stages (create/modify/drop) needs no interface change. */
class Shared {
public:
  //! \brief one (input ordinal → output ordinal) pass-through copy for a role.
  struct PassThrough {
    size_t input_ordinal;
    size_t output_ordinal;
  };

  //! \brief construct from distinct input and output registries (§2.7).
  /*! References are retained; the caller must keep both registries alive for the
   * lifetime of the Shared object (they typically live in the owning input /
   * output Tractogram). */
  Shared(const FieldRegistry &input, const FieldRegistry &output) : input_(input), output_(output) {
    build_passthrough();
  }

  //! \brief construct a pass-through-only context (output identical to input).
  /*! The common Stage 10 case: every field is carried unchanged. The output
   * registry aliases the input registry, so the pass-through map is the identity
   * over both ordinal spaces. */
  explicit Shared(const FieldRegistry &registry) : input_(registry), output_(registry) { build_passthrough(); }

  const FieldRegistry &input() const { return input_; }
  const FieldRegistry &output() const { return output_; }

  //! \brief the dps fields carried unchanged from input to output (§2.7).
  const std::vector<PassThrough> &dps_passthrough() const { return dps_passthrough_; }
  //! \brief the dpv fields carried unchanged from input to output (§2.7).
  const std::vector<PassThrough> &dpv_passthrough() const { return dpv_passthrough_; }

  //! \brief copy every pass-through dps/dpv field from \a in to \a out (§2.7).
  /*! Resizes \a out's payload vectors to the output registry and copies each
   * pass-through field's variant value verbatim, preserving its native dtype
   * (D7). Fields the worker creates/modifies are written by the worker at their
   * output ordinals after this call; dropped fields are simply absent from the
   * output registry. The empty-registry common case is a no-op fast path. */
  template <class ValueType>
  void carry_passthrough(const TractogramItem<ValueType> &in, TractogramItem<ValueType> &out) const {
    out.dps.resize(output_.dps_count());
    out.dpv.resize(output_.dpv_count());
    for (const auto &p : dps_passthrough_)
      out.dps[p.output_ordinal] = in.dps[p.input_ordinal];
    for (const auto &p : dpv_passthrough_)
      out.dpv[p.output_ordinal] = in.dpv[p.input_ordinal];
  }

private:
  const FieldRegistry &input_;
  const FieldRegistry &output_;
  std::vector<PassThrough> dps_passthrough_;
  std::vector<PassThrough> dpv_passthrough_;

  //! \brief two fields match for pass-through iff role, dtype and M coincide.
  static bool compatible(const FieldDescriptor &a, const FieldDescriptor &b) {
    return a.role == b.role && a.dtype == b.dtype && a.columns == b.columns;
  }

  void build_passthrough() {
    for (const auto &in_field : input_) {
      const FieldDescriptor *out_field = output_.find(in_field.name, in_field.role);
      if (out_field == nullptr || !compatible(in_field, *out_field))
        continue;
      const PassThrough entry{in_field.ordinal, out_field->ordinal};
      if (in_field.role == FieldRole::DPV)
        dpv_passthrough_.push_back(entry);
      else if (in_field.role == FieldRole::DPS)
        dps_passthrough_.push_back(entry);
    }
  }
};

} // namespace MR::DWI::Tractography
