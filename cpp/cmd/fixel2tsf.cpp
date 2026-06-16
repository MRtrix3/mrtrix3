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

#include "algo/loop.h"
#include "command.h"
#include "image.h"
#include "ordered_thread_queue.h"
#include "progressbar.h"
#include "thread.h"
#include "transform.h"

#include "fixel/fixel.h"
#include "fixel/helpers.h"
#include "fixel/validate.h"

#include "file/path.h"

#include "dwi/tractography/field_registry.h"
#include "dwi/tractography/formats/list.h"
#include "dwi/tractography/scalar_file.h"
#include "dwi/tractography/sidecar.h"
#include "dwi/tractography/sidecar_embed.h"
#include "dwi/tractography/sidecar_value.h"
#include "dwi/tractography/streamline.h"
#include "dwi/tractography/tractogram.h"
#include "dwi/tractography/tractogram_item.h"

#include "dwi/tractography/mapping/loader.h"
#include "dwi/tractography/mapping/mapper.h"
#include "dwi/tractography/mapping/mapping.h"

#include <cstdint>
#include <filesystem>

using namespace MR;
using namespace App;

using Fixel::index_type;

// clang-format off
void usage() {

  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  SYNOPSIS = "Map fixel values to a track scalar file based on an input tractogram";

  DESCRIPTION
  + "This command is useful for visualising all brain fixels"
    " (e.g. the output from fixelcfestats)"
    " in 3D."

  + "By default the sampled fixel values are written to a standalone track scalar"
    " file (.tsf). Alternatively, they may be embedded into a tractography dataset as a"
    " named per-vertex (data-per-vertex) sidecar field, using the qualified"
    " \"DATASET::NAME\" form for the output argument. If DATASET does not yet exist it is"
    " created as a copy of the input tractogram carrying the new field, generated within"
    " the same pass that performs the sampling. If DATASET already exists and its format"
    " supports adding a field in place (a TRX directory or uncompressed archive), the"
    " field is appended without rewriting the streamline data; the -force option is then"
    " required only if a field named NAME is already present. If DATASET already exists"
    " but cannot be augmented in place (e.g. \".trk\", or a compressed TRX archive), the"
    " -force option is required and the dataset is rewritten with the field added."

  + Fixel::format_description;

  ARGUMENTS
  + Argument ("fixel_in", "the input fixel data file (within the fixel directory)").type_image_in ()
  + Argument ("tracks",   "the input track file").type_tracks_in ()
  + Argument ("tsf",      "the output track scalar file, or a \"DATASET::NAME\" embedded sidecar field")
    .type_tractogram_data_out (TractogramDataOutMode::MayCreateDataset);


  OPTIONS
  + Option ("angle", "the max anglular threshold for computing correspondence"
                     " between a fixel direction and track tangent"
                     " (default = " + str(DWI::Tractography::Mapping::default_streamline2fixel_angle, 2) + " degrees)")
  + Argument ("value").type_float (0.001, 90.0);

}
// clang-format on

using value_type = float;
using SetVoxelDir = DWI::Tractography::Mapping::SetVoxelDir;

//! \brief Maps the closest-fixel scalar value onto each vertex of a streamline.
/*! Holds its own copies of the fixel index / directions / data images and a precise
 * track mapper, so it can be copied into each worker thread of the sampling queue
 * (each copy carries independent image cursors over the shared data). One call
 * produces the per-vertex (.tsf-shaped) scalar sequence for a single streamline:
 * for each vertex the streamline tangent is matched to the fixel within the
 * occupied voxel whose direction is closest (within the angular threshold), and
 * that fixel's scalar is emitted (zero where no fixel qualifies). */
class FixelSampler {
public:
  FixelSampler(const Image<index_type> &index_image,
               const Image<value_type> &directions_image,
               const Image<value_type> &data_image,
               const value_type angular_threshold_dp)
      : index_image(index_image),
        directions_image(directions_image),
        data_image(data_image),
        angular_threshold_dp(angular_threshold_dp),
        mapper(index_image),
        transform(index_image) {
    mapper.set_use_precise_mapping(true);
  }
  FixelSampler(const FixelSampler &) = default;

  void operator()(const DWI::Tractography::Streamline<value_type> &tck,
                  DWI::Tractography::TrackScalar<value_type> &scalars) {
    SetVoxelDir dixels;
    mapper(tck, dixels);
    scalars.clear();
    scalars.set_index(tck.get_index());
    scalars.resize(tck.size(), 0.0f);
    Eigen::Vector3d voxel_pos_float;
    Eigen::Vector3i voxel_pos_int;
    for (size_t p = 0; p < tck.size(); ++p) {
      voxel_pos_float = transform.scanner2voxel * tck[p].cast<default_type>();
      voxel_pos_int = voxel_pos_float.array().round().cast<int>();
      for (const auto &d : dixels) {
        // Invokes Mapping::Voxel::operator==();
        //   ie. only checks 3D voxel indices, not direction within voxel
        if (voxel_pos_int == d) {
          assign_pos_of(d).to(index_image);
          const Eigen::Vector3f dir = d.get_dir().cast<float>().normalized();
          value_type largest_dp = 0.0f;
          int32_t closest_fixel_index = -1;

          index_image.index(3) = 0;
          const index_type num_fixels_in_voxel = index_image.value();
          index_image.index(3) = 1;
          const index_type offset = index_image.value();

          for (size_t fixel = 0; fixel < num_fixels_in_voxel; ++fixel) {
            directions_image.index(0) = offset + fixel;
            const value_type dp = std::fabs(dir.dot(Eigen::Vector3f(directions_image.row(1))));
            if (dp > largest_dp) {
              largest_dp = dp;
              closest_fixel_index = fixel;
            }
          }
          if (largest_dp > angular_threshold_dp) {
            data_image.index(0) = offset + closest_fixel_index;
            const value_type value = data_image.value();
            scalars[p] = std::isfinite(value) ? value : 0.0f;
          } else {
            scalars[p] = 0.0f;
          }
          break;
        }
      }
    }
  }

private:
  Image<index_type> index_image;
  Image<value_type> directions_image;
  Image<value_type> data_image;
  const value_type angular_threshold_dp;
  DWI::Tractography::Mapping::TrackMapperBase mapper;
  Transform transform;
};

//! \brief Queue pipe producing the per-vertex scalar sequence for each streamline.
/*! Reads each composite item through the Tractogram framework and feeds its
 * streamline to the sampler, emitting the .tsf-shaped scalar sequence the
 * standalone receiver consumes. */
class SampleWorker {
public:
  explicit SampleWorker(const FixelSampler &sampler) : sampler(sampler) {}
  SampleWorker(const SampleWorker &) = default;
  bool operator()(const DWI::Tractography::TractogramItem<value_type> &in,
                  DWI::Tractography::TrackScalar<value_type> &out) {
    sampler(in.streamline, out);
    return true;
  }

private:
  FixelSampler sampler;
};

//! \brief Ordered queue sink writing each per-vertex scalar sequence to a .tsf file.
class ScalarFileSink {
public:
  ScalarFileSink(const DWI::Tractography::Properties &properties,
                 const std::filesystem::path &path,
                 const size_t num_tracks)
      : writer(path, properties),
        progress("mapping fixel values to streamline points", num_tracks),
        received(0),
        expected(num_tracks) {}
  ScalarFileSink(const ScalarFileSink &) = delete;
  ~ScalarFileSink() {
    if (received != expected) {
      WARN("Track file reports " + str(expected) + " tracks, but contains " + str(received));
    }
  }
  bool operator()(const DWI::Tractography::TrackScalar<value_type> &in) {
    writer(in);
    ++received;
    ++progress;
    return true;
  }

private:
  DWI::Tractography::ScalarWriter<value_type> writer;
  ProgressBar progress;
  size_t received;
  const size_t expected;
};

//! \brief Queue pipe slotting the sampled per-vertex scalars into a new dpv field.
/*! The output item is the input item plus one extra per-vertex (dpv) field at
 * \a ordinal holding the sampled scalar column, so the embedding output tractogram
 * is generated by the sampling queue itself without a second pass over the input. */
class EmbedWorker {
public:
  EmbedWorker(const FixelSampler &sampler, const size_t ordinal) : sampler(sampler), ordinal(ordinal) {}
  EmbedWorker(const EmbedWorker &) = default;
  bool operator()(const DWI::Tractography::TractogramItem<value_type> &in,
                  DWI::Tractography::TractogramItem<value_type> &out) {
    out = in;
    DWI::Tractography::TrackScalar<value_type> scalar;
    sampler(in.streamline, scalar);
    DWI::Tractography::VectorOrMatrix<value_type> column(static_cast<Eigen::Index>(scalar.size()), 1);
    for (size_t i = 0; i != scalar.size(); ++i)
      column(static_cast<Eigen::Index>(i), 0) = scalar[i];
    if (out.dpv.size() <= ordinal)
      out.dpv.resize(ordinal + 1);
    out.dpv[ordinal] = DWI::Tractography::make_dpv(std::move(column));
    return true;
  }

private:
  FixelSampler sampler;
  size_t ordinal;
};

void run() {
  const std::filesystem::path input_fixel_path{argument[0]};
  const std::filesystem::path input_tracks_path{argument[1]};

  auto in_data_image = Fixel::open_fixel_data_file<value_type>(input_fixel_path);
  if (in_data_image.size(2) != 1)
    throw Exception("Only a single scalar value for each fixel can be output as a track scalar file, "
                    "therefore the input fixel data file must have dimension Nx1x1");
  const std::filesystem::path input_fixel_directory = Fixel::get_fixel_directory(argument[0]);
  Header in_index_header = Fixel::find_index_header(input_fixel_directory);
  Fixel::check_fixel_size(in_index_header, in_data_image);
  auto in_index_image = in_index_header.get_image<index_type>();
  Fixel::debug_validate_index_image(in_index_image);
  auto in_directions_image = Fixel::find_directions_header(input_fixel_directory).get_image<value_type>(DirectIO(1));

  const float angular_threshold = get_option_value("angle", DWI::Tractography::Mapping::default_streamline2fixel_angle);
  const value_type angular_threshold_dp = cos(angular_threshold * (Math::pi / 180.0));
  const FixelSampler sampler(in_index_image, in_directions_image, in_data_image, angular_threshold_dp);

  const DWI::Tractography::SidecarReference reference =
      DWI::Tractography::parse_sidecar_reference(argument[2].as_text());

  DWI::Tractography::Properties properties;
  auto input = DWI::Tractography::Tractogram<value_type>::open(input_tracks_path, properties);
  const size_t num_tracks = properties.find("count") == properties.end() ? 0 : to<size_t>(properties["count"]);

  using Item = DWI::Tractography::TractogramItem<value_type>;

  // A bare path that no tractography handler recognises is a standalone track scalar
  //   file (.tsf); a qualified "DATASET::NAME" reference, or a recognised tractography
  //   format, routes to the shared sidecar-embedding orchestration instead.
  if (DWI::Tractography::Formats::get_handler(reference.dataset) == nullptr) {
    if (reference.is_qualified())
      throw Exception("output \"" + std::string(argument[2].as_text()) + "\"" +
                      " uses the qualified \"DATASET::NAME\" sidecar form," + " but \"" + reference.dataset.string() +
                      "\" is not a recognised tractography format");
    if (!Path::has_suffix(reference.dataset, ".tsf"))
      throw Exception(std::string("standalone fixel2tsf output must be a track scalar file (\".tsf\");") +
                      " to embed the sampled values into a tractogram dataset," +
                      " use the qualified \"DATASET::NAME\" form");
    ScalarFileSink sink(properties, reference.dataset, num_tracks);
    SampleWorker worker(sampler);
    Thread::run_ordered_queue(input,
                              Thread::batch(Item()),
                              Thread::multi(worker),
                              Thread::batch(DWI::Tractography::TrackScalar<value_type>()),
                              sink);
    return;
  }

  DWI::Tractography::embed_sidecar_field<value_type>(
      reference,
      input,
      properties,
      DWI::Tractography::FieldRole::DPV,
      1,
      num_tracks,
      [&](auto &sink, const size_t ordinal) {
        EmbedWorker worker(sampler, ordinal);
        Thread::run_ordered_queue(input, Thread::batch(Item()), Thread::multi(worker), Thread::batch(Item()), sink);
      });
}
