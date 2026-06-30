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
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "dwi/tractography/field_registry.h"
#include "dwi/tractography/properties.h"
#include "mrview/displayable.h"
#include "mrview/tool/tractography/tractography.h"
#include "mrview/tool/tractography/vertex_block_source.h"

#include <filesystem>

namespace MR::GUI {
class Projection;

namespace MRView {
class Window;

namespace Tool {

//! \brief Width of the per-tractogram GPU vertex coordinate buffer.
/*! Selected once at load from the on-disk vertex datatype (or forced by the
 * MRViewTractogramHalfPrecisionGPU config flag). Positions are always exposed
 * to the shader as float (half attributes are promoted on fetch); only the CPU
 * buffer element width and the VertexAttribPointer type enum differ. */
enum class VertexGPUType { Float32, Float16 };

//! \brief Width of a per-vertex scalar (intensity/threshold) GPU buffer.
/*! Floating-point sidecar data may be stored half-precision when the
 * MRViewTractogramHalfPrecisionGPU config flag is set (the same flag that
 * governs vertex storage); otherwise full precision is used. As with vertices,
 * the shader always receives a float (half attributes are promoted on fetch);
 * only the CPU buffer element width and the VertexAttribPointer type enum
 * differ. */
enum class ScalarGPUType { Float32, Float16 };

//! \brief A single user-selectable embedded scalar source (one combo entry).
/*! Identifies one column of one embedded data-per-vertex (dpv) or
 * data-per-streamline (dps) field carried inside the tractogram file. A
 * multi-column field expands into one descriptor per column (decision #4); a
 * single-column field yields exactly one. The descriptor is enumerated from the
 * format's FieldRegistry at load time without reading any sidecar values; the
 * column is read lazily only when the user selects this entry. */
struct EmbeddedScalarField {
  //! the field name as registered by the format handler
  std::string name;
  //! whether the field is per-streamline (dps) or per-vertex (dpv)
  MR::DWI::Tractography::FieldRole role;
  //! the field's role-local ordinal within the matching TractogramItem payload
  size_t ordinal;
  //! the field's total column count M (§2.2)
  size_t columns;
  //! the column of this field this descriptor selects (0 .. columns-1)
  size_t column;
  //! a human-readable label for the combo box: "name" or "name[column]"
  std::string label() const {
    if (columns <= 1)
      return name;
    return name + "[" + std::to_string(column) + "]";
  }
};

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
  //! \brief Load an external per-streamline scalar file for thickness modulation.
  /*! Only data-per-streamline (dps) data may modulate thickness, so a per-vertex
   * track scalar file (.tsf) is rejected; the file must be a plain text vector
   * holding one value per streamline (broadcast to all of its vertices). */
  void load_thickness_track_scalars(const std::filesystem::path &);
  //! \brief Load an embedded dpv/dps field column for streamline colouring.
  /*! \a entry indexes embedded_scalar_fields(); the field column is read lazily
   * from the file and uploaded as the intensity scalar (rendered identically to
   * an external scalar file). */
  void load_intensity_embedded_scalars(size_t entry);
  //! \brief Load an embedded dpv/dps field column for streamline thresholding.
  void load_threshold_embedded_scalars(size_t entry);
  //! \brief Load an embedded dpv/dps field column for streamline thickness modulation.
  /*! \a entry indexes embedded_scalar_fields(). Any per-vertex (dpv) or
   * per-streamline (dps) field column is permissible. */
  void load_thickness_embedded_scalars(size_t entry);
  //! \brief Point the threshold role at the colour role's scalar data (shared).
  /*! Used by the command-line tsf_thresh path: rather than reloading, the
   * threshold simply references the same scalar array already loaded for
   * colouring, so a single vertex array object backs both roles. */
  void use_intensity_data_for_threshold();
  //! \brief The embedded scalar fields (one entry per field column) available
  //!   in this tractogram, enumerated at load time.
  const std::vector<EmbeddedScalarField> &embedded_scalar_fields() const { return embedded_fields; }
  void erase_colour_data();
  void erase_intensity_scalar_data();
  void erase_threshold_scalar_data();
  void erase_thickness_scalar_data();

  void set_color_type(const TrackColourType);
  void set_threshold_type(const TrackThresholdType);
  void set_thickness_type(const TrackThicknessType);
  void set_geometry_type(const TrackGeometryType);
  TrackColourType get_color_type() const { return color_type; }
  TrackThresholdType get_threshold_type() const { return threshold_type; }
  TrackThicknessType get_thickness_type() const { return thickness_type; }
  TrackGeometryType get_geometry_type() const { return geometry_type; }

  float get_threshold_rate() const {
    switch (threshold_type) {
    case TrackThresholdType::None:
      return NaNF;
    case TrackThresholdType::SeparateFile:
      return (1e-3 * (threshold_max - threshold_min));
    }
    assert(0);
    return NaNF;
  }
  float get_threshold_min() const { return threshold_min; }
  float get_threshold_max() const { return threshold_max; }
  //! Step size for the thickness zero-offset spinbox, scaled to the data range.
  float get_thickness_offset_rate() const {
    const float range = thickness_max - thickness_min;
    return (std::isfinite(range) && range > 0.f) ? (1e-3f * range) : 0.1f;
  }

  static TrackGeometryType default_tract_geom;
  //! \brief When set, force half-precision GPU storage for any input.
  /*! Read once at tool init from the MRViewTractogramHalfPrecisionGPU config
   * flag; governs both vertex storage and floating-point sidecar (scalar)
   * storage. */
  static bool force_half_precision_gpu;
  static constexpr float default_line_thickness = 2e-3f;
  static constexpr float default_point_size = 4e-3f;

  bool scalarfile_by_direction;
  bool show_colour_bar;
  bool should_update_lod;
  float original_fov;
  float line_thickness;
  //! Sidecar value mapping to zero thickness; values below it render at zero
  //!   thickness, those above scale with the global thickness. Defaults to 0.
  float thickness_offset;
  //! Sidecar value span (above the offset) mapping to the unmodulated thickness;
  //!   auto-populated with the data mean on load so typical thickness is stable
  //!   across data of differing numerical range. Defaults to 1.
  float thickness_scale;
  //! When true, the (offset/scale-normalised) sidecar value is treated as a
  //!   cross-sectional area (pseudo-tubes) or volume (points), so the rendered
  //!   thickness scales as its square / cube root; when false the value sets the
  //!   thickness directly. Defaults to true.
  bool thickness_power_transform;
  //! Filesystem path of the external intensity scalar file, if one is loaded.
  std::filesystem::path intensity_scalar_path;
  //! Filesystem path of the external threshold scalar file, if one is loaded.
  std::filesystem::path threshold_scalar_path;
  //! Filesystem path of the external thickness-modulation scalar file, if loaded.
  std::filesystem::path thickness_scalar_path;
  //! \brief Index into embedded_scalar_fields() of the loaded intensity source,
  //!   or std::nullopt when the intensity scalar comes from an external file.
  std::optional<size_t> intensity_embedded_field;
  //! \brief Index into embedded_scalar_fields() of the loaded threshold source,
  //!   or std::nullopt when the threshold scalar comes from an external file.
  std::optional<size_t> threshold_embedded_field;
  //! \brief Index into embedded_scalar_fields() of the loaded thickness source,
  //!   or std::nullopt when the thickness scalar comes from an external file.
  std::optional<size_t> thickness_embedded_field;

  class Shader : public Displayable::Shader {
  public:
    Shader()
        : do_crop_to_slab(false),
          use_lighting(false),
          color_type(TrackColourType::Direction),
          threshold_type(TrackThresholdType::None),
          thickness_type(TrackThicknessType::None),
          thickness_power_transform(true),
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
    TrackThicknessType thickness_type;
    bool thickness_power_transform;
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
  TrackThicknessType thickness_type;
  TrackGeometryType geometry_type;

  // Instead of tracking the file path, pre-calculate the per-streamline
  //   endpoint-direction colour and store it; then, if colour by endpoint is
  //   requested, generate the GPU buffer by broadcasting these across the
  //   known track sizes. The colour is constant per streamline and the sign of
  //   the endpoint offset is discarded, so a quantised RGB8 triplet (3 bytes /
  //   streamline) is a lossless-enough and compact CPU representation.
  std::vector<std::array<uint8_t, 3>> endpoint_colours;

  // GPU vertex coordinate storage width, decided once at load.
  VertexGPUType vertex_gpu_type;
  //! \brief Whether vertex chunks carry in-band inter-streamline delimiter slots.
  /*! Set by the ".tck" raw-block fast path, which memcpy's the file's vertex
   * block verbatim (one NaN-delimiter slot after each streamline) rather than
   * copying real vertices contiguously. When set, the per-vertex side buffers
   * (endpoint colour, intensity/threshold scalars) insert one padding slot
   * after each streamline so their slot indices stay aligned with the position
   * buffer; the padding slots are never referenced by the element buffers. */
  bool vertices_have_gaps;
  //! \brief OpenGL element type enum for the chosen vertex width.
  GLenum vertex_gl_type() const { return vertex_gpu_type == VertexGPUType::Float16 ? gl::HALF_FLOAT : gl::FLOAT; }
  //! \brief Byte size of one 3-vector vertex at the chosen width (12 for fp32, 6 for fp16).
  GLsizei vertex_element_bytes() const {
    return static_cast<GLsizei>(vertex_gpu_type == VertexGPUType::Float16 ? 3 * sizeof(Eigen::half)
                                                                          : 3 * sizeof(float));
  }

  // GPU scalar (intensity/threshold) storage width, decided once at load from
  //   the half-precision config flag (shared with the vertex width policy).
  ScalarGPUType scalar_gpu_type;
  //! \brief OpenGL element type enum for the chosen scalar width.
  GLenum scalar_gl_type() const { return scalar_gpu_type == ScalarGPUType::Float16 ? gl::HALF_FLOAT : gl::FLOAT; }

  std::vector<GLuint> vertex_buffers;
  std::vector<GLuint> vertex_array_objects;
  // Per-vertex TrackVertexType classification (uint8_t), one buffer per chunk
  std::vector<GLuint> vertex_type_buffers;
  std::vector<GLuint> colour_buffers;

  //! \brief A per-vertex scalar array resident on the GPU (one VBO per chunk).
  /*! Shared, via std::shared_ptr, by whichever of the colour / threshold /
   * thickness roles select the same sidecar source, so identical data is uploaded
   * once rather than into independent vertex array objects (the role's source
   * identity is tracked separately on the Tractogram). The GL buffers are released
   * when the last referencing role drops the array. */
  struct ScalarArray {
    std::vector<GLuint> buffers; //!< one VBO per vertex chunk, aligned to vertex_buffers
    float value_min{NaNF};       //!< minimum raw value across the array
    float value_max{NaNF};       //!< maximum raw value across the array
    //! Mean over the source elements (per vertex for dpv, per streamline for dps);
    //!   thickness modulation auto-populates its "scale" with this so the typical
    //!   thickness is suitable irrespective of the data's numerical range.
    float value_mean{NaNF};
    ~ScalarArray();
  };
  //! Scalar arrays backing the three roles; two roles share one array when they
  //!   select the same source (compared via the role identity members above).
  std::shared_ptr<ScalarArray> intensity_scalars;
  std::shared_ptr<ScalarArray> threshold_scalars;
  std::shared_ptr<ScalarArray> thickness_scalars;

  // Embedded dpv/dps fields available in this tractogram (one entry per field
  //   column), enumerated from the format registry at load time.
  std::vector<EmbeddedScalarField> embedded_fields;
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
  // Value range of the loaded thickness-modulation scalar (informational; the
  //   shader interprets the raw value directly as a volume / cross-sectional
  //   area multiplier).
  float thickness_min, thickness_max;

  void load_tracks_onto_GPU(std::vector<Eigen::Vector3f> &buffer,
                            std::vector<GLint> &starts,
                            std::vector<GLint> &sizes,
                            size_t &tck_count);

  //! \brief Ask each fast-path-capable format handler for a VertexBlockSource.
  /*! Resolves the file's format handler and, for the four formats that expose a
   * raw vertex block (".tck", ".vtk", ".vtx", TRX), runs that handler's
   * applicability gate (datatype, encoding, endianness, contiguity). On a pass
   * it returns a populated VertexBlockSource describing the block, its on-disk
   * datatype / byte order and its streamline-boundary mechanism (plus the
   * backing that keeps the block alive); on any unmet condition it returns
   * std::nullopt so load_tracks() runs the generic per-streamline loader. This
   * is the single contract that replaced the four bespoke fast-path accessors. */
  std::optional<VertexBlockSource> resolve_vertex_block_source();

  //! \brief Build the vertex chunks directly from a raw VertexBlockSource.
  /*! The single fast-path entry point shared by all four formats. It commits the
   * GPU storage widths exactly as the generic path would, establishes the
   * per-streamline boundaries per \a source.boundary (NaN scan / offsets / lines
   * → starts[] and sizes[]), groups whole streamlines into ≤ MAX_BUFFER_SIZE
   * chunks (inclusive of the 2 VBO sentinels), and uploads each chunk: an in-band
   * NaN-delimited block is memcpy'd verbatim (with one delimiter slot per
   * streamline, so vertices_have_gaps is set), while an offsets/lines block is
   * decoded coordinate by coordinate to native-order float (with optional staging
   * byte-swap) into a gap-free chunk. The per-LOD element buffers skip the
   * delimiter slots iff source.boundary == NaNDelimiter. Returns whether any
   * chunk was uploaded; on an empty / degenerate block it returns false and
   * leaves the generic (empty-tolerant) path to take over. */
  bool load_tracks_fast(const VertexBlockSource &source);

  //! \brief Upload one fast-path vertex chunk to the GPU and register its topology.
  /*! \a chunk_bytes points at the chunk's \a num_slots contiguous on-disk vertex
   * slots (each \a source.element_datatype-wide). When \a source.boundary ==
   * NaNDelimiter the slots are memcpy'd verbatim (in-band delimiter slots ride
   * along, never indexed); otherwise each coordinate is decoded to native-order
   * float in the active GPU width. The block is bracketed by two sentinels
   * (copies of the chunk's first / last real vertex), and the shared topology
   * builder is driven from \a starts / \a sizes (chunk-local slot indices and
   * real per-streamline vertex counts). */
  void upload_chunk_fastpath(const VertexBlockSource &source,
                             const std::byte *chunk_bytes,
                             GLint num_slots,
                             const Eigen::Vector3f &front_sentinel,
                             const Eigen::Vector3f &back_sentinel,
                             const std::vector<GLint> &starts,
                             const std::vector<GLint> &sizes,
                             const std::vector<Eigen::Vector3f> &staged_vertices = {});

  //! \brief Build the class buffer, element buffers and register a chunk.
  /*! Shared by the generic and fast-path uploaders once the position VBO is in
   * place. \a num_slots is the number of buffer slots (real vertices plus any
   * in-band delimiter slots); \a starts are slot indices and \a sizes the real
   * per-streamline vertex counts. */
  void build_chunk_topology(GLuint vertex_array_object,
                            GLuint vertexbuffer,
                            GLint num_slots,
                            const std::vector<GLint> &starts,
                            const std::vector<GLint> &sizes,
                            size_t tck_count);

  void load_end_colours_onto_GPU(std::vector<uint8_t> &);

  //! \brief Append one chunk's staged scalars to \a array as a fresh VBO.
  /*! Honours the half-precision flag; \a buffer and \a tck_count are consumed. */
  void upload_scalar_chunk(ScalarArray &array, std::vector<float> &buffer, size_t &tck_count);

  //! \brief Return an already-loaded role array matching an external file source.
  std::shared_ptr<ScalarArray> find_loaded_scalar_array(const std::filesystem::path &) const;
  //! \brief Return an already-loaded role array matching an embedded field column.
  std::shared_ptr<ScalarArray> find_loaded_scalar_array(size_t entry) const;

  //! \brief Reuse a matching loaded array or read a fresh one from a file source.
  std::shared_ptr<ScalarArray> acquire_file_scalar_array(const std::filesystem::path &);
  //! \brief Reuse a matching loaded array or read a fresh one from an embedded column.
  std::shared_ptr<ScalarArray> acquire_embedded_scalar_array(size_t entry);

  //! \brief Read a scalar array from an external file (.tsf per-vertex, or a plain
  //!   text vector with one value per streamline broadcast to its vertices).
  std::shared_ptr<ScalarArray> read_file_scalar_array(const std::filesystem::path &);
  //! \brief Read a scalar array from an embedded dpv/dps field column.
  /*! Re-opens the tractogram through the generic loader, reads the requested field
   * via the TractogramItem overload, extracts the selected column (dps values
   * broadcast to every vertex of their streamline), and uploads one scalar per
   * real vertex, chunk-aligned to the vertex buffers. */
  std::shared_ptr<ScalarArray> read_embedded_scalar_array(const EmbeddedScalarField &field);

  //! \brief Set thickness_scale from the loaded thickness data's mean (or a fallback).
  void auto_populate_thickness_scale();

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
