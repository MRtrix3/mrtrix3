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
#include <filesystem>
#include <string>

#include "cmdline_option.h"
#include "dwi/tractography/field_registry.h"

namespace MR::DWI::Tractography {

class Properties;
template <class ValueType> class Tractogram;

extern const App::Option TrackWeightsInOption;
extern const App::Option TrackWeightsOutOption;

//! \brief Where the streamline weights a command loaded came from.
/*! Streamline weights are always specified explicitly at the command line, in one
 * of two forms — a standalone scalar file, or a named field within the input
 * tractogram — and are routed into the privileged Streamline::weight. The
 * provenance determines the default OUTPUT behaviour: weights drawn from a named
 * field of the input tractogram propagate to the output by default, whereas
 * weights supplied as an external file are not re-written unless requested. */
enum class WeightProvenance {
  None,         //!< no "-tck_weights_in": every weight is the default 1.0
  ExternalFile, //!< loaded from a standalone scalar file ("-tck_weights_in <file>")
  InternalField //!< loaded from a named field of the input tractogram ("<dataset>::<field>")
};

//! \brief Provenance + default output name of a command's streamline weights.
struct WeightInput {
  WeightProvenance provenance = WeightProvenance::None;
  //! \brief the default output field-name / file-basename for the weights.
  /*! The input field name (internal provenance) or the input file basename
   * (external provenance); empty when no weights were loaded. */
  std::string default_name;
};

//! \brief Read "-tck_weights_in" and wire weight loading onto \a input.
/*! Implements the two explicit input routes: a bare path is a standalone scalar
 * file; a qualified "<dataset>::<field>" names a per-streamline field of the input
 * tractogram itself (\a input_path). Either route populates the privileged
 * Streamline::weight. A bare path that happens to be a tractogram, a cross-dataset
 * qualified reference, or a missing / multi-column field is rejected. Returns the
 * provenance, consumed by plan_weight_output() for the default policy. */
WeightInput register_weight_input(Tractogram<float> &input, const std::filesystem::path &input_path);

//! \brief A resolved "-tck_weights_out" plan plus the registry to create with.
struct WeightOutput {
  //! the output field registry: the input fields, plus an appended weight field
  //!   when the weights are to be embedded under a name not already present
  FieldRegistry registry;
  enum class Kind {
    None,     //!< no weights are written
    External, //!< write a standalone scalar file
    Embedded  //!< re-emit Streamline::weight into an output per-streamline field
  } kind = Kind::None;
  std::filesystem::path external_path; //!< Kind::External: the scalar file to write
  size_t embed_ordinal = 0;            //!< Kind::Embedded: the output dps ordinal to inject into
  size_t initial_streamlines = 0;      //!< pre-size hint for the external-file accumulator
};

//! \brief Resolve "-tck_weights_out" and the provenance default policy into a plan.
/*! Called BEFORE Tractogram::create(): it returns the field registry to create the
 * output with (so an embedded weight field is declared up front) together with the
 * resolved destination. \a input_fields is the input registry; \a input_provenance
 * is the result of register_weight_input(); \a output_path is the command's output
 * tractogram; \a properties supplies the streamline count for pre-sizing. The
 * unsupported combinations — renaming an embedded weight field, or writing an
 * internal weight to an external file while the output format also embeds it — are
 * rejected with an explanatory error. */
WeightOutput plan_weight_output(const FieldRegistry &input_fields,
                                const WeightInput &input_provenance,
                                const std::filesystem::path &output_path,
                                const Properties &properties);

//! \brief Wire a resolved WeightOutput plan onto the created \a output Tractogram.
void apply_weight_output(Tractogram<float> &output, const WeightOutput &plan);

} // namespace MR::DWI::Tractography
