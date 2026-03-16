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

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "dwi/tractography/properties.h"
#include "mrview/displayable.h"
#include "mrview/tool/tractography/tractography.h"
#include "trx/trx.h"

namespace MR::GUI {
class Projection;

namespace MRView {
class Window;

namespace Tool {
class Tractogram : public Displayable {
  Q_OBJECT

public:
  Tractogram(Tractography &tool, std::string_view file_path);

  ~Tractogram();

  Window &window() const { return *Window::main; }

  void render(const Projection &transform);

  void request_render_colourbar(DisplayableVisitor &visitor) override {
    if (color_type == TrackColourType::ScalarFile && show_colour_bar)
      visitor.render_tractogram_colourbar(*this);
  }

  void load_tracks();

  void load_end_colours();
  void load_intensity_track_scalars(std::string_view);
  void load_threshold_track_scalars(std::string_view);
  void erase_colour_data();
  void erase_intensity_scalar_data();
  void erase_threshold_scalar_data();

  // TRX-specific helpers
  bool is_trx() const;
  const std::vector<std::string> &trx_dps_fields() const;
  const std::vector<std::string> &trx_dpv_fields() const;
  const std::vector<std::string> &trx_group_names() const;
  void load_trx_scalar_field(const std::string &field_name, bool is_dpv);
  void load_trx_group_colours();
  void init_group_states();
  void reload_group_colours();

  struct GroupState {
    bool visible = true;
    Eigen::Vector3f color{0.5f, 0.5f, 0.5f};
    size_t count = 0;
  };
  std::map<std::string, GroupState> group_states;
  std::vector<std::string> group_order;
  bool show_ungrouped;
  GroupMultiPolicy group_multi_policy;

  void set_color_type(const TrackColourType);
  void set_threshold_type(const TrackThresholdType);
  void set_geometry_type(const TrackGeometryType);
  TrackColourType get_color_type() const { return color_type; }
  TrackThresholdType get_threshold_type() const { return threshold_type; }
  TrackGeometryType get_geometry_type() const { return geometry_type; }

  float get_threshold_rate() const {
    switch (threshold_type) {
    case TrackThresholdType::None:
      return NaNF;
    case TrackThresholdType::UseColourFile:
      return scaling_rate();
    case TrackThresholdType::SeparateFile:
      return (1e-3 * (threshold_max - threshold_min));
    }
    assert(0);
    return NaNF;
  }
  float get_threshold_min() const { return threshold_min; }
  float get_threshold_max() const { return threshold_max; }

  static TrackGeometryType default_tract_geom;
  static constexpr float default_line_thickness = 2e-3f;
  static constexpr float default_point_size = 4e-3f;

  bool scalarfile_by_direction;
  bool show_colour_bar;
  bool should_update_stride;
  float original_fov;
  float line_thickness;
  std::string intensity_scalar_filename;
  std::string threshold_scalar_filename;

  class Shader : public Displayable::Shader {
  public:
    Shader()
        : do_crop_to_slab(false),
          use_lighting(false),
          color_type(TrackColourType::Direction),
          threshold_type(TrackThresholdType::None),
          geometry_type(Tractogram::default_tract_geom) {}
    std::string vertex_shader_source(const Displayable &) override;
    std::string fragment_shader_source(const Displayable &) override;
    std::string geometry_shader_source(const Displayable &) override;
    virtual bool need_update(const Displayable &) const override;
    virtual void update(const Displayable &) override;

  protected:
    bool do_crop_to_slab, use_lighting;
    TrackColourType color_type;
    TrackThresholdType threshold_type;
    TrackGeometryType geometry_type;

  } track_shader;

signals:
  void scalingChanged();

private:
  static const int track_padding = 6;
  Tractography &tractography_tool;

  const std::string filename;

  TrackColourType color_type;
  TrackThresholdType threshold_type;
  TrackGeometryType geometry_type;

  // Instead of tracking the file path, pre-calculate the
  //   streamline tangents and store them; then, if colour by
  //   endpoint is requested, generate the buffer based on these
  //   and the known track sizes
  std::vector<Eigen::Vector3f> endpoint_tangents;

  std::vector<GLuint> vertex_buffers;
  std::vector<GLuint> vertex_array_objects;
  std::vector<GLuint> colour_buffers;
  std::vector<GLuint> intensity_scalar_buffers;
  std::vector<GLuint> threshold_scalar_buffers;
  MR::DWI::Tractography::Properties properties;
  std::vector<std::vector<GLint>> track_starts;
  std::vector<std::vector<GLint>> track_sizes;
  std::vector<std::vector<GLint>> original_track_sizes;
  std::vector<std::vector<GLint>> original_track_starts;
  std::vector<size_t> num_tracks_per_buffer;

  // EBOs and indices for chunks of tracks
  std::vector<GLuint> element_buffers;
  std::vector<GLsizei> element_counts;

  // TRX metadata and file cached at load time to avoid re-loading on every
  // field change (especially important for float16 files which require a
  // full decode pass and print a warning on every load).
  bool colour_buffers_from_groups;
  std::vector<std::string> cached_trx_dps_names;
  std::vector<std::string> cached_trx_dpv_names;
  std::vector<std::string> cached_trx_group_names;
  std::unique_ptr<trx::TrxFile<float>> cached_trx;

  trx::TrxFile<float> *get_cached_trx();

  GLint sample_stride;
  bool vao_dirty;

  // Extra members now required since different scalar files
  //   may be used for streamline colouring and thresholding
  float threshold_min, threshold_max;

  void load_tracks_onto_GPU(std::vector<Eigen::Vector3f> &buffer,
                            std::vector<GLint> &starts,
                            std::vector<GLint> &sizes,
                            size_t &tck_count);

  void load_end_colours_onto_GPU(std::vector<Eigen::Vector3f> &);

  void load_intensity_scalars_onto_GPU(std::vector<float> &buffer, size_t &tck_count);
  void load_threshold_scalars_onto_GPU(std::vector<float> &buffer, size_t &tck_count);

  void render_streamlines();

  void update_stride();

private slots:
  void on_FOV_changed() { should_update_stride = true; }
};

} // namespace Tool
} // namespace MRView
} // namespace MR::GUI
