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

#include <set>

#include "command.h"
#include "image.h"
#include "image_helpers.h"
#include "mutexprotected.h"
#include "progressbar.h"

#include "fixel/fixel.h"
#include "fixel/helpers.h"

#include "dwi/tractography/mapping/loader.h"
#include "dwi/tractography/mapping/mapper.h"
#include "dwi/tractography/mapping/mapping.h"
#include "dwi/tractography/mapping/writer.h"
#include "dwi/tractography/weights.h"

using namespace MR;
using namespace App;

using Fixel::index_type;

template <class ValueType> class TrackProcessor {

public:
  using SetVoxelDir = DWI::Tractography::Mapping::SetVoxelDir;
  using VectorType = Eigen::Array<ValueType, Eigen::Dynamic, 1>;

  TrackProcessor(Image<index_type> &fixel_indexer,
                 Image<float> &fixel_directions,
                 const size_t upsample_ratio,
                 const bool precise,
                 const float angular_threshold,
                 std::shared_ptr<MutexProtected<VectorType>> fixel_TDI)
      : mapper(fixel_indexer),
        fixel_indexer(fixel_indexer),
        fixel_directions(fixel_directions),
        precise(precise),
        angular_threshold_dp(std::cos(angular_threshold * (Math::pi / 180.0))),
        global_fixel_TDI(fixel_TDI),
        local_fixel_TDI(VectorType::Zero(fixel_TDI->lock()->size())) {
    mapper.set_upsample_ratio(upsample_ratio);
    mapper.set_use_precise_mapping(true);
  }

  TrackProcessor(const TrackProcessor &that)
      : mapper(that.mapper),
        fixel_indexer(that.fixel_indexer),
        fixel_directions(that.fixel_directions),
        precise(that.precise),
        angular_threshold_dp(that.angular_threshold_dp),
        global_fixel_TDI(that.global_fixel_TDI),
        local_fixel_TDI(VectorType::Zero(that.local_fixel_TDI.size())) {}

  ~TrackProcessor() {
    auto acquired_global_fixel_TDI = global_fixel_TDI.lock()->lock();
    *acquired_global_fixel_TDI += local_fixel_TDI;
  }

  bool operator()(const DWI::Tractography::Streamline<> &in) {
    SetVoxelDir visitations;
    mapper(in, visitations);
    // For each voxel tract tangent, assign to a fixel
    // For each fixel, sum the intersection lengths
    std::map<index_type, float> fixels;
    for (SetVoxelDir::const_iterator i = visitations.begin(); i != visitations.end(); ++i) {
      assign_pos_of(*i).to(fixel_indexer);
      fixel_indexer.index(3) = 0;
      index_type num_fibres = fixel_indexer.value();
      if (num_fibres > 0) {
        fixel_indexer.index(3) = 1;
        index_type first_index = fixel_indexer.value();
        index_type last_index = first_index + num_fibres;
        index_type closest_fixel_index = 0;
        float largest_dp = 0.0F;
        for (index_type j = first_index; j < last_index; ++j) {
          fixel_directions.index(0) = j;
          const float dp = std::fabs(i->get_dir().dot(Eigen::Vector3f(fixel_directions.row(1))));
          if (dp > largest_dp) {
            largest_dp = dp;
            closest_fixel_index = j;
          }
        }
        if (largest_dp > angular_threshold_dp) {
          auto existing = fixels.find(closest_fixel_index);
          if (existing == fixels.end())
            fixels.insert({closest_fixel_index, i->get_length()});
          else
            existing->second += static_cast<float>(i->get_length());
        }
      }
    }
    for (auto f : fixels) {
      if constexpr (std::is_integral_v<ValueType>)
        ++local_fixel_TDI[f.first];
      else
        local_fixel_TDI[f.first] += static_cast<ValueType>(in.weight) * (precise ? f.second : 1.0F);
    }
    return true;
  }

private:
  DWI::Tractography::Mapping::TrackMapperBase mapper;
  Image<index_type> fixel_indexer;
  Image<float> fixel_directions;
  const bool precise;
  const float angular_threshold_dp;
  std::weak_ptr<MutexProtected<VectorType>> global_fixel_TDI;
  VectorType local_fixel_TDI;
};

// clang-format off
void usage() {

  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)"
           " and Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Compute a fixel TDI map from a tractogram";

  ARGUMENTS
  + Argument ("tracks",  "the input tracks.").type_tracks_in()
  + Argument ("fixel_folder_in", "the input fixel folder;"
                                 " used to define the fixels and their directions").type_directory_in()
  + Argument ("fixel_folder_out", "the fixel folder to which the output will be written;"
                                  " this can be the same as the input folder if desired").type_text()
  + Argument ("fixel_data_out", "the name of the fixel data image.").type_text();

  OPTIONS
  + Option ("angle", "the max angle threshold for assigning streamline tangents to fixels"
                     " (default: " + str(DWI::Tractography::Mapping::default_streamline2fixel_angle, 2) + " degrees)")
    + Argument ("value").type_float(0.0, 90.0)

  + Option ("precise", "utilise the precise length of streamline-voxel intersections"
                       " rather than simply the number of streamlines / sum of streamline weights")

  + DWI::Tractography::TrackWeightsInOption;
}
// clang-format on

template <class ValueType>
void run(DWI::Tractography::Mapping::TrackLoader &loader,
         Image<index_type> &index_image,
         Image<float> &directions_image,
         const size_t num_fixels,
         const size_t upsample_ratio,
         const bool precise,
         const float angular_threshold,
         std::string_view output_path) {
  if constexpr (std::is_integral_v<ValueType>) {
    assert(!precise);
  }
  auto fixel_TDI = std::make_shared<MutexProtected<Eigen::Array<ValueType, Eigen::Dynamic, 1>>>(
      Eigen::Array<ValueType, Eigen::Dynamic, 1>::Zero(num_fixels));
  {
    TrackProcessor<ValueType> processor(
        index_image, directions_image, upsample_ratio, precise, angular_threshold, fixel_TDI);
    Thread::run_queue(loader, Thread::batch(DWI::Tractography::Streamline<float>()), Thread::multi(processor));
  }
  {
    auto data = *fixel_TDI->lock();
    Header H = Fixel::data_header_from_nfixels(num_fixels);
    H.datatype() = DataType::from<ValueType>();
    auto output = Image<ValueType>::create(output_path, H);
    for (size_t i = 0; i < data.size(); ++i) {
      output.index(0) = i;
      output.value() = data[i];
    }
  }
}

void run() {
  const std::string input_fixel_folder = argument[1];
  Header index_header = Fixel::find_index_header(input_fixel_folder);
  auto index_image = index_header.get_image<index_type>();
  auto directions_image = Fixel::find_directions_header(input_fixel_folder).get_image<float>().with_direct_io(1);
  const index_type num_fixels = Fixel::get_number_of_fixels(index_header);

  const float angular_threshold = get_option_value("angle", DWI::Tractography::Mapping::default_streamline2fixel_angle);
  const bool precise = !get_options("precise").empty();

  const std::string output_fixel_folder = argument[2];
  Fixel::copy_index_and_directions_file(input_fixel_folder, output_fixel_folder);

  const std::string track_filename = argument[0];
  DWI::Tractography::Properties properties;
  DWI::Tractography::Reader<float> track_file(track_filename, properties);
  const size_t num_tracks = properties["count"].empty() ? 0 : to<size_t>(properties["count"]);
  if (!num_tracks)
    throw Exception("no tracks found in input file");
  const size_t upsample_ratio =
      DWI::Tractography::Mapping::determine_upsample_ratio(index_header, properties, precise ? 0.1F : (1.0F / 3.0F));

  DWI::Tractography::Mapping::TrackLoader loader(track_file, num_tracks, "mapping tracks to fixels");
  const std::string output_path = Path::join(output_fixel_folder, argument[3]);

  if (get_options("tck_weights_in").empty() && !precise) {
    run<uint32_t>(
        loader, index_image, directions_image, num_fixels, upsample_ratio, precise, angular_threshold, output_path);
  } else {
    run<float>(
        loader, index_image, directions_image, num_fixels, upsample_ratio, precise, angular_threshold, output_path);
  }
}
