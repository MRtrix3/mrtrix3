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

#include "surface/freesurfer.h"

#include "debug.h"

namespace MR::Surface::FreeSurfer {

void read_annot(const std::filesystem::path &path, label_vector_type &labels, Connectome::LUT &lut) {
  std::ifstream in(path, std::ios_base::in | std::ios_base::binary);
  if (!in)
    throw Exception("Error opening input file!");

  const auto num_vertices = get_BE<int32_t>(in);
  std::vector<int32_t> vertices, vertex_labels;
  vertices.reserve(num_vertices);
  vertex_labels.reserve(num_vertices);
  for (int32_t i = 0; i != num_vertices; ++i) {
    vertices.push_back(get_BE<int32_t>(in));
    vertex_labels.push_back(get_BE<int32_t>(in));
  }

  const auto colortable_present = get_BE<int32_t>(in);
  if (!in.good()) {
    WARN("FreeSurfer annotation file \"" + path.filename().string() + "\" does not contain colortable information");
    labels = label_vector_type::Zero(num_vertices);
    for (size_t i = 0; i != vertices.size(); ++i)
      labels[vertices[i]] = vertex_labels[i];
    return;
  }
  if (colortable_present == 0)
    throw Exception("Error reading FreeSurfer annotation file \"" + path.filename().string() + "\":" + //
                    " Unexpected colortable flag");                                                    // a

  // Structure that will map from the colour-based structure identifier to a more sensible index
  std::map<int32_t, Connectome::node_t> rgb2index;

  const auto num_entries = get_BE<int32_t>(in);
  if (num_entries > 0) {

    const auto orig_lut_name_length = get_BE<int32_t>(in);
    const std::unique_ptr<char[]> orig_lut_name(new char[orig_lut_name_length]);
    in.read(orig_lut_name.get(), orig_lut_name_length);
    for (int32_t i = 0; i != num_entries; ++i) {
      const auto struct_name_length = get_BE<int32_t>(in);
      const std::unique_ptr<char[]> struct_name(new char[struct_name_length]);
      in.read(struct_name.get(), struct_name_length);
      const auto r = get_BE<int32_t>(in);
      const auto g = get_BE<int32_t>(in);
      const auto b = get_BE<int32_t>(in);
      const auto flag = get_BE<int32_t>(in);
      const int32_t id = r + (g << 8) + (b << 16) + (flag << 24);
      lut.insert(std::make_pair(i, Connectome::LUT_node(std::string(struct_name.get()), r, g, b)));
      rgb2index[id] = i;
    }

  } else {

    const int32_t version = -num_entries;
    if (version != 2)
      throw Exception("Error reading FreeSurfer annotation file \"" + path.filename().string() + "\": " + //
                      " unsupported file version (" + str(version) + ")");                                //

    get_BE<int32_t>(in);
    const auto orig_lut_name_length = get_BE<int32_t>(in);
    const std::unique_ptr<char[]> orig_lut_name(new char[orig_lut_name_length]);
    in.read(orig_lut_name.get(), orig_lut_name_length);

    const auto num_entries_to_read = get_BE<int32_t>(in);
    for (int32_t i = 0; i != num_entries_to_read; ++i) {
      const int32_t structure = get_BE<int32_t>(in) + 1;
      if (structure < 0)
        throw Exception("Error reading FreeSurfer annotation file \"" + path.filename().string() +
                        "\": Negative structure index");
      if (lut.find(structure) != lut.end())
        throw Exception("Error reading FreeSurfer annotation file \"" + path.filename().string() +
                        "\": Duplicate structure index");
      const auto struct_name_length = get_BE<int32_t>(in);
      const std::unique_ptr<char[]> struct_name(new char[struct_name_length]);
      in.read(struct_name.get(), struct_name_length);
      const auto r = get_BE<int32_t>(in);
      const auto g = get_BE<int32_t>(in);
      const auto b = get_BE<int32_t>(in);
      const auto flag = get_BE<int32_t>(in);
      const int32_t id = r + (g << 8) + (b << 16) + (flag << 24);
      lut.insert(std::make_pair(structure, Connectome::LUT_node(std::string(struct_name.get()), r, g, b)));
      rgb2index[id] = structure;
    }
  }

  labels = label_vector_type::Zero(num_vertices);
  for (size_t i = 0; i != vertices.size(); ++i)
    labels[vertices[i]] = rgb2index[vertex_labels[i]];
}

void read_label(const std::filesystem::path &path, VertexList &vertices, Scalar &scalar) {
  vertices.clear();
  scalar.resize(0);

  std::ifstream in(path, std::ios_base::in);
  if (!in)
    throw Exception("Error opening input file!");

  std::string line;
  std::getline(in, line);
  if (line.substr(0, 13) != "#!ascii label")
    throw Exception("Error parsing FreeSurfer label file \"" + path.filename().string() + "\":" + //
                    " bad first line identifier");                                                //
  std::getline(in, line);
  uint32_t num_vertices = 0;
  try {
    num_vertices = to<size_t>(line);
  } catch (Exception &e) {
    throw Exception(
        e, "Error parsing FreeSurfer label file \"" + path.filename().string() + "\": Bad second line vertex count");
  }

  for (size_t i = 0; i != num_vertices; ++i) {
    std::getline(in, line);
    uint32_t index = std::numeric_limits<uint32_t>::max();
    default_type x = NaN;
    default_type y = NaN;
    default_type z = NaN;
    default_type value = NaN;
    sscanf(line.c_str(), "%u %lf %lf %lf %lf", &index, &x, &y, &z, &value);
    if (index == std::numeric_limits<uint32_t>::max())
      throw Exception("Error parsing FreeSurfer label file \"" + path.filename().string() + "\": Malformed line");
    if (index >= scalar.size()) {
      scalar.conservativeResizeLike(Scalar::Base::Constant(index + 1, NaN));
      vertices.resize(index + 1, Vertex(NaN, NaN, NaN));
    }
    if (std::isfinite(scalar[index]))
      throw Exception("Error parsing FreeSurfer label file \"" + path.filename().string() + "\":" + //
                      " duplicated index (" + str(scalar[index]) + ")");                            //
    scalar[index] = value;
    vertices[index] = Vertex(x, y, z);
  }

  if (!in.good())
    throw Exception("Error parsing FreeSurfer label file \"" + path.filename().string() + "\": end of file reached");
  scalar.set_name(path.filename().string());
}

} // namespace MR::Surface::FreeSurfer
