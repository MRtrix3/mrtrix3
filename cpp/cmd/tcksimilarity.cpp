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

#include "command.h"
#include "image.h"
#include "types.h"

#include "file/ofstream.h"
#include "file/path.h"
#include "file/utils.h"

#include <unordered_map>
#include <unordered_set>
#include <cmath>

#include "dwi/tractography/file.h"
#include "dwi/tractography/mapping/loader.h"
#include "dwi/tractography/mapping/mapper.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/streamline.h"

#include "thread_queue.h"

#include "progressbar.h"
#include "algo/loop.h"

#define DEFAULT_SIMILARITY_THRESHOLD 0.1

using namespace MR::DWI;
using namespace MR::DWI::Tractography;
using namespace MR::DWI::Tractography::Mapping;

using namespace MR;
using namespace App;

void usage() {
  AUTHOR = "Simone Zanoni (simone.zanoni@sydney.edu.au)";

  SYNOPSIS = "Generate a streamline-streamline similarity matrix";

  DESCRIPTION
  +"This command will generate a directory containing three images, which "
   "encode the streamline-streamline similarity matrix.";

  ARGUMENTS

  + Argument("tracks", "the tracks the similarity is computed on")
          .type_tracks_in()
  + Argument("template", "an image file to be used as a template for the "
                          "shared volume ")
        .type_image_in()
  + Argument("matrix",
              "the output streamline-streamline similarity matrix directory path")
        .type_directory_out();

  OPTIONS
  + OptionGroup("Options that influence generation of the similarity matrix")
  + Option("threshold",
            "a threshold for the streamline-streamline similarity to determine the "
            "the similarity values to be included in the matrix (default: " +
                str(DEFAULT_SIMILARITY_THRESHOLD, 2) + ")") +
    Argument("value").type_float(0.0, 1.0);

  REFERENCES
  + "Zanoni, S.; Lv, J.; Smith, R. E. & Calamante, F. "
    "Streamline-Based Analysis: "
    "A novel framework for tractogram-driven streamline-wise statistical analysis. "
    "Proceedings of the International Society for Magnetic Resonance in Medicine, 2025, 4781";
}

// Index source class for generating track indices
class TrackIndexSource {
public:
  TrackIndexSource(size_t max_indices)
      : current_index(0), max_indices(max_indices) {}

  bool operator()(size_t &index) {
    if (current_index >= max_indices)
      return false;

    index = current_index++;
    return true;
  }

private:
  size_t current_index;
  const size_t max_indices;
};

struct VoxelComparator {
  bool operator()(const Eigen::Vector3i &a, const Eigen::Vector3i &b) const {
    if (a[0] != b[0])
      return a[0] < b[0];
    if (a[1] != b[1])
      return a[1] < b[1];
    return a[2] < b[2];
  }
};

class LabelCollector {
public:
  LabelCollector(Image<float> &label_image,
                 std::shared_ptr<std::vector<std::map<int, float>>> &int_lst)
      : image(label_image),
        mapper(new DWI::Tractography::Mapping::TrackMapperBase(label_image)),
        intersected_lists(int_lst) {
    mapper->set_use_precise_mapping(false);
  }

  bool operator()(const Streamline<float> &tck) {
    std::map<int, float> curr_lab;
    DWI::Tractography::Mapping::SetVoxel voxels;
    (*mapper)(tck, voxels);

    // original voxels
    std::set<Eigen::Vector3i, VoxelComparator> original_voxels;
    for (const auto &v : voxels) {
      Eigen::Vector3i voxel_pos(v[0], v[1], v[2]);
      original_voxels.insert(voxel_pos);
    }

    // intersected voxels
    for (const auto &v : voxels) {
      assign_pos_of(v).to(image);
      curr_lab[image.value()] += 1.0f;
    }

    // dilated voxels
    std::set<Eigen::Vector3i, VoxelComparator> dilated_voxels;

    for (const auto &voxel_pos : original_voxels) {
      for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
          for (int dz = -1; dz <= 1; ++dz) {
            // skip the center
            if (dx == 0 && dy == 0 && dz == 0)
              continue;

            Eigen::Vector3i neighbor(voxel_pos[0] + dx, voxel_pos[1] + dy,
                                     voxel_pos[2] + dz);

            if (original_voxels.find(neighbor) == original_voxels.end() &&
                check(neighbor, image)) {

              dilated_voxels.insert(neighbor);
            }
          }
        }
      }
    }

    float dilated_weight = 0.5f;

    for (const auto &voxel_pos : dilated_voxels) {
      assign_pos_of(voxel_pos).to(image);
      curr_lab[image.value()] += dilated_weight;
    }

    float total_count = 0.0f;
    for (const auto &entry : curr_lab)
      total_count += entry.second;

    if (total_count > 0.0f) {
      for (auto &entry : curr_lab) {
        entry.second /= total_count;
      }
    }

    (*intersected_lists)[tck.get_index()] = curr_lab;

    return true;
  }

private:
  Image<float> image;
  std::shared_ptr<DWI::Tractography::Mapping::TrackMapperBase> mapper;
  std::shared_ptr<std::vector<std::map<int, float>>> intersected_lists;
};

float compute_inner_product(const std::map<int, float> &normalized_labels1,
                            const std::map<int, float> &normalized_labels2) {
  float inner_product = 0.0f;

  auto it1 = normalized_labels1.begin();
  auto it2 = normalized_labels2.begin();

  while (it1 != normalized_labels1.end() && it2 != normalized_labels2.end()) {
    if (it1->first == it2->first) {
      inner_product += it1->second * it2->second;
      ++it1;
      ++it2;
    } else if (it1->first < it2->first) {
      ++it1;
    } else {
      ++it2;
    }
  }

  return inner_product;
}

float compute_similarity(const std::map<int, float> &normalized_labels1,
                         const std::map<int, float> &normalized_labels2) {

  float intersection_size = 0;
  float union_size = 0;

  // intersection size
  for (const auto &l1 : normalized_labels1) {
    if (normalized_labels2.find(l1.first) != normalized_labels2.end()) {
      intersection_size++;
    }
  }

  union_size =
      normalized_labels1.size() + normalized_labels2.size() - intersection_size;

  float iou = 0.0f;
  // possibly this check is not needed
  if (union_size > 0) {
    iou = intersection_size / union_size;
  } else {
    return 0.0f;
  }

  float inner_product =
      compute_inner_product(normalized_labels1, normalized_labels2);

  return iou * inner_product;
}

class StreamlineSimilarityProcessor {
public:
  using ResultPair = std::pair<size_t, float>;

  StreamlineSimilarityProcessor(
      const std::shared_ptr<std::vector<std::map<int, float>>> &labels,
      const std::shared_ptr<std::unordered_map<int, std::vector<size_t>>>
          &inverted_index,
      float similarity_threshold)
      : track_labels(labels), inverted_index(inverted_index),
        threshold(similarity_threshold),
        results(
            std::make_shared<std::vector<std::vector<ResultPair>>>(labels->size())),
        progress(std::make_shared<ProgressBar>(
            "computing streamline similarities", labels->size())) {}

  bool operator()(size_t track_index) {
    const auto &input = (*track_labels)[track_index];
    std::vector<ResultPair> current_results;
    current_results.reserve(track_labels->size());

    // compute self-similarity
    float max_similarity = compute_similarity(input, input);
    current_results.emplace_back(ResultPair(track_index, 1.0f));

    // find candidates
    std::set<int> track_labels_set;
    for (const auto &label_entry : input) {
      track_labels_set.insert(label_entry.first);
    }
    std::unordered_set<size_t> candidate_indices;
    for (int label : track_labels_set) {
      if (inverted_index->find(label) != inverted_index->end()) {
        const auto &track_indices_with_label = (*inverted_index)[label];
        candidate_indices.insert(track_indices_with_label.begin(),
                                 track_indices_with_label.end());
      }
    }

    for (size_t other_index : candidate_indices) {
      if (track_index != other_index) {
        const auto &track_data = (*track_labels)[other_index];

        // Compute similarity only for tracks with overlapping labels
        float similarity =
            compute_similarity(input, track_data) / max_similarity;
        similarity = std::min(similarity, 1.0f);

        if (similarity > threshold)
          current_results.emplace_back(ResultPair(other_index, similarity));
      }
    }

    (*results)[track_index] = current_results;

    ++(*progress);

    return true;
  }

  const std::vector<std::vector<ResultPair>> &get_results() const {
    return *results;
  }

  ~StreamlineSimilarityProcessor() { progress.reset(); }

private:
  const std::shared_ptr<std::vector<std::map<int, float>>> track_labels;
  const std::shared_ptr<std::unordered_map<int, std::vector<size_t>>>
      inverted_index;
  float threshold;
  std::shared_ptr<std::vector<std::vector<ResultPair>>> results;
  std::shared_ptr<ProgressBar> progress;
};

void run() {
  const float similarity_threshold =
      get_option_value("threshold", float(DEFAULT_SIMILARITY_THRESHOLD));
  if (similarity_threshold == 1.0)
    throw Exception("Setting -threshold to 1 would defeat the purpose of the "
                    "similarity computation");

  // destination folder for tracks similarity
  const std::string path = argument[2];
  if (Path::exists(path)) {
    if (!Path::is_dir(path)) {
      if (App::overwrite_files) {
        File::remove(path);
      } else {
        throw Exception("Cannot create streamline similarity matrix \"" + path +
                        "\": Already exists as file");
      }
    }
  } else {
    File::mkdir(path);
  }

  Properties properties;
  Reader<float> reader(argument[0], properties);
  const size_t num_tracks =
      properties["count"].empty() ? 0 : to<size_t>(properties["count"]);
  CONSOLE("Number of tracks found in input tck file: " + str(num_tracks));
  if (num_tracks < 2) {
    WARN("Not enough tracks found in input tck file");
    return;
  }

  // create label grid for mapping based on the template image
  Header header = Header::open(argument[1]);
  auto labeled = Image<float>::scratch(header);
  float current_label = 1;
  for (auto l = Loop(labeled)(labeled); l; ++l) {
    labeled.value() = current_label++;
  }

  auto image = header.get_image<float>();

  // SAMPLE LABEL GRID
  Reader<float> tck_reader(argument[0], properties);
  TrackLoader loader(tck_reader, num_tracks);
  auto intersected_lists =
      std::make_shared<std::vector<std::map<int, float>>>(num_tracks);
  auto inverted_index =
      std::make_shared<std::unordered_map<int, std::vector<size_t>>>(
          current_label - 1);

  Thread::run_queue(loader, Thread::batch(Streamline<>()),
                    Thread::multi(LabelCollector(labeled, intersected_lists)));

  CONSOLE("Computing inverted index vector");
  for (size_t track_index = 0; track_index < intersected_lists->size();
       ++track_index) {
    const auto &labels = (*intersected_lists)[track_index];
    for (const auto &label_entry : labels) {
      int label = label_entry.first;
      (*inverted_index)[label].emplace_back(track_index);
    }
  }

  // SIMILARITY COMPUTATION
  const auto results = [&]() {
    StreamlineSimilarityProcessor processor(intersected_lists, inverted_index,
                                            similarity_threshold);

    TrackIndexSource index_source(intersected_lists->size());

    Thread::run_queue(index_source, Thread::batch(size_t()),
                      Thread::multi(processor));

    return processor.get_results();
  }();

  // WRITE

  // TODO: use .npy instead of .mif

  Header index_header;
  index_header.ndim() = 4;
  index_header.size(0) = results.size();
  index_header.size(1) = 1;
  index_header.size(2) = 1;
  index_header.size(3) = 2;
  index_header.stride(0) = 2;
  index_header.stride(1) = 3;
  index_header.stride(2) = 4;
  index_header.stride(3) = 1;
  index_header.spacing(0) = index_header.spacing(1) = index_header.spacing(2) =
      1.0;
  index_header.transform() = transform_type::Identity();
  index_header.datatype() = DataType::UInt64;
  auto index_image = Image<uint64_t>::create(
      Path::join(argument[2], "index.mif"), index_header);

  File::OFStream streamline_stream(Path::join(argument[2], "streamlines.mif"),
                                   std::ios_base::out | std::ios_base::binary);
  File::OFStream value_stream(Path::join(argument[2], "values.mif"),
                              std::ios_base::out | std::ios_base::binary);

  const std::string leadin = "mrtrix image\ndim: ";
  const size_t dim_padding = std::log10(std::numeric_limits<size_t>::max()) + 4;
  Eigen::IOFormat fmt(Eigen::FullPrecision, Eigen::DontAlignCols, ", ",
                      "\ntransform: ", "", "", "\ntransform: ", "");

  for (size_t stream_index = 0; stream_index != 2; ++stream_index) {
    File::OFStream &stream(stream_index ? value_stream : streamline_stream);
    stream << leadin << std::string(dim_padding, ' ') << "\n";
    stream << "vox: 1,1,1\n";
    stream << "layout: +0,+1,+2\n";
    stream << "datatype: "
           << (stream_index ? DataType::from<float>().specifier()
                            : DataType::from<int32_t>().specifier());
    stream << transform_type::Identity().matrix().topLeftCorner(3, 4).format(
                  fmt)
           << "\n";
    stream << "file: ";
    uint64_t offset = uint64_t(stream.tellp()) + 18;
    offset += ((4 - (offset % 4)) % 4);
    stream << ". " << offset << "\nEND\n";
    stream << std::string(offset - uint64_t(stream.tellp()), '\0');
  }

  // write index.mif values
  size_t data_count = 0;
  ProgressBar progress("writing streamline similarity matrix", results.size());
  for (size_t i = 0; i < results.size(); ++i) {

    index_image.index(0) = i;
    index_image.index(3) = 0;
    index_image.value() = results[i].size();
    index_image.index(3) = 1;
    index_image.value() = results[i].size() ? data_count : uint64_t(0);

    for (const auto &pair : results[i]) {
      streamline_stream.write(reinterpret_cast<const char *>(&pair.first),
                              sizeof(int32_t));
      value_stream.write(reinterpret_cast<const char *>(&pair.second),
                         sizeof(float));
    }
    data_count += results[i].size();
    ++progress;
  }

  // update headers with final count
  std::string dim_string = str(data_count) + ",1,1";
  dim_string += std::string(dim_padding - dim_string.size(), ' ');
  for (size_t stream_index = 0; stream_index != 2; ++stream_index) {
    File::OFStream &stream(stream_index ? value_stream : streamline_stream);
    stream.seekp(leadin.size());
    stream << dim_string;
  }
}
