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

#include <filesystem>

#include "algo/threaded_loop.h"
#include "command.h"
#include "enum.h"
#include "fixel/helpers.h"
#include "header.h"

#include "fixel/correspondence/algorithms/all2all.h"
#include "fixel/correspondence/algorithms/base.h"
#include "fixel/correspondence/algorithms/ismrm2018.h"
#include "fixel/correspondence/algorithms/legacy.h"
#include "fixel/correspondence/algorithms/pot.h"
#include "fixel/correspondence/algorithms/rs2023.h"
#include "fixel/correspondence/correspondence.h"
#include "fixel/correspondence/matcher.h"

using namespace MR;
using namespace App;
using namespace MR::Fixel::Correspondence;

// Old TODOs
// How to deal with computational intractability of > 5 fixels?
// - A new algorithm could do a comprehensive check for all origin fixel combinations for the first target
//     fixel, then repeat with whatever is left for the next largest fixel, and so on. May however compromise the
//     matching quality of major target fixels for the sake of keeping computational tractability of many
//     very small fixels...
// - Further restrictions on permissible sets of origin source fixel sets.
//   E.g. If two source fixels are 90 degrees to one another, don't even consider any candidate
//     remapping that includes both as origins.
//   Also, if two target fixels are 90 degrees to one another, then don't consider any candidate
//     remapping that involves one source fixel contributing to both.
//   (Advantage is that this can reduce the combinatorial explosion)
//
//   Potentially one way to try to get at this would be to determine the convex sets of the content of
//     each voxel (i.e. based on fixel directions), and only permit groupings where there are no
//     disconnected fixels
//   The calculation of the convex set will not be able to initialise with fewer than 4 (?) directions
//   Ideally, for voxels with fewer fixels than this, initialise some structure that bypasses this
//     initialisation, and quicky returns that it is permissible for any fixels to be treated in a group
//   This condition should ideally be applied to both source and target voxels
//
//
//
// For future enhancements, if sparse re-parameterisation of FODs is implemented, and hence orientation
//   dispersion information is available, this could be utilised within the correspondence cost function
//
//
//
// Provide ability to export remapped source fixels
// Would this actually belong better in fixel2fixel command?
// No, I don't think so; that explicitly maps one fixel data file to another, whereas this requires new fixels
//
//
//
// Currently, when generating remapped source fixels, the objective target fixel is
//   used for determining antipodal orientation;
//   could this be bypassed to make the generation of remapped source fixels entirely
//   independent of the objective target fixels?
//
//
// Consider instead of Mapping class a pair of classes where
//   one uses a data representation that clearly comes straight from disk and is read-only,
//   and one is intended to be dynamically resizable,
//   but they operate using the same interface (perhaps using CRTP)
// In this way the interface for fixelcorrespondence and fixel2fixel could look identical,
//   even though the underlying data structures would be different,
//   and the latter would not need to do an explicit load into RAM

enum class algorithm_t {
#ifdef FIXELCORRESPONDENCE_INCLUDE_ALL2ALL
  ALL2ALL,
#endif
  LEGACY,
  ISMRM2018,
  POT,
  RS2023
};
constexpr algorithm_t default_algorithm = algorithm_t::POT;

// clang-format off
void usage() {

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Establish correpondence between two fixel datasets";

  DESCRIPTION
  + "It is assumed that the source image has already been spatially normalised and is defined on the same voxel grid as the target. "
    "One would typically also want to have performed a reorientation of fibre information to reflect this spatial normalisation "
    "prior to invoking this command, as this would be expected to improve fibre orientation correspondence across datasets."

  + "The output of the command is a .npz file (uncompressed ZIP archive)"
    " encoding how data from source fixels should be remapped "
    " in order to express those data in target fixel space."
    " This information would typically then be utilised by command fixel2fixel "
    " to project some quantitative parameter from the source fixel dataset to the target fixels."

  + "Multiple algorithms are provided; a brief description of each of these is provided below."

#ifdef FIXELCORRESPONDENCE_INCLUDE_ALL2ALL
  + "\"all2all\": This algorithm is defined for debugging / demonstrative purposes only. "
    "It assigns all source fixels to all target fixels, and is therefore not appropriate for practical use."
#endif

  + "\"legacy\": This algorithm duplicates the behaviour of the fixelcorrespondence command in MRtrix versions 3.0.x. and earlier. "
    "It determines, for every target fixel, the nearest source fixel, and assigns that source fixel to the target fixel "
    "with a weight of 1.0, as long as the angle between them is less than some threshold. "
    "Note that if multiple target fixels select the same source fixel, "
    "the entirety of the data from that source fixel is projected to each of those target fixels independently."

  + "\"ismrm2018\": This is a combinatorial algorithm, for which the algorithm and cost function are described in the "
    "relevant reference (Smith et al., 2018)."

  + "\"in2023\": This is a combinatorial algorithm, for which the combinatorial algorithm utilised is described in reference "
    "(Smith et al., 2018), but an alternative cost function is proposed (publication pending)."

  + "\"pot\": This is a combinatorial algorithm using a partial-optimal-transport-inspired cost function. "
    "Matched fibre density between subject and template fixels is \"transported\" at a cost determined by directional misalignment, "
    "while surplus density on either side is created or destroyed at unit cost; "
    "subject or template fixels with no correspondence are penalised by their density. "
    "Mapping topology (multiple subject fixels merged into one template fixel, "
    "or one subject fixel split across multiple template fixels) is penalised linearly "
    "with weight controlled by the \"gamma\" parameter, "
    "and the angular sensitivity is controlled by exponent \"p\".";

  ARGUMENTS
  + Argument ("source_density", "the input source fixel data file corresponding to the FD or FDC metric").type_image_in()
  + Argument ("target_density", "the input target fixel data file corresponding to the FD or FDC metric").type_image_in()
  + Argument ("output", "the name of the output .npz file encoding the fixel correspondence").type_file_out();

  OPTIONS
  + Option ("algorithm", "the algorithm to use when establishing fixel correspondence; "
                         "options are: " + Enum::join<algorithm_t>() + " (default: " + Enum::lowercase_name<algorithm_t>(default_algorithm) + ")")
    + Argument ("choice").type_choice<algorithm_t>()

  + Option ("remapped", "export the remapped source fixels to a new fixel directory")
    + Argument ("path").type_directory_out()

  + Algorithms::LegacyOptions

  + Algorithms::CombinatorialOptions

  + Algorithms::POTOptions

  + Algorithms::RS2023Options;

  REFERENCES
  + "* If using -algorithm ismrm2018 or -algorithm rs2023: " // Internal
    "Smith, R.E.; Connelly, A. "
    "Mitigating the effects of imperfect fixel correspondence in Fixel-Based Analysis. "
    "In Proc ISMRM 2018: 456.";
}
// clang-format on

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
  const algorithm_t algorithm_choice = get_option_choice<algorithm_t>("algorithm", default_algorithm);
  std::shared_ptr<Algorithms::Base> algorithm;
  switch (algorithm_choice) {
#ifdef FIXELCORRESPONDENCE_INCLUDE_ALL2ALL
  case algorithm_t::ALL2ALL:
    algorithm.reset(new Algorithms::All2All());
    break;
#endif
  case algorithm_t::LEGACY:
    algorithm.reset(new Algorithms::Legacy(get_option_value("angle", default_nearest_maxangle)));
    break;
  case algorithm_t::ISMRM2018:
    algorithm.reset(new Algorithms::ISMRM2018(get_option_value("max_origins", default_max_origins_per_target),
                                              get_option_value("max_objectives", default_max_objectives_per_source),
                                              H_cost));
    break;
  case algorithm_t::POT:
    algorithm.reset(new Algorithms::POT(get_option_value("max_origins", default_max_origins_per_target),
                                        get_option_value("max_objectives", default_max_objectives_per_source),
                                        H_cost));
    // dynamic_cast<Algorithms::POT *>(algorithm.get())
    //     ->set_constants(get_option_value("pot_steepness", default_pot_p),
    //                     get_option_value("pot_complexity", default_pot_gamma));
    dynamic_cast<Algorithms::POT *>(algorithm.get())->set_gamma(get_option_value("pot_complexity", default_pot_gamma));
    break;
  case algorithm_t::RS2023:
    algorithm.reset(new Algorithms::RS2023(get_option_value("max_origins", default_max_origins_per_target),
                                           get_option_value("max_objectives", default_max_objectives_per_source),
                                           H_cost));
    {
      auto opt = get_options("rs2023_constants");
      if (opt.size())
        dynamic_cast<Algorithms::RS2023 *>(algorithm.get())->set_constants(opt[0][0], opt[0][1]);
    }
    break;
  default:
    assert(0);
  }

  Matcher matcher(input_filepath, argument[1], algorithm);

  auto image(matcher.get_template());
  ThreadedLoop("determining fixel correspondence", image, 0, 3).run(matcher, image);

  matcher.get_mapping().save(argument[2]);

  auto opt = get_options("cost");
  if (opt.size())
    algorithm->export_cost_image(opt[0][0]);
  opt = get_options("remapped");
  if (opt.size())
    matcher.export_remapped(opt[0][0]);
}
