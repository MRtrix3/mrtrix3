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

#include "dwi/tractography/weights.h"

#include <filesystem>
#include <optional>
#include <string>
#include <system_error>

#include "app.h"
#include "datatype.h"
#include "exception.h"
#include "mrtrix.h"

#include "dwi/tractography/formats/base.h"
#include "dwi/tractography/formats/list.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/sidecar.h"
#include "dwi/tractography/tractogram.h"

namespace MR::DWI::Tractography {

using namespace App;

// clang-format off
const Option TrackWeightsInOption =
    Option("tck_weights_in",
           "specify the streamline weights:"
           " either a standalone scalar file,"
           " or \"[<tractogram>]::<field>\" naming a per-streamline field of the input tractogram"
           " (an empty <tractogram>, i.e. \"::<field>\", refers to the command's own input tractogram,"
           " which is the only way to name a field of a piped input)")
      + Argument("spec").type_tractogram_data_in();

const Option TrackWeightsOutOption =
    Option("tck_weights_out",
           "specify where to write the output streamline weights:"
           " either a standalone scalar file,"
           " or \"[<tractogram>]::<field>\" naming a per-streamline field of the output tractogram"
           " (an empty <tractogram>, i.e. \"::<field>\", refers to the command's own output tractogram,"
           " which is the only way to name a field of a piped output)")
      + Argument("spec").type_tractogram_data_out();
// clang-format on

namespace {

//! \brief whether two (possibly not-yet-existent) paths denote the same file.
bool same_path(const std::filesystem::path &a, const std::filesystem::path &b) {
  std::error_code ec;
  const std::filesystem::path ca = std::filesystem::weakly_canonical(a, ec);
  if (ec)
    return a.lexically_normal() == b.lexically_normal();
  const std::filesystem::path cb = std::filesystem::weakly_canonical(b, ec);
  if (ec)
    return a.lexically_normal() == b.lexically_normal();
  return ca == cb;
}

//! \brief whether the format for \a path can carry per-streamline (dps) data.
bool format_embeds_dps(const std::filesystem::path &path) {
  const Formats::Base *handler = Formats::get_handler(path);
  return handler != nullptr && handler->capabilities.sidecar_data != Formats::SidecarData::Unsupported;
}

//! \brief the dps ordinal of a Float32 weight field named \a name in \a registry,
//!   adding it (as an internally-carried field) when not already present.
size_t ensure_weight_field(FieldRegistry &registry, const std::string &name) {
  const std::optional<size_t> existing = registry.ordinal(name, FieldRole::DPS);
  if (existing.has_value())
    return *existing;
  return registry.add({name, FieldRole::DPS, DataType::Float32, 1, FieldSource::Internal, 0});
}

//! \brief a default standalone weights path beside \a output_path, basenamed \a name.
std::filesystem::path sibling_weights_path(const std::filesystem::path &output_path, const std::string &name) {
  const std::filesystem::path dir = output_path.parent_path();
  return (dir.empty() ? std::filesystem::path(".") : dir) / (output_path.stem().string() + "_" + name + ".txt");
}

//! \brief the streamline count recorded in \a properties, or 0 if absent.
size_t streamline_count(const Properties &properties) {
  const auto it = properties.find("count");
  return it == properties.end() ? size_t(0) : to<size_t>(it->second);
}

} // namespace

WeightInput register_weight_input(Tractogram<float> &input, const std::filesystem::path &input_path) {
  WeightInput result;
  auto opt = App::get_options("tck_weights_in");
  if (opt.empty())
    return result;
  const SidecarReference reference = parse_sidecar_reference(opt[0][0].as_text());
  if (reference.is_qualified()) {
    // An empty DATASET ("::<field>") is the self-reference to the command's own
    //   input tractogram — the only way to name a field of a piped input, whose
    //   path is the non-filesystem pipe sentinel "-".
    const bool self_reference = reference.dataset.empty() || same_path(reference.dataset, input_path);
    if (!self_reference)
      throw Exception("streamline-weight import \"" + reference.dataset.string() + "::" + *reference.name + "\"" +
                      " must name a field of the input tractogram \"" + input_path.string() + "\" itself" +
                      " (a cross-dataset weight import is not supported;" +
                      " extract the field to a standalone scalar file first)");
    const FieldDescriptor *descriptor = input.fields().find(*reference.name, FieldRole::DPS);
    if (descriptor == nullptr)
      throw Exception("input tractogram \"" + input_path.string() + "\"" + " carries no per-streamline field named \"" +
                      *reference.name + "\" to use as the streamline weights");
    if (descriptor->columns != 1)
      throw Exception("per-streamline field \"" + *reference.name + "\" has " + str(descriptor->columns) +
                      " columns; only a single-column (scalar) field can be used as the streamline weights");
    input.register_weight_input_internal(descriptor->ordinal);
    result.provenance = WeightProvenance::InternalField;
    result.default_name = *reference.name;
  } else {
    if (format_embeds_dps(reference.dataset)) {
      throw Exception("streamline-weight input \"" + reference.dataset.string() + "\"" +
                      " is itself a tractogram; name the field explicitly as \"" + reference.dataset.string() +
                      "::<field>\" to use one of its per-streamline fields as the weights");
    }
    input.register_weight_input_external(reference.dataset);
    result.provenance = WeightProvenance::ExternalFile;
    result.default_name = reference.dataset.stem().string();
  }
  return result;
}

WeightOutput plan_weight_output(const FieldRegistry &input_fields,
                                const WeightInput &input_provenance,
                                const std::filesystem::path &output_path,
                                const Properties &properties) {
  WeightOutput result;
  result.registry = input_fields;
  result.initial_streamlines = streamline_count(properties);
  const bool embeds = format_embeds_dps(output_path);

  auto opt = App::get_options("tck_weights_out");
  if (!opt.empty()) {
    const SidecarReference reference = parse_sidecar_reference(opt[0][0].as_text());
    if (reference.is_qualified()) {
      // An empty DATASET ("::<field>") is the self-reference to the command's own
      //   output tractogram — the only way to name a field of a piped output.
      const bool self_reference = reference.dataset.empty() || same_path(reference.dataset, output_path);
      if (!self_reference)
        throw Exception("streamline-weight field \"" + reference.dataset.string() + "::" + *reference.name + "\"" +
                        " must name the output tractogram \"" + output_path.string() + "\" itself");
      if (!embeds)
        throw Exception(std::string("output tractography format cannot carry per-streamline data,") +
                        " so the streamline weights cannot be embedded as field \"" + *reference.name +
                        "\" (write a standalone weights file instead)");
      if (input_provenance.provenance == WeightProvenance::InternalField &&
          *reference.name != input_provenance.default_name)
        throw Exception("the embedded streamline-weights field must keep the input field name \"" +
                        input_provenance.default_name +
                        "\"; renaming an embedded weight field on output is not supported");
      result.kind = WeightOutput::Kind::Embedded;
      result.embed_ordinal = ensure_weight_field(result.registry, *reference.name);
    } else {
      if (input_provenance.provenance == WeightProvenance::InternalField && embeds)
        throw Exception(std::string("writing the internal streamline weights to a standalone file") +
                        " (\"-tck_weights_out " + reference.dataset.string() + "\")" +
                        " would also embed them in the output tractogram;" +
                        " omit -tck_weights_out to embed them, or select a vertices-only output format");
      result.kind = WeightOutput::Kind::External;
      result.external_path = reference.dataset;
    }
    return result;
  }

  // No "-tck_weights_out": apply the provenance default policy.
  switch (input_provenance.provenance) {
  case WeightProvenance::InternalField:
    if (embeds) {
      // Propagate: re-emit the weight under the input field name.
      result.kind = WeightOutput::Kind::Embedded;
      result.embed_ordinal = ensure_weight_field(result.registry, input_provenance.default_name);
    } else {
      // Output cannot embed: propagate to a standalone sibling file.
      result.kind = WeightOutput::Kind::External;
      result.external_path = sibling_weights_path(output_path, input_provenance.default_name);
      WARN("output tractography format cannot carry per-streamline data; streamline weights written to"
           " standalone file \"" +
           result.external_path.string() + "\"");
    }
    break;
  case WeightProvenance::ExternalFile:
    INFO("input streamline weights were supplied as a standalone file;"
         " they are not written to the output unless -tck_weights_out is given");
    break;
  case WeightProvenance::None:
    break;
  }
  return result;
}

void apply_weight_output(Tractogram<float> &output, const WeightOutput &plan) {
  switch (plan.kind) {
  case WeightOutput::Kind::None:
    break;
  case WeightOutput::Kind::External:
    output.register_weight_output_external(plan.external_path, plan.initial_streamlines);
    break;
  case WeightOutput::Kind::Embedded:
    output.register_weight_output_embedded(plan.embed_ordinal);
    break;
  }
}

} // namespace MR::DWI::Tractography
