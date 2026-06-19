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

// -----------------------------------------------------------------------------
// TEMPORARY cost-function evaluation command, living alongside fixelcorrespondence.
//
// Purpose: evaluate and tune many combinatorial fixel-correspondence cost functions
//   against a fixed dataset without paying the (dominant) cost of regenerating the
//   candidate re-mappings once per cost function. The complete set of candidate
//   re-mappings is generated exactly once per voxel (reusing the existing
//   combinatorial machinery); every cost function and parameter set listed in
//   MegaCost::configs() is evaluated against those candidates in a single pass.
//
// Inputs are, as for fixelcorrespondence, a subject (source) fibre density fixel
//   data file and a template (target) fibre density fixel data file. The output is
//   a directory, populated with one .npz fixel-fixel mapping per configuration plus
//   a parameters.tsv manifest describing each.
//
// This command (and the supporting MegaCost class) are intended to be removed once
//   the cost-function design has been settled.
// -----------------------------------------------------------------------------

#include <filesystem>
#include <fstream>

#include "algo/threaded_loop.h"
#include "command.h"
#include "fixel/helpers.h"
#include "header.h"
#include "progressbar.h"
#include "types.h"

#include "fixel/correspondence/algorithms/base.h"
#include "fixel/correspondence/algorithms/megacost.h"
#include "fixel/correspondence/correspondence.h"
#include "fixel/correspondence/fixel.h"
#include "fixel/correspondence/mapping.h"
#include "fixel/correspondence/matcher.h"

using namespace MR;
using namespace App;
using namespace MR::Fixel::Correspondence;

// clang-format off
void usage() {

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Evaluate many combinatorial fixel correspondence cost functions in a single pass";

  DESCRIPTION
  + "This is a temporary development command for evaluating the efficacy of different combinatorial "
    "fixel correspondence cost functions, and for optimising the internal parameters of those cost functions, "
    "against a fixed test dataset."

  + "For the combinatorial algorithms, the dominant computational expense is generation of the complete set of "
    "candidate fixel re-mappings within each voxel; the cost functions themselves are comparatively trivial. "
    "Rather than executing the fixelcorrespondence command once per cost function (and once per internal parameter "
    "set of such), this command generates the candidate re-mappings exactly once per voxel and evaluates every "
    "cost function and parameter set against those candidates simultaneously."

  + "The cost functions and parameter sets that are evaluated are hard-coded; refer to the source code "
    "(MegaCost::configs()) for the complete list. The output is a directory containing one .npz file per "
    "configuration (in the same format produced by the fixelcorrespondence command), named according to the "
    "cost function and its parameter values, together with a \"parameters.tsv\" manifest.";

  ARGUMENTS
  + Argument ("source_density", "the input source fixel data file corresponding to the FD or FDC metric").type_image_in()
  + Argument ("target_density", "the input target fixel data file corresponding to the FD or FDC metric").type_image_in()
  + Argument ("output", "the output directory into which one .npz file per configuration will be written").type_directory_out();

  OPTIONS
  + Algorithms::CombinatorialOptions;
}
// clang-format on

namespace {

using CFixel = MR::Fixel::Correspondence::Fixel;

// Per-thread, per-voxel driver: load fixels, run the single-pass multi-configuration
//   evaluation, and commit one mapping per configuration. Copy-constructed per thread
//   (the Matcher provides independent image cursors); the configuration outputs are
//   shared and written at disjoint target-fixel indices, so no locking is required.
class Sweeper {
public:
  Sweeper(const Matcher &matcher, std::shared_ptr<Algorithms::MegaCost> mega, std::vector<Mapping> &results)
      : matcher(matcher), mega(std::move(mega)), results(&results) {}

  void operator()(Image<index_type> &pos) {
    std::vector<CFixel> source_fixels, target_fixels;
    index_type offset_source, offset_target;
    matcher.load_voxel(pos, source_fixels, target_fixels, offset_source, offset_target);
    if (target_fixels.empty())
      return;

    const voxel_t v{
        static_cast<uint32_t>(pos.index(0)), static_cast<uint32_t>(pos.index(1)), static_cast<uint32_t>(pos.index(2))};
    const std::vector<std::vector<std::vector<index_type>>> &best_inv =
        mega->evaluate_all(v, source_fixels, target_fixels);

    const index_type nfixels_target = target_fixels.size();
    for (size_t k = 0; k != best_inv.size(); ++k) {
      const std::vector<std::vector<index_type>> &inv = best_inv[k];
      // Reconstruct the forward mapping (entries per target fixel) from the inverse mapping
      std::vector<std::vector<Mapping::Entry>> forward(nfixels_target);
      for (index_type is = 0; is != source_fixels.size(); ++is) {
        if (inv[is].empty())
          continue;
        const float weight = 1.0f / static_cast<float>(inv[is].size());
        for (const index_type it : inv[is])
          forward[it].push_back({offset_source + is, weight});
      }
      for (index_type it = 0; it != nfixels_target; ++it)
        (*results)[k][offset_target + it] = forward[it];
    }
  }

private:
  Matcher matcher;
  std::shared_ptr<Algorithms::MegaCost> mega;
  std::vector<Mapping> *results;
};

std::string family_name(const Algorithms::CostConfig::Family family) {
  using Family = Algorithms::CostConfig::Family;
  switch (family) {
  case Family::ISMRM2018:
    return "ismrm2018";
  case Family::POT:
    return "pot";
  case Family::RS2023:
    return "rs2023";
  case Family::TRANSPORT:
    return "transport";
  case Family::TRANSPORTDISP:
    return "transportdisp";
  case Family::AGREEMENT:
    return "agreement";
  case Family::TRANSPORTGUARD:
    return "transportguard";
  }
  return "unknown";
}

void write_manifest(const std::filesystem::path &path, const std::vector<Algorithms::CostConfig> &configs) {
  std::ofstream out(path);
  if (!out)
    throw Exception("Unable to create manifest file \"" + path.string() + "\"");
  out << "filename\tfamily\tkernel\tgamma\talpha\tbeta\tsigma\tlambda\tmu\trho\tangle\n";
  for (const auto &cfg : configs) {
    out << cfg.name << ".npz\t" << family_name(cfg.family) << "\t"
        << (cfg.kernel == Algorithms::AngularKernel::TAN2 ? "tan2" : "tan") << "\t" //
        << cfg.gamma << "\t" << cfg.alpha << "\t" << cfg.beta << "\t" << cfg.sigma << "\t" << cfg.lambda << "\t"
        << cfg.mu << "\t" << cfg.rho << "\t" << cfg.angle << "\n";
  }
}

} // namespace

void run() {
  const std::filesystem::path input_filepath(argument[0]);
  if (std::filesystem::is_directory(input_filepath))
    throw Exception("Input the specific fixel data file to participate in matching,"
                    " not the fixel directory");
  const std::filesystem::path input_fixel_directory = Fixel::get_fixel_directory(input_filepath);
  auto input_index_header = Fixel::find_index_header(input_fixel_directory);

  Header H_cost(input_index_header);
  H_cost.ndim() = 3;
  H_cost.datatype() = DataType::Float32;
  H_cost.datatype().set_byte_order_native();

  const index_type max_origins = get_option_value("max_origins", default_max_origins_per_target);
  const index_type max_objectives = get_option_value("max_objectives", default_max_objectives_per_source);

  const std::filesystem::path output_directory(argument[2]);
  std::filesystem::create_directories(output_directory);

  auto mega = std::make_shared<Algorithms::MegaCost>(max_origins, max_objectives, H_cost);
  const std::vector<Algorithms::CostConfig> &configs = Algorithms::MegaCost::configs();

  std::shared_ptr<Algorithms::Base> algorithm = mega;
  Matcher matcher(input_filepath, argument[1], algorithm);

  INFO("Evaluating " + str(configs.size()) + " cost-function configurations in a single pass");

  // One output mapping per configuration, pre-sized to the total fixel counts so that
  //   per-voxel writes (at disjoint target-fixel indices) are race-free.
  std::vector<Mapping> results(configs.size(),
                               Mapping(static_cast<index_type>(matcher.num_source_fixels()),
                                       static_cast<index_type>(matcher.num_target_fixels())));

  Sweeper sweeper(matcher, mega, results);
  auto image(matcher.get_template());
  ThreadedLoop("evaluating fixel correspondence cost functions", image, 0, 3).run(sweeper, image);

  ProgressBar progress("writing per-configuration fixel correspondence files", configs.size());
  for (size_t k = 0; k != configs.size(); ++k) {
    results[k].save(output_directory / (configs[k].name + ".npz"));
    ++progress;
  }

  write_manifest(output_directory / "parameters.tsv", configs);
}
