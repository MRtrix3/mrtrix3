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

#include <array>
#include <cstdint>
#include <vector>

#include "dwi/tractography/properties.h"
#include "mrview/displayable.h"
#include "mrview/tool/tractography/tractography.h"

#include <filesystem>

namespace MR::GUI {
class Projection;

namespace MRView {
class Window;

namespace Tool {
class Tractogram : public Displayable {
  Q_OBJECT

public:
  Tractogram(Tractography &tool, const std::filesystem::path &file_path);

  ~Tractogram();

  Window &window() const { return *Window::main; }

  void render(const Projection &transform);

  void request_render_colourbar(DisplayableVisitor &visitor) override {
    if (color_type == TrackColourType::ScalarFile && show_colour_bar)
      visitor.render_tractogram_colourbar(*this);
  }

  void load_tracks();

  void load_end_colours();
  void load_intensity_track_scalars(const std::filesystem::path &);
  void load_threshold_track_scalars(const std::filesystem::path &);
  void erase_colour_data();
  void erase_intensity_scalar_data();
  void erase_threshold_scalar_data();

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
  bool should_update_lod;
  float original_fov;
  float line_thickness;
  std::filesystem::path intensity_scalar_path;
  std::filesystem::path threshold_scalar_path;

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
  //! Number of available level-of-detail sub-sampling levels.
  //! Levels use geometric sub-sampling ratios (1, 2, 4, 8, ...): level \a l draws
  //!   every (1 << l)-th vertex, so the maximum ratio is (1 << (num_lod_levels - 1)).
  //!   The total host-side index storage across all levels converges to ~2x the
  //!   vertex count irrespective of the level count.
  static constexpr size_t num_lod_levels = 6;
  //! Sub-sampling ratio (stride) for a given level index.
  static constexpr GLint lod_ratio_for_level(const size_t level) { return GLint(1) << level; }
  //! Minimum permissible on-screen spacing between consecutive drawn vertices, in
  //!   pixels; below this, samples are visually redundant regardless of geometry.
  static constexpr float min_vertex_spacing_px = 2.0f;
  Tractography &tractography_tool;

  const std::filesystem::path filepath;

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
  // Per-vertex TrackVertexType classification (uint8_t), one buffer per chunk
  std::vector<GLuint> vertex_type_buffers;
  std::vector<GLuint> colour_buffers;
  std::vector<GLuint> intensity_scalar_buffers;
  std::vector<GLuint> threshold_scalar_buffers;
  MR::DWI::Tractography::Properties properties;
  // Number of vertices per streamline, retained per chunk for rebuilding
  //   element buffers and for replaying chunk boundaries when loading the
  //   per-vertex colour / scalar side buffers
  std::vector<std::vector<GLint>> original_track_sizes;
  std::vector<size_t> num_tracks_per_buffer;

  // One GPU element buffer object per chunk; its contents are swapped between
  //   the precomputed sub-sampling levels as the level of detail changes
  std::vector<GLuint> element_buffers;
  std::vector<GLsizei> element_counts;
  // Precomputed (host-side) draw indices per chunk, per sub-sampling level
  //   (index l -> ratio (1 << l): level 0 -> ratio 1, level 1 -> ratio 2, ...)
  std::vector<std::array<std::vector<uint32_t>, num_lod_levels>> element_indices;

  // Active sub-sampling level (0 .. num_lod_levels - 1); selects which precomputed
  //   element buffer is resident on the GPU
  size_t lod_level;
  // Flags that the resident element buffers no longer match lod_level
  bool ebo_dirty;
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

  //! On-screen sizes (in pixels) shared by rendering and level-of-detail selection.
  //! Derived from the current field of view and viewport so that the level of
  //!   detail tracks zoom, and computed from the same expressions the shaders draw.
  struct ScreenMetrics {
    float pixels_per_mm;     //!< on-screen pixels per world-space millimetre
    float tube_width_px;     //!< pseudotube full width on screen
    float point_diameter_px; //!< point disk diameter on screen
  };
  ScreenMetrics screen_metrics(const Projection &transform) const;

  void render_streamlines(const ScreenMetrics &metrics);

  //! Recompute lod_level from the current screen-space metrics.
  void update_lod(const ScreenMetrics &metrics);
  //! Upload the element buffers for the active lod_level to the GPU.
  void update_element_buffers();

private slots:
  void on_FOV_changed() { should_update_lod = true; }
};

} // namespace Tool
} // namespace MRView
} // namespace MR::GUI
