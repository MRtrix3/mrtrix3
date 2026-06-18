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

#include "mrview/tool/tractography/tractogram.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

#include "datatype.h"
#include "dwi/tractography/field_registry.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/formats/base.h"
#include "dwi/tractography/formats/list.h"
#include "dwi/tractography/formats/tck.h"
#include "dwi/tractography/formats/trx.h"
#include "dwi/tractography/formats/tsf.h"
#include "dwi/tractography/formats/vtk.h"
#include "dwi/tractography/formats/vtx.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/sidecar_value.h"
#include "dwi/tractography/tractogram_item.h"
#include "dwi/tractography/validate.h"
#include "file/entry.h"
#include "file/matrix.h"
#include "file/mmap.h"
#include "half.h"
#include "mrview/mode/base.h"
#include "mrview/window.h"
#include "opengl/gl_core_3_3.h"
#include "opengl/lighting.h"
#include "progressbar.h"
#include "projection.h"
#include "raw.h"

const size_t MAX_BUFFER_SIZE = 2796200;                      // number of points to fill 32MB
constexpr uint32_t PRIMITIVE_RESTART_SENTINEL = 0xFFFFFFFFu; // Primitive restart index for UNSIGNED_INT

namespace MR::GUI::MRView::Tool {
TrackGeometryType Tractogram::default_tract_geom(TrackGeometryType::Pseudotubes);
bool Tractogram::force_half_precision_gpu(false);

std::string Tractogram::Shader::vertex_shader_source(const Displayable &displayable) {
  const Tractogram &tractogram = dynamic_cast<const Tractogram &>(displayable);

  std::string source = "layout (location = 0) in vec3 vertex;\n"
                       "layout (location = 1) in vec3 prev_vertex;\n"
                       "layout (location = 2) in vec3 next_vertex;\n"
                       "layout (location = 5) in uint vertex_class;\n";

  if (color_type == TrackColourType::Ends)
    source += "layout (location = 3) in vec3 end_colour;\n";
  else if (color_type == TrackColourType::ScalarFile)
    source += "layout (location = 3) in float amp;\n";

  if (threshold_type == TrackThresholdType::SeparateFile)
    source += "layout (location = 4) in float thresh_amp;\n";

  source += "uniform mat4 MVP;\n"
            "uniform float line_thickness;\n"

            // Uniforms won't be included in compiled shader if not referenced
            // so we can unconditionally list all of them
            "uniform vec3 screen_normal;\n"
            "uniform float crop_var;\n"
            "uniform float slab_width;\n"
            "uniform float offset, scale;\n"
            "uniform float scale_x, scale_y;\n"
            "uniform vec3 colourmap_colour;\n"

            "out vec3 v_tangent;\n"
            "out vec2 v_end;\n";

  if (do_crop_to_slab)
    source += "out float v_include;\n";

  if (threshold_type != TrackThresholdType::None)
    source += "out float v_amp;\n";

  if (color_type == TrackColourType::Ends || color_type == TrackColourType::ScalarFile)
    source += "out vec3 v_colour;\n";

  // Main function
  source += "void main() {\n"
            "  gl_Position = MVP * vec4(vertex, 1);\n"
            // Derive the local tangent from neighbouring vertices; the
            // per-vertex classification selects which neighbours are valid,
            // so no duplicate endpoint padding is required
            // (TrackVertexType: 0=Single, 1=First, 2=Middle, 3=Last)
            "  if (vertex_class == 1u)\n"
            "    v_tangent = next_vertex - vertex;\n"
            "  else if (vertex_class == 3u)\n"
            "    v_tangent = vertex - prev_vertex;\n"
            "  else if (vertex_class == 2u)\n"
            "    v_tangent = next_vertex - prev_vertex;\n"
            "  else\n"
            "    v_tangent = vec3 (0.0);\n"
            "  vec2 dir = mat3x2(MVP) * v_tangent;\n"
            // A single-vertex streamline has no tangent: emit a zero offset
            // rather than normalising a zero vector (which would be NaN)
            "  if (dot (dir, dir) > 0.0) {\n"
            "    v_end = line_thickness * normalize (vec2 (dir.y/scale_x, -dir.x/scale_y));\n"
            "    v_end.x *= scale_y; v_end.y *= scale_x;\n"
            "  } else {\n"
            "    v_end = vec2 (0.0);\n"
            "  }\n";

  if (do_crop_to_slab)
    source += "  v_include = (dot(vertex, screen_normal) - crop_var) / slab_width;\n";

  if (threshold_type == TrackThresholdType::UseColourFile)
    source += "  v_amp = amp;\n";
  else if (threshold_type == TrackThresholdType::SeparateFile)
    source += "  v_amp = thresh_amp;\n";

  if (color_type == TrackColourType::Ends)
    source += "  v_colour = end_colour;\n";
  else if (color_type == TrackColourType::ScalarFile) { // TODO: move to frag shader:
    if (!ColourMap::maps[colourmap].special) {
      source += "  float amplitude = clamp (";
      if (tractogram.scale_inverted())
        source += "1.0 -";
      source += " scale * (amp - offset), 0.0, 1.0);\n";
    }
    source += std::string("  vec3 color;\n  ") + ColourMap::maps[colourmap].glsl_mapping + "  v_colour = color;\n";
  }

  source += "}\n";

  return source;
}

std::string Tractogram::Shader::geometry_shader_source(const Displayable &) {
  if (geometry_type != TrackGeometryType::Pseudotubes)
    return std::string();

  std::string source = "layout(lines) in;\n"
                       "layout(triangle_strip, max_vertices = 4) out;\n"
                       "uniform float line_thickness;\n"
                       "uniform float downscale_factor;\n"
                       "uniform mat4 MV;\n"

                       "in vec3 v_tangent[];\n" // check_syntax off
                       "in vec2 v_end[];\n";    // check_syntax off

  if (threshold_type != TrackThresholdType::None)
    source += "in float v_amp[];\n" // check_syntax off
              "out float g_amp;\n";

  if (do_crop_to_slab)
    source += "in float v_include[];\n" // check_syntax off
              "out float g_include;\n";

  if (use_lighting || color_type == TrackColourType::Direction)
    source += "out vec3 g_tangent;\n";

  if (color_type == TrackColourType::ScalarFile || color_type == TrackColourType::Ends)
    source += "in vec3 v_colour[];\n" // check_syntax off
              "out vec3 fColour;\n";

  if (use_lighting)
    source += "const float PI = " + str(Math::pi) +
              ";\n"
              "out float g_height;\n";

  source += "void main() {\n";

  if (do_crop_to_slab)
    source += "  if (v_include[0] < 0.0 && v_include[1] < 0.0) return;\n"
              "  if (v_include[0] > 1.0 && v_include[1] > 1.0) return;\n";

  // First vertex:
  if (use_lighting || color_type == TrackColourType::Direction)
    source += "  g_tangent = v_tangent[0];\n";
  if (do_crop_to_slab)
    source += "  g_include = v_include[0];\n";
  if (threshold_type != TrackThresholdType::None)
    source += "  g_amp = v_amp[0];\n";
  if (color_type == TrackColourType::ScalarFile || color_type == TrackColourType::Ends)
    source += "  fColour = v_colour[0];\n";

  if (use_lighting)
    source += "  g_height = 0.0;\n";
  source += "  gl_Position = gl_in[0].gl_Position - vec4(v_end[0],0,0);\n"
            "  EmitVertex();\n";

  if (use_lighting)
    source += "  g_height = PI;\n";
  source += "  gl_Position = gl_in[0].gl_Position + vec4(v_end[0],0,0);\n"
            "  EmitVertex();\n";

  // Second vertex:
  if (use_lighting || color_type == TrackColourType::Direction)
    source += "  g_tangent = v_tangent[1];\n";
  if (do_crop_to_slab)
    source += "  g_include = v_include[1];\n";
  if (threshold_type != TrackThresholdType::None)
    source += "  g_amp = v_amp[1];\n";
  if (color_type == TrackColourType::ScalarFile || color_type == TrackColourType::Ends)
    source += "  fColour = v_colour[1];\n";

  if (use_lighting)
    source += "  g_height = 0.0;\n";
  source += "  gl_Position = gl_in[1].gl_Position - vec4 (v_end[1],0,0);\n"
            "  EmitVertex();\n";

  if (use_lighting)
    source += "  g_height = PI;\n";
  source += "  gl_Position = gl_in[1].gl_Position + vec4 (v_end[1],0,0);\n"
            "  EmitVertex();\n"
            "}\n";

  return source;
}

std::string Tractogram::Shader::fragment_shader_source(const Displayable &displayable) {
  const Tractogram &tractogram = dynamic_cast<const Tractogram &>(displayable);
  bool using_geom = geometry_type == TrackGeometryType::Pseudotubes;
  bool using_points = geometry_type == TrackGeometryType::Points;

  std::string source = "uniform float lower, upper;\n"
                       "uniform vec3 colourmap_colour;\n"
                       "uniform mat4 MV;\n"
                       "out vec3 colour;\n";

  if (color_type == TrackColourType::ScalarFile || color_type == TrackColourType::Ends)
    source += using_geom ? "in vec3 fColour;\n" : "in vec3 v_colour;\n";
  if (use_lighting || color_type == TrackColourType::Direction)
    source += using_geom ? "in vec3 g_tangent;\n" : "in vec3 v_tangent;\n";

  if (threshold_type != TrackThresholdType::None)
    source += using_geom ? "in float g_amp;\n" : "in float v_amp;\n";

  if (use_lighting && (using_geom || using_points)) {
    source += "uniform float ambient, diffuse, specular, shine;\n"
              "uniform vec3 light_pos;\n";

    if (using_geom)
      source += "in float g_height;\n";
  }

  if (do_crop_to_slab)
    source += using_geom ? "in float g_include;\n" : "in float v_include;\n";

  source += "void main() {\n";

  if (using_points)
    source += "vec2 pos = gl_PointCoord-0.5;\n"
              "float d_pos = dot(pos, pos);\n"
              "if(d_pos >0.25)\n"
              "  discard;\n";

  if (do_crop_to_slab)
    source += using_geom ? "  if (g_include < 0.0 || g_include > 1.0) discard;\n"
                         : "  if (v_include < 0.0 || v_include > 1.0) discard;\n";

  if (threshold_type != TrackThresholdType::None) {
    if (tractogram.use_discard_lower())
      source += using_geom ? "  if (g_amp < lower) discard;\n" : "  if (v_amp < lower) discard;\n";
    if (tractogram.use_discard_upper())
      source += using_geom ? "  if (g_amp > upper) discard;\n" : "  if (v_amp > upper) discard;\n";
  }

  switch (color_type) {
  case TrackColourType::Direction:
    // Guard against a zero tangent (single-vertex streamline) to avoid NaN
    source += using_geom ? "  colour = dot (g_tangent, g_tangent) > 0.0 ? abs (normalize (g_tangent)) : vec3 (0.0);\n"
                         : "  colour = dot (v_tangent, v_tangent) > 0.0 ? abs (normalize (v_tangent)) : vec3 (0.0);\n";
    break;
  case TrackColourType::ScalarFile:
    [[fallthrough]];
  case TrackColourType::Ends:
    source += using_geom ? "  colour = fColour;\n" : "  colour = v_colour;\n";
    break;
  case TrackColourType::Manual:
    source += "  colour = colourmap_colour;\n";
  }

  if (use_lighting && (using_geom || using_points)) {

    if (using_geom) {
      // g_height tells us where we are across the cylinder (0 - PI)
      source +=
          // compute surface normal:
          "  float s = sin (g_height);\n"
          "  float c = cos (g_height);\n"
          "  vec3 tangent = normalize (mat3(MV) * g_tangent);\n"
          "  vec3 in_plane_x = normalize (vec3(-tangent.y, tangent.x, 0.0f));\n"
          "  vec3 in_plane_y = normalize (vec3(-tangent.x, -tangent.y, 0.0f));\n"
          "  vec3 surface_normal = c*in_plane_x +  s*abs(tangent.z)*in_plane_y;\n"
          "  surface_normal.z -= s * sqrt(tangent.x*tangent.x + tangent.y*tangent.y);\n";
    } else if (using_points) {
      source += "vec3 surface_normal = normalize(vec3(pos, sin((d_pos - 0.25) *" + str(Math::pi_2) + ")));\n";
    }

    source += "  float light_dot_surfaceN = -dot(light_pos, surface_normal);"
              // Ambient and diffuse component
              "  colour *= ambient + diffuse * clamp(light_dot_surfaceN, 0, 1);\n"

              // Specular component
              "  if (light_dot_surfaceN > 0.0) {\n"
              "    vec3 reflection = light_pos + 2 * light_dot_surfaceN * surface_normal;\n"
              "    colour += specular * pow(clamp(-reflection.z, 0, 1), shine);\n"
              "  }\n";
  }

  source += "}\n";

  return source;
}

bool Tractogram::Shader::need_update(const Displayable &object) const {
  const Tractogram &tractogram(dynamic_cast<const Tractogram &>(object));
  if (do_crop_to_slab != tractogram.tractography_tool.crop_to_slab())
    return true;
  if (color_type != tractogram.color_type)
    return true;
  if (threshold_type != tractogram.threshold_type)
    return true;
  if (use_lighting != tractogram.tractography_tool.use_lighting)
    return true;
  if (geometry_type != tractogram.geometry_type)
    return true;

  return Displayable::Shader::need_update(object);
}

void Tractogram::Shader::update(const Displayable &object) {
  const Tractogram &tractogram(dynamic_cast<const Tractogram &>(object));
  do_crop_to_slab = tractogram.tractography_tool.crop_to_slab();
  use_lighting = tractogram.tractography_tool.use_lighting;
  color_type = tractogram.color_type;
  threshold_type = tractogram.threshold_type;
  geometry_type = tractogram.geometry_type;
  Displayable::Shader::update(object);
}

Tractogram::Tractogram(Tractography &tool, const std::filesystem::path &filepath)
    : Displayable(filepath),
      show_colour_bar(true),
      original_fov(NaNF),
      line_thickness(0.f),
      tractography_tool(tool),
      filepath(filepath),
      color_type(TrackColourType::Direction),
      threshold_type(TrackThresholdType::None),
      geometry_type(default_tract_geom),
      vertex_gpu_type(VertexGPUType::Float32),
      vertices_have_gaps(false),
      scalar_gpu_type(ScalarGPUType::Float32),
      lod_level(0),
      ebo_dirty(false),
      vao_dirty(true),
      threshold_min(NaNF),
      threshold_max(NaNF) {
  set_allowed_features(true, true, true);
  colourmap = 1;
  connect(&window(), SIGNAL(fieldOfViewChanged()), this, SLOT(on_FOV_changed()));
  on_FOV_changed();
}

Tractogram::~Tractogram() {
  GL::assert_context_is_current();
  if (!vertex_buffers.empty())
    gl::DeleteBuffers(vertex_buffers.size(), &vertex_buffers[0]);
  if (!vertex_array_objects.empty())
    gl::DeleteVertexArrays(vertex_array_objects.size(), &vertex_array_objects[0]);
  if (!vertex_type_buffers.empty())
    gl::DeleteBuffers(vertex_type_buffers.size(), &vertex_type_buffers[0]);
  if (!colour_buffers.empty())
    gl::DeleteBuffers(colour_buffers.size(), &colour_buffers[0]);
  if (!intensity_scalar_buffers.empty())
    gl::DeleteBuffers(intensity_scalar_buffers.size(), &intensity_scalar_buffers[0]);
  if (!threshold_scalar_buffers.empty())
    gl::DeleteBuffers(threshold_scalar_buffers.size(), &threshold_scalar_buffers[0]);
  if (!element_buffers.empty())
    gl::DeleteBuffers(element_buffers.size(), &element_buffers[0]);
  GL::assert_context_is_current();
}

void Tractogram::render(const Projection &transform) {
  GL::assert_context_is_current();
  if (tractography_tool.do_crop_to_slab && tractography_tool.slab_thickness <= 0.0)
    return;

  start(track_shader);
  transform.set(track_shader);

  if (tractography_tool.do_crop_to_slab) {
    gl::Uniform3fv(gl::GetUniformLocation(track_shader, "screen_normal"), 1, transform.screen_normal().data());
    gl::Uniform1f(gl::GetUniformLocation(track_shader, "crop_var"),
                  window().focus().dot(transform.screen_normal()) - tractography_tool.slab_thickness / 2);
    gl::Uniform1f(gl::GetUniformLocation(track_shader, "slab_width"), tractography_tool.slab_thickness);
  }

  if (threshold_type != TrackThresholdType::None) {
    if (use_discard_lower())
      gl::Uniform1f(gl::GetUniformLocation(track_shader, "lower"), lessthan);
    if (use_discard_upper())
      gl::Uniform1f(gl::GetUniformLocation(track_shader, "upper"), greaterthan);
  }

  if (color_type == TrackColourType::Manual)
    gl::Uniform3f(gl::GetUniformLocation(track_shader, "colourmap_colour"),
                  colour[0] / 255.0,
                  colour[1] / 255.0,
                  colour[2] / 255.0);

  if (color_type == TrackColourType::ScalarFile) {
    gl::Uniform1f(gl::GetUniformLocation(track_shader, "offset"), display_midpoint - 0.5f * display_range);
    gl::Uniform1f(gl::GetUniformLocation(track_shader, "scale"), 1.0 / display_range);
  }

  if (tractography_tool.use_lighting) {
    gl::UniformMatrix4fv(gl::GetUniformLocation(track_shader, "MV"), 1, gl::FALSE_, transform.modelview());
    gl::Uniform3fv(gl::GetUniformLocation(track_shader, "light_pos"), 1, tractography_tool.lighting->lightpos.data());
    gl::Uniform1f(gl::GetUniformLocation(track_shader, "ambient"), tractography_tool.lighting->ambient);
    gl::Uniform1f(gl::GetUniformLocation(track_shader, "diffuse"), tractography_tool.lighting->diffuse);
    gl::Uniform1f(gl::GetUniformLocation(track_shader, "specular"), tractography_tool.lighting->specular);
    gl::Uniform1f(gl::GetUniformLocation(track_shader, "shine"), tractography_tool.lighting->shine);
  }

  if (!std::isfinite(original_fov)) {
    // set line thickness once upon loading, but don't touch it after that:
    // it shouldn't change when the background image changes
    const std::array<default_type, 3> dim = {
        window().image()->header().size(0) * window().image()->header().spacing(0),  //
        window().image()->header().size(1) * window().image()->header().spacing(1),  //
        window().image()->header().size(2) * window().image()->header().spacing(2)}; //
    original_fov = std::pow(dim[0] * dim[1] * dim[2], 1.0F / 3.0F);
  }

  const ScreenMetrics metrics = screen_metrics(transform);

  // The vertex and geometry shaders scale the line_thickness uniform back up by
  //   (width * height) / 2 (the v_end aspect-ratio correction), so the uniform that
  //   yields a tube of tube_width_px on screen is tube_width_px / (width * height).
  gl::Uniform1f(gl::GetUniformLocation(track_shader, "line_thickness"),
                metrics.tube_width_px / (transform.width() * transform.height()));
  gl::Uniform1f(gl::GetUniformLocation(track_shader, "scale_x"), transform.width());
  gl::Uniform1f(gl::GetUniformLocation(track_shader, "scale_y"), transform.height());

  glPointSize(metrics.point_diameter_px);

  if (tractography_tool.line_opacity < 1.0) {
    gl::Enable(gl::BLEND);
    gl::BlendEquation(gl::FUNC_ADD);
    gl::BlendFunc(gl::CONSTANT_ALPHA, gl::ONE);
    gl::Disable(gl::DEPTH_TEST);
    gl::DepthMask(gl::TRUE_);
    gl::BlendColor(1.0, 1.0, 1.0, tractography_tool.line_opacity / 0.5);
    render_streamlines(metrics);
    gl::BlendFunc(gl::CONSTANT_ALPHA, gl::ONE_MINUS_CONSTANT_ALPHA);
    gl::Enable(gl::DEPTH_TEST);
    gl::DepthMask(gl::TRUE_);
    gl::BlendColor(1.0, 1.0, 1.0, tractography_tool.line_opacity / 0.5);
    render_streamlines(metrics);

  } else {
    gl::Disable(gl::BLEND);
    gl::Enable(gl::DEPTH_TEST);
    gl::DepthMask(gl::TRUE_);
    render_streamlines(metrics);
  }

  if (tractography_tool.line_opacity < 1.0) {
    gl::Disable(gl::BLEND);
    gl::Enable(gl::DEPTH_TEST);
    gl::DepthMask(gl::TRUE_);
  }

  stop(track_shader);
  GL::assert_context_is_current();
}

Tractogram::ScreenMetrics Tractogram::screen_metrics(const Projection &transform) const {
  // Orthographic projection (mode/base.cpp): the visible world extent is
  //   2 * width * FOV / (width + height) horizontally (and the analogous vertical),
  //   so the on-screen scale is isotropic at (width + height) / (2 * FOV) pixels/mm.
  const float pixels_per_mm = (transform.width() + transform.height()) / (2.0f * window().FOV());
  // Tube width and point diameter are fixed in world space (proportional to the
  //   image extent frozen at load via original_fov, modulated by the thickness
  //   slider); convert to their on-screen pixel sizes at the current zoom.
  const float world_scale = std::exp(2.0e-3f * line_thickness) * original_fov;
  return {pixels_per_mm,
          Tractogram::default_line_thickness * world_scale * pixels_per_mm,
          Tractogram::default_point_size * world_scale * pixels_per_mm};
}

inline void Tractogram::render_streamlines(const ScreenMetrics &metrics) {
  GL::assert_context_is_current();

  if (should_update_lod)
    update_lod(metrics);
  if (ebo_dirty)
    update_element_buffers();

  const GLenum mode = geometry_type == TrackGeometryType::Points ? gl::POINTS : gl::LINE_STRIP;

  for (size_t buf = 0, N = vertex_buffers.size(); buf < N; ++buf) {
    gl::BindVertexArray(vertex_array_objects[buf]);

    if (vao_dirty) {

      // Attribute 3: per-vertex colour (Ends) or intensity scalar (ScalarFile).
      //   Side buffers carry one element per vertex with no padding, so they
      //   are addressed directly by the element index (tightly packed, offset 0)
      switch (color_type) {
      case TrackColourType::Ends:
        gl::BindBuffer(gl::ARRAY_BUFFER, colour_buffers[buf]);
        gl::EnableVertexAttribArray(3);
        // RGB8 packed; normalized = GL_TRUE so the shader receives a vec3 in
        //   [0,1] (without it the shader would see raw 0-255 values).
        gl::VertexAttribPointer(3, 3, gl::UNSIGNED_BYTE, gl::TRUE_, 0, nullptr);
        break;
      case TrackColourType::ScalarFile:
        gl::BindBuffer(gl::ARRAY_BUFFER, intensity_scalar_buffers[buf]);
        gl::EnableVertexAttribArray(3);
        // Half-precision scalars are promoted to float on fetch (config flag);
        //   only the attribute type enum changes between fp32 and fp16.
        gl::VertexAttribPointer(3, 1, scalar_gl_type(), gl::FALSE_, 0, nullptr);
        break;
      default:
        break;
      }

      if (threshold_type == TrackThresholdType::SeparateFile) {
        gl::BindBuffer(gl::ARRAY_BUFFER, threshold_scalar_buffers[buf]);
        gl::EnableVertexAttribArray(4);
        gl::VertexAttribPointer(4, 1, scalar_gl_type(), gl::FALSE_, 0, nullptr);
      }

      // Position attributes prev / curr / next all read from the one vertex
      //   buffer, offset by one vertex each (the "offset trick"). The leading
      //   and trailing sentinel vertices keep the chunk-boundary fetches in
      //   bounds. Stride is fixed at one vertex regardless of sub-sampling
      //   level (sub-sampling is encoded entirely in the element buffers).
      const GLsizei vertex_stride = vertex_element_bytes();
      const GLenum vertex_type = vertex_gl_type();
      gl::BindBuffer(gl::ARRAY_BUFFER, vertex_buffers[buf]);
      gl::EnableVertexAttribArray(0); // curr
      gl::VertexAttribPointer(0, 3, vertex_type, gl::FALSE_, vertex_stride, reinterpret_cast<void *>(vertex_stride));
      gl::EnableVertexAttribArray(1); // prev
      gl::VertexAttribPointer(1, 3, vertex_type, gl::FALSE_, vertex_stride, nullptr);
      gl::EnableVertexAttribArray(2); // next
      gl::VertexAttribPointer(
          2, 3, vertex_type, gl::FALSE_, vertex_stride, reinterpret_cast<void *>(2 * vertex_stride));

      // Attribute 5: per-vertex TrackVertexType classification (integer)
      gl::BindBuffer(gl::ARRAY_BUFFER, vertex_type_buffers[buf]);
      gl::EnableVertexAttribArray(5);
      gl::VertexAttribIPointer(5, 1, gl::UNSIGNED_BYTE, static_cast<GLsizei>(sizeof(uint8_t)), nullptr);
    }

    if (element_counts[buf] > 0) {
      // The element buffer (resident in VAO state) draws the active
      //   sub-sampling level for both points and lines; primitive restart is
      //   kept enabled for points too so the separators are consumed as
      //   no-ops rather than dereferenced
      gl::Enable(gl::PRIMITIVE_RESTART);
      gl::PrimitiveRestartIndex(PRIMITIVE_RESTART_SENTINEL);
      gl::DrawElements(mode, element_counts[buf], gl::UNSIGNED_INT, nullptr);
      gl::Disable(gl::PRIMITIVE_RESTART);
    }
  }

  vao_dirty = false;
  GL::assert_context_is_current();
}

inline void Tractogram::update_lod(const ScreenMetrics &metrics) {
  // Note: if streamlines have been resampled, the stored step size (and hence
  //   the chosen level) may not be representative; if it is unavailable (variable
  //   step), no reasoning about on-screen spacing is possible and the level stays 0.
  const float step_size = properties.get_stepsize();
  size_t new_level = 0;

  if (std::isfinite(step_size) && step_size > 0.0f) {
    // On-screen distance between consecutive samples at the current zoom. This is
    //   an upper bound on the true projected spacing (a streamline angled towards
    //   the view normal projects shorter), so the resulting level is conservative.
    const float vertex_spacing_px = step_size * metrics.pixels_per_mm;
    // Per-geometry on-screen feature below which denser sampling is redundant:
    //   pseudotubes produce banner-edge discontinuities when a segment is shorter
    //   than the tube is wide; point disks overlap when spaced below their diameter;
    //   lines have no width feature and rely solely on the sub-pixel floor.
    float feature_px = 0.0f;
    switch (geometry_type) {
    case TrackGeometryType::Pseudotubes:
      feature_px = metrics.tube_width_px;
      break;
    case TrackGeometryType::Points:
      feature_px = metrics.point_diameter_px;
      break;
    case TrackGeometryType::Lines:
      feature_px = 0.0f;
      break;
    }
    const float target_spacing_px = std::max(feature_px, min_vertex_spacing_px);
    const float ratio = target_spacing_px / vertex_spacing_px;
    // Geometric levels: pick the smallest level whose ratio (1 << level) is at least
    //   the desired ratio, so the effective on-screen spacing meets the target.
    if (ratio > 1.0f)
      new_level = std::min(num_lod_levels - 1, static_cast<size_t>(std::ceil(std::log2(ratio))));
    DEBUG("Proposed subsampling for geometry " + str(static_cast<int>(geometry_type)) + ":" + //
          " line thickness " + str(line_thickness) + "," +                                    //
          " pixels/mm " + str(metrics.pixels_per_mm) + "," +                                  //
          " feature " + str(feature_px) + "px," +                                             //
          " vertex spacing " + str(vertex_spacing_px) + "px" +                                //
          " -> ratio " + str(ratio) + " -> level " + str(new_level) +                         //
          " (stride " + str(lod_ratio_for_level(new_level)) + ")");
  }

  if (new_level != lod_level) {
    DEBUG("Changing tractogram LOD to level " + str(new_level) + " (stride " + str(lod_ratio_for_level(new_level)) +
          ")");
    lod_level = new_level;
    ebo_dirty = true;
  }

  should_update_lod = false;
}

inline void Tractogram::update_element_buffers() {
  GL::assert_context_is_current();
  for (size_t buf = 0, N = element_buffers.size(); buf < N; ++buf) {
    const std::vector<uint32_t> &indices = element_indices[buf][lod_level];
    // Bind the VAO first so the element-array binding is recorded as VAO state,
    //   then replace the resident buffer contents with the active level
    gl::BindVertexArray(vertex_array_objects[buf]);
    gl::BindBuffer(gl::ELEMENT_ARRAY_BUFFER, element_buffers[buf]);
    gl::BufferData(gl::ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), gl::STATIC_DRAW);
    element_counts[buf] = static_cast<GLsizei>(indices.size());
  }
  ebo_dirty = false;
  GL::assert_context_is_current();
}

void Tractogram::load_tracks() {
  // Make sure to set graphics context!
  // We're setting up vertex array objects
  GL::Context::Grab context;
  GL::assert_context_is_current();

  on_FOV_changed();

  // Fast path: for the four formats that expose a contiguous on-disk vertex
  //   block (".tck", ".vtk", ".vtx", TRX), ask the format handler for a
  //   VertexBlockSource and, if one is produced (its applicability gate passing),
  //   build the vertex chunks directly from the raw block — bypassing the
  //   per-streamline streaming reader. A single entry point serves all four; the
  //   boundary mechanism carried in the descriptor selects the per-format
  //   behaviour (NaN-delimiter scan with in-band gaps vs offsets/lines-driven
  //   contiguous staging). Falls back to the generic path below when no handler
  //   serves the file by raw block (std::nullopt) or the block proves degenerate.
  if (const std::optional<VertexBlockSource> source = resolve_vertex_block_source()) {
    if (load_tracks_fast(*source)) {
      GL::assert_context_is_current();
      return;
    }
  }

  // Dispatch to the generic multi-format framework rather than a hardcoded
  //   .tck reader, so that any supported tractogram format opens through one
  //   path. Vertices are read via the Streamline<float>& overload (no sidecars),
  //   which preserves the previous behaviour and cost for .tck.
  const DWI::Tractography::Formats::Base *const handler = DWI::Tractography::Formats::get_handler(filepath);
  if (handler == nullptr)
    throw Exception("unsupported tractogram format for \"" + filepath.string() + "\" (unrecognised file extension)");
  DWI::Tractography::FieldRegistry registry;
  auto reader = handler->read<float>(filepath, properties, registry);

  // Decide the GPU vertex storage width from the on-disk vertex datatype:
  //   Float16 -> half-precision; Float32 -> float; Float64 -> down-cast to
  //   float (the GPU has no f64 path); unknown -> float. The half-precision
  //   forcing config overrides any input.
  vertex_gpu_type = VertexGPUType::Float32;
  if (properties.vertex_datatype.has_value()) {
    const uint8_t vertex_type_bits = (*properties.vertex_datatype)() & DataType::Type;
    if (vertex_type_bits == DataType::Float16)
      vertex_gpu_type = VertexGPUType::Float16;
  }
  if (force_half_precision_gpu)
    vertex_gpu_type = VertexGPUType::Float16;

  // The same config flag governs floating-point sidecar (scalar) storage width.
  scalar_gpu_type = force_half_precision_gpu ? ScalarGPUType::Float16 : ScalarGPUType::Float32;

  // Enumerate the embedded data-per-vertex (dpv) / data-per-streamline (dps)
  //   fields the format carries, expanding any multi-column field into one
  //   entry per column (decision #4). No sidecar values are read here: the
  //   registry already describes every field after read<>() opened the file.
  //   The values themselves are read lazily on first selection of an entry.
  embedded_fields.clear();
  for (const auto &descriptor : registry) {
    if (descriptor.role != DWI::Tractography::FieldRole::DPV && descriptor.role != DWI::Tractography::FieldRole::DPS)
      continue;
    const size_t columns = std::max<size_t>(descriptor.columns, 1);
    for (size_t column = 0; column != columns; ++column) {
      EmbeddedScalarField field;
      field.name = descriptor.name;
      field.role = descriptor.role;
      field.ordinal = descriptor.ordinal;
      field.columns = columns;
      field.column = column;
      embedded_fields.push_back(std::move(field));
    }
  }

  DWI::Tractography::Streamline<float> tck;
  std::vector<Eigen::Vector3f> buffer;
  std::vector<GLint> starts;
  std::vector<GLint> sizes;
  size_t tck_count = 0;

  while ((*reader)(tck)) {

    const size_t N = tck.size();
    if (N == 0)
      continue;

    // Commit the current chunk before appending a streamline that would push
    //   it beyond the maximum buffer size, so each chunk holds at most
    //   MAX_BUFFER_SIZE vertices
    if (!buffer.empty() && buffer.size() + N > MAX_BUFFER_SIZE)
      load_tracks_onto_GPU(buffer, starts, sizes, tck_count);

    // No endpoint padding: the per-vertex classification (TrackVertexType)
    //   lets the vertex shader compute endpoint tangents directly.
    //   starts[] records the vertex index of each streamline within the chunk
    starts.push_back(static_cast<GLint>(buffer.size()));
    buffer.insert(buffer.end(), tck.begin(), tck.end());
    sizes.push_back(static_cast<GLint>(N));
    ++tck_count;

    // Quantise the endpoint-direction colour to RGB8 once at load time. The
    //   colour is the absolute components of the normalised endpoint offset;
    //   round (not truncate) so the result matches the previous float
    //   rendering within 1/255, and clamp to guard against rounding to 256.
    const Eigen::Vector3f colour((tck.back() - tck.front()).normalized().array().abs());
    std::array<uint8_t, 3> rgb;
    for (size_t c = 0; c != 3; ++c)
      rgb[c] = static_cast<uint8_t>(std::min(255.0f, std::round(255.0f * colour[c])));
    endpoint_colours.push_back(rgb);
  }
  if (!buffer.empty())
    load_tracks_onto_GPU(buffer, starts, sizes, tck_count);
  GL::assert_context_is_current();
}

namespace {

//! \brief Classification of one ".tck" binary slot (a 3-vector triplet).
enum class SlotKind { Real, Delimiter, Barrier };

//! \brief Read a triplet's first component (native byte order) as float.
/*! The ".tck" fast path only engages when on-disk byte order matches the host,
 * so the mapped bytes are reinterpreted directly. \a element_type is the on-disk
 * vertex datatype's Type bits (DataType::Float16 or DataType::Float32). */
inline float tck_slot_first_component(const std::byte *const slot, const uint8_t element_type) {
  if (element_type == DataType::Float16) {
    Eigen::half value;
    std::memcpy(&value, slot, sizeof(value));
    return static_cast<float>(value);
  }
  float value = 0.0f;
  std::memcpy(&value, slot, sizeof(value));
  return value;
}

//! \brief Classify a ".tck" binary slot from its first component.
/*! ".tck" delimits streamlines with an all-NaN triplet and marks end-of-data
 * with an all-Inf triplet; the first component suffices to distinguish them. */
inline SlotKind tck_classify_slot(const std::byte *const slot, const uint8_t element_type) {
  const float first = tck_slot_first_component(slot, element_type);
  if (std::isnan(first))
    return SlotKind::Delimiter;
  if (std::isinf(first))
    return SlotKind::Barrier;
  return SlotKind::Real;
}

//! \brief Read a full ".tck" triplet (native byte order) into a float 3-vector.
inline Eigen::Vector3f tck_read_vertex(const std::byte *const slot, const uint8_t element_type) {
  if (element_type == DataType::Float16) {
    std::array<Eigen::half, 3> value;
    std::memcpy(value.data(), slot, sizeof(value));
    return {static_cast<float>(value[0]), static_cast<float>(value[1]), static_cast<float>(value[2])};
  }
  std::array<float, 3> value;
  std::memcpy(value.data(), slot, sizeof(value));
  return {value[0], value[1], value[2]};
}

//! \brief Quantise an endpoint-direction colour to a packed RGB8 triplet.
/*! The colour is the absolute components of the streamline's normalised endpoint
 * offset; round (not truncate) so the result matches the previous float
 * rendering within 1/255, and clamp to guard against rounding to 256. Shared by
 * every fast path so each produces the exact same per-streamline colour as the
 * generic loader. */
inline std::array<uint8_t, 3> endpoint_colour_rgb8(const Eigen::Vector3f &front, const Eigen::Vector3f &back) {
  const Eigen::Vector3f colour((back - front).normalized().array().abs());
  std::array<uint8_t, 3> rgb;
  for (size_t c = 0; c != 3; ++c)
    rgb[c] = static_cast<uint8_t>(std::min(255.0f, std::round(255.0f * colour[c])));
  return rgb;
}

//! \brief Decode one on-disk coordinate into native-order float.
/*! Honours the block's byte order and element width: a Native block is read in
 * place; a BigEndian/LittleEndian block is byte-swapped on fetch. Float16 is
 * promoted to float. \a slot points at the first byte of the coordinate. */
inline float decode_coord(const std::byte *const slot,
                          const uint8_t element_type,
                          const MR::GUI::MRView::Tool::VertexByteOrder byte_order) {
  using MR::GUI::MRView::Tool::VertexByteOrder;
  if (element_type == DataType::Float16) {
    // Eigen::half is not a fundamental type, so the byte-order swap templates do
    //   not apply to it; the only fp16 fast path in play (TRX) is little-endian
    //   per spec and runs on a little-endian host, where the native fetch equals
    //   the little-endian fetch. Read it natively (mirrors the TRX fast path).
    (void)byte_order;
    return static_cast<float>(Raw::fetch_native<Eigen::half>(slot));
  }
  switch (byte_order) {
  case VertexByteOrder::BigEndian:
    return Raw::fetch_BE<float>(slot);
  case VertexByteOrder::LittleEndian:
    return Raw::fetch_LE<float>(slot);
  case VertexByteOrder::Native:
    return Raw::fetch_native<float>(slot);
  }
  assert(0);
  return 0.0f;
}

//! \brief Read a full triplet from an offsets/lines block, decoded to native float.
inline Eigen::Vector3f decode_vertex(const std::byte *const slot,
                                     const size_t element_bytes,
                                     const uint8_t element_type,
                                     const MR::GUI::MRView::Tool::VertexByteOrder byte_order) {
  return {decode_coord(slot, element_type, byte_order),
          decode_coord(slot + element_bytes, element_type, byte_order),
          decode_coord(slot + 2 * element_bytes, element_type, byte_order)};
}

} // namespace

std::optional<MR::GUI::MRView::Tool::VertexBlockSource> Tractogram::resolve_vertex_block_source() {
  GL::assert_context_is_current();

  const DWI::Tractography::Formats::Base *const handler = DWI::Tractography::Formats::get_handler(filepath);
  if (handler == nullptr)
    return std::nullopt;

  // ".tck": an in-band NaN-delimited block of native-order Float16/Float32
  //   triplets. The block is memory-mapped from its byte offset to EOF; the
  //   boundaries are scanned in place (no precomputed sizes).
  if (const auto *const tck_handler = dynamic_cast<const DWI::Tractography::Formats::TCK *>(handler)) {
    const std::optional<DWI::Tractography::Formats::TCKBinaryLayout> layout = tck_handler->binary_layout(filepath);
    if (!layout.has_value())
      return std::nullopt;

    // Applicability gate: Float16/Float32 only, and on-disk byte order must match
    //   the host — a verbatim memcpy of mismatched-endian data would upload
    //   byte-swapped coordinates. Float64 and any byte-order mismatch defer to
    //   the generic streaming reader, which converts per point.
    const uint8_t element_type = layout->datatype() & DataType::Type;
    if (element_type != DataType::Float16 && element_type != DataType::Float32)
      return std::nullopt;
    DataType file_datatype = layout->datatype;
    bool byte_order_native = false;
    try {
      byte_order_native = file_datatype.is_byte_order_native();
    } catch (Exception &) {
      return std::nullopt; // byte order unspecified in header
    }
    if (!byte_order_native) {
      INFO("tractogram \"" + filepath.string() +
           "\": .tck byte order differs from host; using generic (byte-swapping) loader");
      return std::nullopt;
    }

    const int64_t element_bytes_disk = static_cast<int64_t>(file_datatype.bytes()) * 3;

    auto mmap = std::make_shared<File::MMap>(File::Entry(layout->data_path, layout->data_offset));
    const std::byte *const block = mmap->address();
    const int64_t block_bytes = mmap->size();
    if (block == nullptr || block_bytes < element_bytes_disk)
      return std::nullopt;
    int64_t total_slots = block_bytes / element_bytes_disk;

    // Reject degenerate files before committing: the side-buffer alignment relies
    //   on exactly one in-band delimiter slot per (non-empty) streamline, so an
    //   empty streamline (two adjacent delimiters, or a leading delimiter) breaks
    //   the uniform-gap invariant; the generic path represents these identically
    //   without gaps. Also clip total_slots to the barrier so num_slots excludes
    //   the Inf end-of-data triplet.
    {
      bool prev_was_real = false;
      for (int64_t slot = 0; slot < total_slots; ++slot) {
        const SlotKind kind = tck_classify_slot(block + slot * element_bytes_disk, element_type);
        if (kind == SlotKind::Barrier) {
          total_slots = slot;
          break;
        }
        if (kind == SlotKind::Delimiter) {
          if (!prev_was_real)
            return std::nullopt; // empty streamline / leading delimiter: not conforming
          prev_was_real = false;
        } else {
          prev_was_real = true;
        }
      }
    }

    MR::GUI::MRView::Tool::VertexBlockSource source;
    source.block = block;
    source.num_slots = total_slots;
    source.element_datatype = file_datatype;
    source.byte_order = MR::GUI::MRView::Tool::VertexByteOrder::Native;
    source.boundary = MR::GUI::MRView::Tool::BoundaryMechanism::NaNDelimiter;
    source.file_datatype = file_datatype;
    source.mmap = std::move(mmap);
    return source;
  }

  // ".vtk": Float32 big-endian POINTS delimited by a LINES connectivity list,
  //   verified to be a contiguous, sequentially-ordered vertex run (the only
  //   topology MRtrix streamlines admit and the only one a contiguous-range copy
  //   of POINTS can serve). A non-contiguous / out-of-order connectivity defers.
  if (const auto *const vtk_handler = dynamic_cast<const DWI::Tractography::Formats::VTK *>(handler)) {
    const std::optional<DWI::Tractography::Formats::VTKBinaryLayout> layout = vtk_handler->binary_layout(filepath);
    if (!layout.has_value())
      return std::nullopt;

    // Applicability gate: Float32 POINTS only. Float64 POINTS would require a
    //   per-coordinate narrowing cast the generic streaming reader already
    //   performs; defer to it (matches the .tck Float64 policy).
    const uint8_t element_type = layout->points_datatype() & DataType::Type;
    if (element_type != DataType::Float32)
      return std::nullopt;

    auto mmap = std::make_shared<File::MMap>(File::Entry(filepath), false, true);
    const std::byte *const base = mmap->address();
    const int64_t mapped_size = mmap->size();
    if (base == nullptr)
      return std::nullopt;

    // Parse the LINES connectivity to recover, per streamline, its vertex count,
    //   verifying each line is a contiguous, sequentially-ordered run of indices.
    const size_t index_size = layout->lines_int64 ? sizeof(int64_t) : sizeof(int32_t);
    std::vector<GLint> streamline_sizes;
    streamline_sizes.reserve(layout->num_lines);
    {
      int64_t pos = layout->lines_offset;
      size_t running_vertex = 0;
      for (size_t l = 0; l != layout->num_lines; ++l) {
        if (pos + static_cast<int64_t>(index_size) > mapped_size)
          return std::nullopt;
        const int64_t count =
            layout->lines_int64 ? Raw::fetch_BE<int64_t>(base + pos) : Raw::fetch_BE<int32_t>(base + pos);
        pos += static_cast<int64_t>(index_size);
        if (count < 0)
          return std::nullopt;
        if (pos + count * static_cast<int64_t>(index_size) > mapped_size)
          return std::nullopt;
        for (int64_t v = 0; v != count; ++v) {
          const int64_t idx =
              layout->lines_int64 ? Raw::fetch_BE<int64_t>(base + pos) : Raw::fetch_BE<int32_t>(base + pos);
          pos += static_cast<int64_t>(index_size);
          if (idx != static_cast<int64_t>(running_vertex + static_cast<size_t>(v)))
            return std::nullopt; // non-contiguous / out-of-order: not a fast-path streamline
        }
        running_vertex += static_cast<size_t>(count);
        if (count > 0)
          streamline_sizes.push_back(static_cast<GLint>(count));
      }
      // The connectivity must reference exactly the POINTS it declares.
      if (running_vertex != layout->num_points)
        return std::nullopt;
    }

    if (streamline_sizes.empty())
      return std::nullopt; // no non-empty streamlines: defer to the generic (empty-tolerant) path

    // Verify the entire POINTS block is mapped before any copy.
    const int64_t points_bytes = static_cast<int64_t>(layout->num_points) * 3 * static_cast<int64_t>(sizeof(float));
    if (layout->points_offset + points_bytes > mapped_size)
      return std::nullopt;

    MR::GUI::MRView::Tool::VertexBlockSource source;
    source.block = base + layout->points_offset;
    source.num_slots = static_cast<int64_t>(layout->num_points);
    source.element_datatype = DataType(DataType::Float32);
    source.byte_order = MR::GUI::MRView::Tool::VertexByteOrder::BigEndian;
    source.boundary = MR::GUI::MRView::Tool::BoundaryMechanism::LinesConnectivity;
    source.streamline_sizes = std::move(streamline_sizes);
    source.file_datatype = DataType(DataType::Float32);
    source.mmap = std::move(mmap);
    return source;
  }

  // ".vtx": Float32 big-endian POINTS whose streamlines are delimited by an
  //   END-vertex OFFSETS array (no per-vertex connectivity; contiguous by
  //   construction). Streamline j spans offsetEnd[j-1]+1 .. offsetEnd[j], with
  //   offsetEnd[-1] = -1, so the per-streamline size is the difference of
  //   consecutive offsets and the first streamline starts at index 0.
  if (const auto *const vtx_handler = dynamic_cast<const DWI::Tractography::Formats::VTX *>(handler)) {
    const std::optional<DWI::Tractography::Formats::VTXBinaryLayout> layout = vtx_handler->binary_layout(filepath);
    if (!layout.has_value())
      return std::nullopt;

    const uint8_t element_type = layout->points_datatype() & DataType::Type;
    if (element_type != DataType::Float32)
      return std::nullopt;

    auto mmap = std::make_shared<File::MMap>(File::Entry(filepath), false, true);
    const std::byte *const base = mmap->address();
    const int64_t mapped_size = mmap->size();
    if (base == nullptr)
      return std::nullopt;

    const size_t index_size = layout->offsets_int64 ? sizeof(int64_t) : sizeof(int32_t);
    const int64_t offsets_bytes = static_cast<int64_t>(layout->num_streamlines) * static_cast<int64_t>(index_size);
    if (layout->offsets_offset + offsets_bytes > mapped_size)
      return std::nullopt;
    std::vector<GLint> streamline_sizes;
    streamline_sizes.reserve(layout->num_streamlines);
    {
      int64_t previous_offset_end = -1;
      for (size_t j = 0; j != layout->num_streamlines; ++j) {
        const std::byte *const slot = base + layout->offsets_offset + static_cast<int64_t>(j * index_size);
        const int64_t offset_end =
            layout->offsets_int64 ? Raw::fetch_BE<int64_t>(slot) : static_cast<int64_t>(Raw::fetch_BE<int32_t>(slot));
        const int64_t count = offset_end - previous_offset_end;
        if (count < 0)
          return std::nullopt; // non-monotonic OFFSETS: defer to the (error-reporting) generic reader
        previous_offset_end = offset_end;
        if (count > 0)
          streamline_sizes.push_back(static_cast<GLint>(count));
      }
      // The OFFSETS must reference exactly the POINTS the header declares: the
      //   last END-vertex index is the final point (num_points - 1).
      if (layout->num_streamlines > 0 && previous_offset_end != static_cast<int64_t>(layout->num_points) - 1)
        return std::nullopt;
    }

    if (streamline_sizes.empty())
      return std::nullopt; // no non-empty streamlines: defer to the generic (empty-tolerant) path

    const int64_t points_bytes = static_cast<int64_t>(layout->num_points) * 3 * static_cast<int64_t>(sizeof(float));
    if (layout->points_offset + points_bytes > mapped_size)
      return std::nullopt;

    MR::GUI::MRView::Tool::VertexBlockSource source;
    source.block = base + layout->points_offset;
    source.num_slots = static_cast<int64_t>(layout->num_points);
    source.element_datatype = DataType(DataType::Float32);
    source.byte_order = MR::GUI::MRView::Tool::VertexByteOrder::BigEndian;
    source.boundary = MR::GUI::MRView::Tool::BoundaryMechanism::OffsetsArray;
    source.streamline_sizes = std::move(streamline_sizes);
    source.file_datatype = DataType(DataType::Float32);
    source.mmap = std::move(mmap);
    return source;
  }

  // TRX (directory / uncompressed / compressed archive): a contiguous row-major
  //   (NB_VERTICES, 3) little-endian Float16/Float32 "positions" array in world
  //   (RASMM) space, delimited by a separate START-vertex "offsets" array (no
  //   per-vertex connectivity). The TrxSource resolves each member to a
  //   contiguous byte range (mmap in place for a directory / ZIP_STORE archive,
  //   extract-then-map for a ZIP_DEFLATE archive — inflating "positions" exactly
  //   once); it owns its backing and is kept alive in the descriptor.
  if (dynamic_cast<const DWI::Tractography::Formats::TRXBase *>(handler) != nullptr) {
    std::shared_ptr<DWI::Tractography::Formats::TRXUtils::TrxSource> trx_source;
    DWI::Tractography::Formats::TRXUtils::DatasetSummary summary;
    try {
      trx_source = DWI::Tractography::Formats::TRXUtils::open_source(filepath);
      summary = trx_source->summarise();
    } catch (Exception &e) {
      INFO("tractogram \"" + filepath.string() + "\": TRX fast path unavailable (" + e[0] + "); using generic loader");
      return std::nullopt;
    }

    // Applicability gate: Float16/Float32 positions only. Float64 positions would
    //   require a per-coordinate narrowing cast the generic streaming reader
    //   already performs; defer to it (matches the .tck policy).
    const uint8_t element_type = summary.positions_dtype() & DataType::Type;
    if (element_type != DataType::Float16 && element_type != DataType::Float32)
      return std::nullopt;
    const size_t pos_elem = summary.positions_dtype.bytes();

    // Verify the positions member spans the whole (NB_VERTICES, 3) array before
    //   any copy, and that the offsets member spans the entries summarised.
    if (summary.positions.data == nullptr || summary.offsets.data == nullptr)
      return std::nullopt;
    if (summary.positions.size < summary.nb_vertices * 3 * pos_elem)
      return std::nullopt;
    const size_t offsets_elem = summary.offsets_dtype.bytes();
    if (offsets_elem != 4 && offsets_elem != 8)
      return std::nullopt;
    if (summary.offsets.size < summary.offsets_count * offsets_elem)
      return std::nullopt;

    // Decode one entry of the START-vertex offsets array (little-endian per spec).
    //   If a trailing total is stored (NB_STREAMLINES+1 entries) the final
    //   streamline ends there, otherwise it runs to NB_VERTICES. Mirrors
    //   TRXReader::offset_at().
    const std::byte *const offsets_base = summary.offsets.data;
    const auto offset_at = [&](const size_t i) -> uint64_t {
      if (i < summary.offsets_count) {
        if (offsets_elem == 8)
          return Raw::fetch_LE<uint64_t>(offsets_base, i);
        return static_cast<uint64_t>(Raw::fetch_LE<uint32_t>(offsets_base, i));
      }
      return summary.nb_vertices;
    };

    // Derive each streamline's vertex count from consecutive START offsets. A
    //   non-monotonic or out-of-range span defers to the (error-reporting) reader.
    std::vector<GLint> streamline_sizes;
    streamline_sizes.reserve(summary.nb_streamlines);
    for (size_t j = 0; j != summary.nb_streamlines; ++j) {
      const uint64_t start = offset_at(j);
      const uint64_t end = offset_at(j + 1);
      if (end < start || end > summary.nb_vertices)
        return std::nullopt;
      const uint64_t count = end - start;
      if (count > 0)
        streamline_sizes.push_back(static_cast<GLint>(count));
    }

    if (streamline_sizes.empty())
      return std::nullopt; // no non-empty streamlines: defer to the generic (empty-tolerant) path

    MR::GUI::MRView::Tool::VertexBlockSource source;
    source.block = summary.positions.data;
    source.num_slots = static_cast<int64_t>(summary.nb_vertices);
    source.element_datatype = summary.positions_dtype;
    source.byte_order = MR::GUI::MRView::Tool::VertexByteOrder::LittleEndian;
    source.boundary = MR::GUI::MRView::Tool::BoundaryMechanism::OffsetsArray;
    source.streamline_sizes = std::move(streamline_sizes);
    source.file_datatype = summary.positions_dtype;
    source.source = std::move(trx_source);
    return source;
  }

  return std::nullopt;
}

bool Tractogram::load_tracks_fast(const VertexBlockSource &source) {
  GL::assert_context_is_current();

  const uint8_t element_type = source.element_datatype() & DataType::Type;
  const GLsizei element_bytes_disk = static_cast<GLsizei>(source.element_datatype.bytes()) * 3;
  const bool nan_delimited = (source.boundary == BoundaryMechanism::NaNDelimiter);

  // Commit the GPU storage widths exactly as the generic path would: the on-disk
  //   element width selects Float16/Float32, the force-half-precision config
  //   overrides any input, and the same flag governs scalar (sidecar) storage.
  vertex_gpu_type = (element_type == DataType::Float16) ? VertexGPUType::Float16 : VertexGPUType::Float32;
  if (force_half_precision_gpu)
    vertex_gpu_type = VertexGPUType::Float16;
  scalar_gpu_type = force_half_precision_gpu ? ScalarGPUType::Float16 : ScalarGPUType::Float32;
  properties.vertex_datatype = source.file_datatype;
  // None of the four fast paths reads embedded dpv/dps sidecars here; those are
  //   enumerated by the generic loader on demand. Register none.
  embedded_fields.clear();
  vertices_have_gaps = nan_delimited;

  // The in-band NaN-delimited path memcpy's the file verbatim, so the on-disk
  //   element width must match the chosen GPU width (the force-half-precision
  //   flag would up-/down-convert Float32 and so is incompatible with a verbatim
  //   memcpy); defer to the generic path in that case. The offsets/lines paths
  //   decode coordinate by coordinate and so honour any width unconditionally.
  if (nan_delimited && element_bytes_disk != vertex_element_bytes()) {
    vertices_have_gaps = false;
    return false;
  }

  // A chunk spans whole streamlines and, counting the 2 VBO sentinels, holds at
  //   most MAX_BUFFER_SIZE slots; a single streamline longer than the budget
  //   forms its own (over-budget) chunk, never split — mirroring the generic
  //   path. For the offsets/lines paths a "slot" is a real vertex; for the
  //   NaN-delimited path each streamline additionally consumes one delimiter slot.
  const int64_t max_chunk_slots = static_cast<int64_t>(MAX_BUFFER_SIZE) - 2;
  size_t chunk_count = 0;

  if (nan_delimited) {
    // Walk the in-band block, snapping each chunk to a trailing delimiter slot.
    int64_t chunk_start = 0;
    while (chunk_start < source.num_slots) {
      // Locate the chunk end: the last delimiter slot whose inclusion keeps the
      //   chunk within budget. Scan forward, remembering delimiter positions.
      int64_t chunk_end = -1;   // inclusive slot index of the chosen trailing delimiter
      int64_t first_delim = -1; // first delimiter at/after chunk_start (oversize fallback)
      for (int64_t slot = chunk_start; slot < source.num_slots; ++slot) {
        if (tck_classify_slot(source.block + slot * element_bytes_disk, element_type) != SlotKind::Delimiter)
          continue;
        if (first_delim < 0)
          first_delim = slot;
        if (slot - chunk_start + 1 <= max_chunk_slots)
          chunk_end = slot;
        else
          break;
      }
      if (chunk_end < 0) {
        // No delimiter fits the budget: the first streamline alone exceeds it.
        //   Emit it as an over-budget single-streamline chunk (never split).
        if (first_delim < 0)
          break; // malformed: no delimiter before EOF
        chunk_end = first_delim;
      }

      // Scan the chosen range [chunk_start, chunk_end] for streamline boundaries.
      //   starts[] are chunk-local slot indices; sizes[] the real vertex counts.
      //   The trailing slot of each streamline is its NaN delimiter.
      std::vector<GLint> starts;
      std::vector<GLint> sizes;
      int64_t run_start = chunk_start;
      Eigen::Vector3f chunk_first_vertex;
      Eigen::Vector3f chunk_last_vertex;
      bool have_first = false;
      for (int64_t slot = chunk_start; slot <= chunk_end; ++slot) {
        if (tck_classify_slot(source.block + slot * element_bytes_disk, element_type) != SlotKind::Delimiter)
          continue;
        const int64_t run_length = slot - run_start;
        if (run_length > 0) {
          starts.push_back(static_cast<GLint>(run_start - chunk_start));
          sizes.push_back(static_cast<GLint>(run_length));
          const Eigen::Vector3f front = tck_read_vertex(source.block + run_start * element_bytes_disk, element_type);
          const Eigen::Vector3f back = tck_read_vertex(source.block + (slot - 1) * element_bytes_disk, element_type);
          if (!have_first) {
            chunk_first_vertex = front;
            have_first = true;
          }
          chunk_last_vertex = back;
          endpoint_colours.push_back(endpoint_colour_rgb8(front, back));
        }
        run_start = slot + 1;
      }

      if (!starts.empty()) {
        const GLint num_slots = static_cast<GLint>(chunk_end - chunk_start + 1);
        upload_chunk_fastpath(source,
                              source.block + chunk_start * element_bytes_disk,
                              num_slots,
                              chunk_first_vertex,
                              chunk_last_vertex,
                              starts,
                              sizes);
        ++chunk_count;
      }

      chunk_start = chunk_end + 1;
    }
  } else {
    // Offsets / lines path: the block holds only real vertices, contiguous, in
    //   streamline order. Group whole streamlines into ≤ MAX_BUFFER_SIZE chunks,
    //   decoding each chunk's coordinates into a native-order staging buffer.
    const size_t max_chunk_vertices = MAX_BUFFER_SIZE - 2;
    const size_t element_bytes = source.element_datatype.bytes();

    std::vector<Eigen::Vector3f> chunk_vertices;
    std::vector<GLint> starts;
    std::vector<GLint> sizes;
    size_t global_vertex = 0; // running vertex index across the whole block

    //! \brief Stage one streamline's vertices (decoded to native float) into the chunk.
    const auto append_streamline = [&](const GLint n) {
      starts.push_back(static_cast<GLint>(chunk_vertices.size()));
      for (GLint v = 0; v != n; ++v) {
        const std::byte *const slot = source.block + (global_vertex + static_cast<size_t>(v)) * 3 * element_bytes;
        chunk_vertices.push_back(decode_vertex(slot, element_bytes, element_type, source.byte_order));
      }
      sizes.push_back(n);
      const Eigen::Vector3f &front = chunk_vertices[chunk_vertices.size() - static_cast<size_t>(n)];
      const Eigen::Vector3f &back = chunk_vertices.back();
      endpoint_colours.push_back(endpoint_colour_rgb8(front, back));
      global_vertex += static_cast<size_t>(n);
    };

    for (const GLint n : source.streamline_sizes) {
      // Commit the current chunk before a streamline that would push it beyond the
      //   budget, so each chunk spans whole streamlines within MAX_BUFFER_SIZE.
      if (!chunk_vertices.empty() && chunk_vertices.size() + static_cast<size_t>(n) > max_chunk_vertices) {
        upload_chunk_fastpath(source,
                              nullptr,
                              static_cast<GLint>(chunk_vertices.size()),
                              chunk_vertices.front(),
                              chunk_vertices.back(),
                              starts,
                              sizes,
                              chunk_vertices);
        ++chunk_count;
        chunk_vertices.clear();
        starts.clear();
        sizes.clear();
      }
      append_streamline(n);
    }
    if (!chunk_vertices.empty()) {
      upload_chunk_fastpath(source,
                            nullptr,
                            static_cast<GLint>(chunk_vertices.size()),
                            chunk_vertices.front(),
                            chunk_vertices.back(),
                            starts,
                            sizes,
                            chunk_vertices);
      ++chunk_count;
    }
  }

  if (chunk_count == 0) {
    // Empty / degenerate block: nothing uploaded. Leave the generic path to
    //   handle it (it tolerates empty tractograms) and undo the gap flag.
    vertices_have_gaps = false;
    return false;
  }

  const char *const width = (vertex_gpu_type == VertexGPUType::Float16) ? "Float16" : "Float32";
  const char *mechanism = "raw block";
  switch (source.boundary) {
  case BoundaryMechanism::NaNDelimiter:
    mechanism = ".tck NaN-delimited";
    break;
  case BoundaryMechanism::OffsetsArray:
    mechanism = "offsets-driven";
    break;
  case BoundaryMechanism::LinesConnectivity:
    mechanism = "lines-driven";
    break;
  }
  CONSOLE("tractogram \"" + filepath.string() + "\": loaded via fast path (" + mechanism + ", " + width + ", " +
          str(chunk_count) + " chunk(s))");
  GL::assert_context_is_current();
  return true;
}

void Tractogram::upload_chunk_fastpath(const VertexBlockSource &source,
                                       const std::byte *const chunk_bytes,
                                       const GLint num_slots,
                                       const Eigen::Vector3f &front_sentinel,
                                       const Eigen::Vector3f &back_sentinel,
                                       const std::vector<GLint> &starts,
                                       const std::vector<GLint> &sizes,
                                       const std::vector<Eigen::Vector3f> &staged_vertices) {
  GL::assert_context_is_current();
  assert(num_slots > 0);

  GLuint vertex_array_object = 0;
  gl::GenVertexArrays(1, &vertex_array_object);
  gl::BindVertexArray(vertex_array_object);

  // Position buffer: [front sentinel][num_slots vertex slots][back sentinel].
  //   The sentinels are copies of the chunk's first / last *real* vertex. For the
  //   in-band NaN-delimited path the slots are memcpy'd from the file verbatim
  //   (delimiter slots ride along, never indexed); otherwise the already-decoded
  //   native-order staging vertices are uploaded in the active GPU width.
  const GLsizei element_bytes = vertex_element_bytes();
  const bool half_precision = (vertex_gpu_type == VertexGPUType::Float16);
  GLuint vertexbuffer = 0;
  gl::GenBuffers(1, &vertexbuffer);
  gl::BindBuffer(gl::ARRAY_BUFFER, vertexbuffer);
  gl::BufferData(gl::ARRAY_BUFFER, (num_slots + 2) * element_bytes, nullptr, gl::STATIC_DRAW);

  if (source.boundary == BoundaryMechanism::NaNDelimiter) {
    // Verbatim memcpy: the on-disk element width matches the GPU width (gated in
    //   load_tracks_fast), so the bytes are uploaded as-is between the sentinels.
    assert(chunk_bytes != nullptr);
    assert(static_cast<GLsizei>(source.element_datatype.bytes()) * 3 == element_bytes);
    gl::BufferSubData(gl::ARRAY_BUFFER, element_bytes, num_slots * element_bytes, chunk_bytes);
    if (half_precision) {
      const std::array<Eigen::half, 3> front_half{
          Eigen::half(front_sentinel[0]), Eigen::half(front_sentinel[1]), Eigen::half(front_sentinel[2])};
      const std::array<Eigen::half, 3> back_half{
          Eigen::half(back_sentinel[0]), Eigen::half(back_sentinel[1]), Eigen::half(back_sentinel[2])};
      gl::BufferSubData(gl::ARRAY_BUFFER, 0, element_bytes, front_half.data());
      gl::BufferSubData(gl::ARRAY_BUFFER, (num_slots + 1) * element_bytes, element_bytes, back_half.data());
    } else {
      gl::BufferSubData(gl::ARRAY_BUFFER, 0, element_bytes, front_sentinel.data());
      gl::BufferSubData(gl::ARRAY_BUFFER, (num_slots + 1) * element_bytes, element_bytes, back_sentinel.data());
    }
  } else {
    // Staged decode: the chunk's real vertices are already native-order float.
    if (half_precision) {
      std::vector<Eigen::half> half_vertices(static_cast<size_t>(num_slots) * 3);
      for (GLint v = 0; v != num_slots; ++v) {
        for (size_t c = 0; c != 3; ++c)
          half_vertices[static_cast<size_t>(v) * 3 + c] = Eigen::half(staged_vertices[v][c]);
      }
      const std::array<Eigen::half, 3> front_half{
          Eigen::half(front_sentinel[0]), Eigen::half(front_sentinel[1]), Eigen::half(front_sentinel[2])};
      const std::array<Eigen::half, 3> back_half{
          Eigen::half(back_sentinel[0]), Eigen::half(back_sentinel[1]), Eigen::half(back_sentinel[2])};
      gl::BufferSubData(gl::ARRAY_BUFFER, 0, element_bytes, front_half.data());
      gl::BufferSubData(gl::ARRAY_BUFFER, element_bytes, num_slots * element_bytes, half_vertices.data());
      gl::BufferSubData(gl::ARRAY_BUFFER, (num_slots + 1) * element_bytes, element_bytes, back_half.data());
    } else {
      gl::BufferSubData(gl::ARRAY_BUFFER, 0, element_bytes, front_sentinel.data());
      gl::BufferSubData(gl::ARRAY_BUFFER, element_bytes, num_slots * element_bytes, &staged_vertices[0][0]);
      gl::BufferSubData(gl::ARRAY_BUFFER, (num_slots + 1) * element_bytes, element_bytes, back_sentinel.data());
    }
  }
  DEBUG("tractogram \"" + filepath.string() + "\" fast-path chunk: " + str(num_slots) + " slots uploaded as " +
        (half_precision ? "Float16" : "Float32") + " (" + str((num_slots + 2) * element_bytes) + " bytes, " +
        (source.boundary == BoundaryMechanism::NaNDelimiter ? "memcpy" : "staged") + ")");

  build_chunk_topology(vertex_array_object, vertexbuffer, num_slots, starts, sizes, starts.size());
  GL::assert_context_is_current();
}

void Tractogram::load_end_colours() {
  // These data are now retained in memory - no need to re-scan track file
  if (!colour_buffers.empty())
    return;

  // Make sure to set graphics context!
  // We're setting up vertex array objects
  GL::Context::Grab context;
  GL::assert_context_is_current();

  erase_colour_data();
  size_t total_tck_counter = 0;
  for (size_t buffer_index = 0, N = vertex_buffers.size(); buffer_index < N; ++buffer_index) {

    const size_t num_tracks = num_tracks_per_buffer[buffer_index];
    // Packed RGB8: 3 bytes per slot, addressed directly by element index,
    //   broadcasting each streamline's constant colour. When the vertex chunk
    //   carries in-band delimiter slots (fast path), one padding triplet is
    //   appended after each streamline so the colour slots stay aligned with
    //   the position buffer; the padding slot is never indexed.
    std::vector<uint8_t> buffer;
    for (size_t buffer_tck_counter = 0; buffer_tck_counter != num_tracks; ++buffer_tck_counter) {

      const std::array<uint8_t, 3> &colour(endpoint_colours[total_tck_counter++]);
      const size_t tck_length = original_track_sizes[buffer_index][buffer_tck_counter];

      for (size_t i = 0; i != tck_length; ++i)
        buffer.insert(buffer.end(), colour.begin(), colour.end());
      if (vertices_have_gaps)
        buffer.insert(buffer.end(), {uint8_t(0), uint8_t(0), uint8_t(0)});
    }
    load_end_colours_onto_GPU(buffer);
  }
  assert(colour_buffers.size() == vertex_buffers.size());
  // Don't need this now that we've initialised the GPU buffers
  endpoint_colours.clear();
  GL::assert_context_is_current();
}

void Tractogram::load_intensity_track_scalars(const std::filesystem::path &filepath) {
  // Make sure to set graphics context!
  // We're setting up vertex array objects
  GL::Context::Grab context;
  GL::assert_context_is_current();

  erase_intensity_scalar_data();
  value_min = std::numeric_limits<float>::infinity();
  value_max = -std::numeric_limits<float>::infinity();
  std::vector<float> buffer;
  DWI::Tractography::TrackScalar<float> tck_scalar;

  if (filepath.extension() == ".tsf") {
    DWI::Tractography::Properties scalar_properties;
    DWI::Tractography::ScalarReader<float> file(filepath, scalar_properties);
    DWI::Tractography::validate_tsf_properties(properties, scalar_properties, ".tck / .tsf pair");
    // Replay the chunk boundaries established when loading the tracks so that
    //   the scalar buffers align exactly with the vertex buffers
    size_t buffer_index = 0;
    size_t chunk_track_count = 0;
    while (file(tck_scalar)) {

      const size_t tck_size = tck_scalar.size();
      if (tck_size == 0)
        continue; // empty streamlines were skipped when loading the tracks

      if (buffer_index >= vertex_buffers.size() ||
          tck_size != static_cast<size_t>(original_track_sizes[buffer_index][chunk_track_count]))
        throw Exception("Track scalar file is inconsistent with the selected tractogram");

      for (size_t i = 0; i < tck_size; ++i) {
        buffer.push_back(tck_scalar[i]);
        value_max = std::max(value_max, tck_scalar[i]);
        value_min = std::min(value_min, tck_scalar[i]);
      }
      // Pad the in-band delimiter slot when the vertex chunk carries gaps.
      if (vertices_have_gaps)
        buffer.push_back(0.0f);

      if (++chunk_track_count == num_tracks_per_buffer[buffer_index]) {
        load_intensity_scalars_onto_GPU(buffer, chunk_track_count);
        ++buffer_index;
        chunk_track_count = 0;
      }
    }
    file.close();
    if (buffer_index != vertex_buffers.size())
      throw Exception("Track scalar file contains fewer streamlines than the selected tractogram");
  } else {
    const Eigen::VectorXf scalars = File::Matrix::load_vector<float>(filepath);
    size_t total_num_tracks = 0;
    for (std::vector<size_t>::const_iterator i = num_tracks_per_buffer.begin(); i != num_tracks_per_buffer.end(); ++i)
      total_num_tracks += *i;
    if (static_cast<size_t>(scalars.size()) != total_num_tracks)
      throw Exception("The scalar text file does not contain the same number of elements as the selected tractogram");
    size_t running_index = 0;

    for (size_t buffer_index = 0; buffer_index != vertex_buffers.size(); ++buffer_index) {

      size_t num_tracks = num_tracks_per_buffer[buffer_index];
      std::vector<GLint> &track_lengths(original_track_sizes[buffer_index]);

      for (size_t index = 0; index != num_tracks; ++index, ++running_index) {
        const float value = scalars[running_index];

        // One scalar per real vertex, plus a padding slot per streamline when
        //   the vertex chunk carries in-band delimiter slots (fast path).
        for (GLint i = 0; i < track_lengths[index]; ++i)
          buffer.push_back(value);
        if (vertices_have_gaps)
          buffer.push_back(0.0f);

        value_max = std::max(value_max, value);
        value_min = std::min(value_min, value);
      }

      load_intensity_scalars_onto_GPU(buffer, num_tracks);
    }
  }
  assert(intensity_scalar_buffers.size() == vertex_buffers.size());
  intensity_scalar_path = filepath;
  intensity_embedded_field.reset();
  this->set_windowing(value_min, value_max);
  if (!std::isfinite(greaterthan))
    greaterthan = value_max;
  if (!std::isfinite(lessthan))
    lessthan = value_min;
  GL::assert_context_is_current();
}

void Tractogram::load_threshold_track_scalars(const std::filesystem::path &filepath) {
  // Make sure to set graphics context!
  // We're setting up vertex array objects
  GL::Context::Grab context;
  GL::assert_context_is_current();

  erase_threshold_scalar_data();
  threshold_min = std::numeric_limits<float>::infinity();
  threshold_max = -std::numeric_limits<float>::infinity();
  std::vector<float> buffer;
  DWI::Tractography::TrackScalar<float> tck_scalar;

  if (filepath.extension() == ".tsf") {
    DWI::Tractography::Properties scalar_properties;
    DWI::Tractography::ScalarReader<float> file(filepath, scalar_properties);
    DWI::Tractography::validate_tsf_properties(properties, scalar_properties, ".tck / .tsf pair");
    // Replay the chunk boundaries established when loading the tracks so that
    //   the scalar buffers align exactly with the vertex buffers
    size_t buffer_index = 0;
    size_t chunk_track_count = 0;
    while (file(tck_scalar)) {

      const size_t tck_size = tck_scalar.size();
      if (tck_size == 0)
        continue; // empty streamlines were skipped when loading the tracks

      if (buffer_index >= vertex_buffers.size() ||
          tck_size != static_cast<size_t>(original_track_sizes[buffer_index][chunk_track_count]))
        throw Exception("Track scalar file is inconsistent with the selected tractogram");

      for (size_t i = 0; i < tck_size; ++i) {
        buffer.push_back(tck_scalar[i]);
        threshold_max = std::max(threshold_max, tck_scalar[i]);
        threshold_min = std::min(threshold_min, tck_scalar[i]);
      }
      // Pad the in-band delimiter slot when the vertex chunk carries gaps.
      if (vertices_have_gaps)
        buffer.push_back(0.0f);

      if (++chunk_track_count == num_tracks_per_buffer[buffer_index]) {
        load_threshold_scalars_onto_GPU(buffer, chunk_track_count);
        ++buffer_index;
        chunk_track_count = 0;
      }
    }
    file.close();
    if (buffer_index != vertex_buffers.size())
      throw Exception("Track scalar file contains fewer streamlines than the selected tractogram");
  } else {
    const Eigen::VectorXf scalars = File::Matrix::load_vector<float>(filepath);
    size_t total_num_tracks = 0;
    for (std::vector<size_t>::const_iterator i = num_tracks_per_buffer.begin(); i != num_tracks_per_buffer.end(); ++i)
      total_num_tracks += *i;
    if (static_cast<size_t>(scalars.size()) != total_num_tracks)
      throw Exception("The scalar text file does not contain the same number of elements as the selected tractogram");
    size_t running_index = 0;

    for (size_t buffer_index = 0; buffer_index != vertex_buffers.size(); ++buffer_index) {

      size_t num_tracks = num_tracks_per_buffer[buffer_index];
      std::vector<GLint> &track_lengths(original_track_sizes[buffer_index]);

      for (size_t index = 0; index != num_tracks; ++index, ++running_index) {
        const float value = scalars[running_index];

        // One scalar per real vertex, plus a padding slot per streamline when
        //   the vertex chunk carries in-band delimiter slots (fast path).
        for (GLint i = 0; i < track_lengths[index]; ++i)
          buffer.push_back(value);
        if (vertices_have_gaps)
          buffer.push_back(0.0f);

        threshold_max = std::max(threshold_max, value);
        threshold_min = std::min(threshold_min, value);
      }

      load_threshold_scalars_onto_GPU(buffer, num_tracks);
    }
  }
  assert(threshold_scalar_buffers.size() == vertex_buffers.size());
  threshold_scalar_path = filepath;
  threshold_embedded_field.reset();
  greaterthan = threshold_max;
  lessthan = threshold_min;

  GL::assert_context_is_current();
}

namespace {
//! \brief One element of a dpv field at (vertex, column), as float.
/*! Generalises dpv_scalar_to_float() to a chosen column of a multi-column
 * field; a single templated visitor converts any element type to float. */
inline float
dpv_element_to_float(const DWI::Tractography::DPVValue &value, const Eigen::Index vertex, const Eigen::Index column) {
  return MR::match_v(
      value, [vertex, column](const auto &matrix) -> float { return static_cast<float>(matrix(vertex, column)); });
}
//! \brief One element of a dps field at \a column, as float.
inline float dps_element_to_float(const DWI::Tractography::DPSValue &value, const Eigen::Index column) {
  return MR::match_v(value, [column](const auto &row) -> float { return static_cast<float>(row.vector()(0, column)); });
}
} // namespace

Tractogram::ScalarRange Tractogram::load_embedded_scalars_onto_GPU(const EmbeddedScalarField &field,
                                                                   const ScalarDestination destination) {
  GL::assert_context_is_current();

  // Re-open the tractogram through the generic loader and read the requested
  //   field via the TractogramItem overload. For formats whose sidecars are
  //   lazily backed (TRX directory / uncompressed archive memory-map; compressed
  //   TRX decompress-on-demand), only the selected field's bytes are
  //   materialised here — nothing is allocated at tractogram load time.
  const DWI::Tractography::Formats::Base *const handler = DWI::Tractography::Formats::get_handler(filepath);
  if (handler == nullptr)
    throw Exception("unsupported tractogram format for \"" + filepath.string() + "\"");
  DWI::Tractography::Properties read_properties;
  DWI::Tractography::FieldRegistry read_registry;
  auto reader = handler->read<float>(filepath, read_properties, read_registry);

  const DWI::Tractography::FieldRole role = field.role;
  const Eigen::Index column = static_cast<Eigen::Index>(field.column);

  float range_min = std::numeric_limits<float>::infinity();
  float range_max = -std::numeric_limits<float>::infinity();

  std::vector<float> buffer;
  DWI::Tractography::TractogramItem<float> item;
  // Replay the chunk boundaries established when loading the tracks so the
  //   scalar buffers align exactly with the vertex buffers (mirrors the .tsf
  //   path, including its size-consistency validation).
  size_t buffer_index = 0;
  size_t chunk_track_count = 0;
  while ((*reader)(item)) {

    const size_t tck_size = item.streamline.size();
    if (tck_size == 0)
      continue; // empty streamlines were skipped when loading the tracks

    if (buffer_index >= vertex_buffers.size() ||
        tck_size != static_cast<size_t>(original_track_sizes[buffer_index][chunk_track_count]))
      throw Exception("Embedded scalar field \"" + field.name + "\" is inconsistent with the selected tractogram");

    if (role == DWI::Tractography::FieldRole::DPV) {
      if (field.ordinal >= item.dpv.size())
        throw Exception("Embedded per-vertex field \"" + field.name + "\" is missing from a streamline");
      const DWI::Tractography::DPVValue &value = item.dpv[field.ordinal];
      for (size_t i = 0; i != tck_size; ++i) {
        const float v = dpv_element_to_float(value, static_cast<Eigen::Index>(i), column);
        buffer.push_back(v);
        range_max = std::max(range_max, v);
        range_min = std::min(range_min, v);
      }
    } else {
      if (field.ordinal >= item.dps.size())
        throw Exception("Embedded per-streamline field \"" + field.name + "\" is missing from a streamline");
      // One value per streamline, broadcast to all its vertices (mirrors the
      //   endpoint-colour broadcast).
      const float v = dps_element_to_float(item.dps[field.ordinal], column);
      for (size_t i = 0; i != tck_size; ++i)
        buffer.push_back(v);
      range_max = std::max(range_max, v);
      range_min = std::min(range_min, v);
    }
    // Pad the in-band delimiter slot when the vertex chunk carries gaps, so the
    //   scalar slots stay aligned with the position buffer (never indexed).
    if (vertices_have_gaps)
      buffer.push_back(0.0f);

    if (++chunk_track_count == num_tracks_per_buffer[buffer_index]) {
      if (destination == ScalarDestination::Intensity)
        load_intensity_scalars_onto_GPU(buffer, chunk_track_count);
      else
        load_threshold_scalars_onto_GPU(buffer, chunk_track_count);
      ++buffer_index;
      chunk_track_count = 0;
    }
  }
  if (buffer_index != vertex_buffers.size())
    throw Exception("Embedded scalar field \"" + field.name +
                    "\" spans fewer streamlines than the selected tractogram");

  GL::assert_context_is_current();
  return {range_min, range_max};
}

void Tractogram::load_intensity_embedded_scalars(const size_t entry) {
  GL::Context::Grab context;
  GL::assert_context_is_current();
  assert(entry < embedded_fields.size());
  const EmbeddedScalarField &field = embedded_fields[entry];

  erase_intensity_scalar_data();
  const ScalarRange range = load_embedded_scalars_onto_GPU(field, ScalarDestination::Intensity);
  assert(intensity_scalar_buffers.size() == vertex_buffers.size());

  value_min = range.min;
  value_max = range.max;
  intensity_embedded_field = entry;
  set_windowing(value_min, value_max);
  if (!std::isfinite(greaterthan))
    greaterthan = value_max;
  if (!std::isfinite(lessthan))
    lessthan = value_min;
  GL::assert_context_is_current();
}

void Tractogram::load_threshold_embedded_scalars(const size_t entry) {
  GL::Context::Grab context;
  GL::assert_context_is_current();
  assert(entry < embedded_fields.size());
  const EmbeddedScalarField &field = embedded_fields[entry];

  erase_threshold_scalar_data();
  const ScalarRange range = load_embedded_scalars_onto_GPU(field, ScalarDestination::Threshold);
  assert(threshold_scalar_buffers.size() == vertex_buffers.size());

  threshold_min = range.min;
  threshold_max = range.max;
  threshold_embedded_field = entry;
  greaterthan = threshold_max;
  lessthan = threshold_min;
  GL::assert_context_is_current();
}

void Tractogram::erase_colour_data() {
  GL::Context::Grab context;
  GL::assert_context_is_current();
  if (!colour_buffers.empty()) {
    gl::DeleteBuffers(colour_buffers.size(), &colour_buffers[0]);
    colour_buffers.clear();
  }
  GL::assert_context_is_current();
}

void Tractogram::erase_intensity_scalar_data() {
  GL::Context::Grab context;
  GL::assert_context_is_current();
  if (!intensity_scalar_buffers.empty()) {
    gl::DeleteBuffers(intensity_scalar_buffers.size(), &intensity_scalar_buffers[0]);
    intensity_scalar_buffers.clear();
  }
  intensity_scalar_path.clear();
  intensity_embedded_field.reset();
  GL::assert_context_is_current();
}

void Tractogram::erase_threshold_scalar_data() {
  GL::Context::Grab context;
  GL::assert_context_is_current();
  if (!threshold_scalar_buffers.empty()) {
    gl::DeleteBuffers(threshold_scalar_buffers.size(), &threshold_scalar_buffers[0]);
    threshold_scalar_buffers.clear();
  }
  threshold_scalar_path.clear();
  threshold_embedded_field.reset();
  threshold_min = NaNF;
  threshold_max = NaNF;
  set_use_discard_lower(false);
  set_use_discard_upper(false);
  GL::assert_context_is_current();
}

void Tractogram::set_color_type(const TrackColourType c) {
  if ((color_type == TrackColourType::Ends && c == TrackColourType::ScalarFile) ||
      (color_type == TrackColourType::ScalarFile && c == TrackColourType::Ends))
    vao_dirty = true;
  color_type = c;
}

void Tractogram::set_threshold_type(const TrackThresholdType t) {
  threshold_type = t;
  switch (threshold_type) {
  case TrackThresholdType::None:
    threshold_min = threshold_max = NaN;
    break;
  case TrackThresholdType::UseColourFile:
    threshold_min = value_min;
    threshold_max = value_max;
    break;
  case TrackThresholdType::SeparateFile:
    break;
  }
}

void Tractogram::set_geometry_type(const TrackGeometryType t) {
  geometry_type = t;
  should_update_lod = true;
}

void Tractogram::load_tracks_onto_GPU(std::vector<Eigen::Vector3f> &buffer,
                                      std::vector<GLint> &starts,
                                      std::vector<GLint> &sizes,
                                      size_t &tck_count) {
  GL::assert_context_is_current();
  assert(!buffer.empty());

  // Number of real (unpadded) vertices in this chunk
  const GLint num_vertices = static_cast<GLint>(buffer.size());

  GLuint vertex_array_object = 0;
  gl::GenVertexArrays(1, &vertex_array_object);
  gl::BindVertexArray(vertex_array_object);

  // Position buffer: the real vertices are bracketed by one sentinel slot at
  //   each end so that the prev / next neighbour fetch of the chunk's first
  //   and last vertices stays within the buffer. The sentinel values are
  //   never used by the shader (the first vertex is classified First and so
  //   ignores prev; the last is classified Last and so ignores next)
  const Eigen::Vector3f front_sentinel = buffer.front();
  const Eigen::Vector3f back_sentinel = buffer.back();
  const GLsizei element_bytes = vertex_element_bytes();
  GLuint vertexbuffer = 0;
  gl::GenBuffers(1, &vertexbuffer);
  gl::BindBuffer(gl::ARRAY_BUFFER, vertexbuffer);
  gl::BufferData(gl::ARRAY_BUFFER, (num_vertices + 2) * element_bytes, nullptr, gl::STATIC_DRAW);
  if (vertex_gpu_type == VertexGPUType::Float16) {
    // The fallback path stages vertices in float; convert to half-precision
    //   (3 components per vertex) for the upload. The fast paths in later
    //   stages will supply native half-precision bytes directly.
    std::vector<Eigen::half> half_vertices(static_cast<size_t>(num_vertices) * 3);
    for (GLint v = 0; v != num_vertices; ++v) {
      for (size_t c = 0; c != 3; ++c)
        half_vertices[static_cast<size_t>(v) * 3 + c] = Eigen::half(buffer[v][c]);
    }
    const std::array<Eigen::half, 3> front_half{
        Eigen::half(front_sentinel[0]), Eigen::half(front_sentinel[1]), Eigen::half(front_sentinel[2])};
    const std::array<Eigen::half, 3> back_half{
        Eigen::half(back_sentinel[0]), Eigen::half(back_sentinel[1]), Eigen::half(back_sentinel[2])};
    gl::BufferSubData(gl::ARRAY_BUFFER, 0, element_bytes, front_half.data());
    gl::BufferSubData(gl::ARRAY_BUFFER, element_bytes, num_vertices * element_bytes, half_vertices.data());
    gl::BufferSubData(gl::ARRAY_BUFFER, (num_vertices + 1) * element_bytes, element_bytes, back_half.data());
  } else {
    gl::BufferSubData(gl::ARRAY_BUFFER, 0, element_bytes, front_sentinel.data());
    gl::BufferSubData(gl::ARRAY_BUFFER, element_bytes, num_vertices * element_bytes, &buffer[0][0]);
    gl::BufferSubData(gl::ARRAY_BUFFER, (num_vertices + 1) * element_bytes, element_bytes, back_sentinel.data());
  }
  DEBUG("tractogram \"" + filepath.string() + "\" chunk: " + str(num_vertices) + " vertices uploaded as " +
        (vertex_gpu_type == VertexGPUType::Float16 ? "Float16" : "Float32") + " (" +
        str((num_vertices + 2) * element_bytes) + " bytes)");

  // The fallback chunk holds only real vertices, contiguously: there are no
  //   in-band delimiter slots, so the slot count equals the vertex count and
  //   the per-vertex side buffers need no padding.
  build_chunk_topology(vertex_array_object, vertexbuffer, num_vertices, starts, sizes, tck_count);

  buffer.clear();
  starts.clear();
  sizes.clear();
  tck_count = 0;
  GL::assert_context_is_current();
}

void Tractogram::build_chunk_topology(const GLuint vertex_array_object,
                                      const GLuint vertexbuffer,
                                      const GLint num_slots,
                                      const std::vector<GLint> &starts,
                                      const std::vector<GLint> &sizes,
                                      const size_t tck_count) {
  GL::assert_context_is_current();

  // Per-vertex classification, one entry per buffer slot (no VBO sentinels:
  //   addressed directly by element index). starts[] are slot indices; for the
  //   fast path the slots between streamlines hold in-band NaN delimiters and
  //   are left Middle (never indexed by the element buffers below).
  std::vector<uint8_t> vertex_classes(num_slots, static_cast<uint8_t>(TrackVertexType::Middle));
  for (size_t t = 0; t < sizes.size(); ++t) {
    const GLint start = starts[t];
    const GLint n = sizes[t];
    if (n == 1) {
      vertex_classes[start] = static_cast<uint8_t>(TrackVertexType::Single);
    } else {
      vertex_classes[start] = static_cast<uint8_t>(TrackVertexType::First);
      vertex_classes[start + n - 1] = static_cast<uint8_t>(TrackVertexType::Last);
    }
  }
  GLuint typebuffer = 0;
  gl::GenBuffers(1, &typebuffer);
  gl::BindBuffer(gl::ARRAY_BUFFER, typebuffer);
  gl::BufferData(gl::ARRAY_BUFFER, vertex_classes.size() * sizeof(uint8_t), vertex_classes.data(), gl::STATIC_DRAW);

  vertex_array_objects.push_back(vertex_array_object);
  vertex_buffers.push_back(vertexbuffer);
  vertex_type_buffers.push_back(typebuffer);
  original_track_sizes.push_back(sizes);
  num_tracks_per_buffer.push_back(tck_count);

  // Precompute, for every sub-sampling level, the element indices that draw
  //   this chunk. Element indices address buffer slots directly (0-based);
  //   the position-buffer sentinels are reached implicitly via the prev / next
  //   attribute offsets. Both streamline endpoints are always included, and a
  //   PRIMITIVE_RESTART_SENTINEL separates streamlines. Interior indices step
  //   only within [start, start + span], so in-band delimiter slots are never
  //   emitted (the NaN-skip property).
  std::array<std::vector<uint32_t>, num_lod_levels> indices;
  for (size_t level = 0; level < num_lod_levels; ++level) {
    const GLint ratio = lod_ratio_for_level(level);
    std::vector<uint32_t> &chunk_indices = indices[level];
    for (size_t t = 0; t < sizes.size(); ++t) {
      const GLint start = starts[t];
      const GLint span = sizes[t] - 1;
      // Always include the first streamline endpoint.
      chunk_indices.push_back(static_cast<uint32_t>(start));
      if (span > 0) {
        // Number of interior vertices to display, chosen so that consecutive
        //   displayed vertices are separated by approximately "ratio" steps:
        //   round(span / ratio) intervals, hence one fewer interior vertex.
        const GLint interior_count = (span + ratio / 2) / ratio - 1;
        if (interior_count > 0) {
          // The interior vertices are evenly spaced by exactly "ratio"; the
          //   leftover (span is rarely an exact multiple of the stride) is
          //   split symmetrically across the two terminal intervals. The
          //   resulting gap from the start vertex to the first interior vertex
          //   thus matches the gap from the last interior vertex to the end
          //   vertex, rather than dumping the entire remainder at one end.
          const GLint first = (span - (interior_count - 1) * ratio + 1) / 2;
          for (GLint i = 0; i < interior_count; ++i)
            chunk_indices.push_back(static_cast<uint32_t>(start + first + i * ratio));
        }
        // Always include the terminating streamline endpoint.
        chunk_indices.push_back(static_cast<uint32_t>(start + span));
      }
      chunk_indices.push_back(PRIMITIVE_RESTART_SENTINEL);
    }
  }

  // Create the chunk's element buffer object and upload the active ratio.
  //   Bind the VAO first so the ELEMENT_ARRAY_BUFFER binding is recorded as
  //   part of the VAO state.
  GLuint ebo = 0;
  gl::GenBuffers(1, &ebo);
  gl::BindVertexArray(vertex_array_object);
  gl::BindBuffer(gl::ELEMENT_ARRAY_BUFFER, ebo);
  const std::vector<uint32_t> &active_indices = indices[lod_level];
  gl::BufferData(
      gl::ELEMENT_ARRAY_BUFFER, active_indices.size() * sizeof(uint32_t), active_indices.data(), gl::STATIC_DRAW);
  element_buffers.push_back(ebo);
  element_counts.push_back(static_cast<GLsizei>(active_indices.size()));
  element_indices.push_back(std::move(indices));

  GL::assert_context_is_current();
}

void Tractogram::load_end_colours_onto_GPU(std::vector<uint8_t> &buffer) {
  GL::assert_context_is_current();

  GLuint vertexbuffer;
  gl::GenBuffers(1, &vertexbuffer);
  gl::BindBuffer(gl::ARRAY_BUFFER, vertexbuffer);
  // Packed RGB8: 3 bytes per vertex. The shader receives a normalised vec3 in
  //   [0,1] via the GL_TRUE normalisation flag on the attribute pointer.
  gl::BufferData(gl::ARRAY_BUFFER, buffer.size() * sizeof(uint8_t), buffer.data(), gl::STATIC_DRAW);

  vao_dirty = true;

  colour_buffers.push_back(vertexbuffer);
  buffer.clear();
  GL::assert_context_is_current();
}

//! \brief Upload one per-vertex scalar buffer, honouring the half-precision flag.
/*! When scalar_gpu_type is Float16, the staged float values are cast to
 * Eigen::half before upload (and bound as gl::HALF_FLOAT in the VAO); otherwise
 * the floats are uploaded directly. */
static void upload_scalar_buffer(const std::vector<float> &buffer, const ScalarGPUType gpu_type) {
  if (gpu_type == ScalarGPUType::Float16) {
    std::vector<Eigen::half> half_buffer(buffer.size());
    for (size_t i = 0; i != buffer.size(); ++i)
      half_buffer[i] = Eigen::half(buffer[i]);
    gl::BufferData(gl::ARRAY_BUFFER, half_buffer.size() * sizeof(Eigen::half), half_buffer.data(), gl::STATIC_DRAW);
  } else {
    gl::BufferData(gl::ARRAY_BUFFER, buffer.size() * sizeof(float), buffer.data(), gl::STATIC_DRAW);
  }
}

void Tractogram::load_intensity_scalars_onto_GPU(std::vector<float> &buffer, size_t &tck_count) {
  GL::assert_context_is_current();

  assert(num_tracks_per_buffer[intensity_scalar_buffers.size()] == tck_count);

  GLuint vertexbuffer;
  gl::GenBuffers(1, &vertexbuffer);
  gl::BindBuffer(gl::ARRAY_BUFFER, vertexbuffer);
  upload_scalar_buffer(buffer, scalar_gpu_type);

  vao_dirty = true;

  intensity_scalar_buffers.push_back(vertexbuffer);
  buffer.clear();
  tck_count = 0;

  GL::assert_context_is_current();
}

void Tractogram::load_threshold_scalars_onto_GPU(std::vector<float> &buffer, size_t &tck_count) {
  GL::assert_context_is_current();

  assert(num_tracks_per_buffer[threshold_scalar_buffers.size()] == tck_count);

  GLuint vertexbuffer;
  gl::GenBuffers(1, &vertexbuffer);
  gl::BindBuffer(gl::ARRAY_BUFFER, vertexbuffer);
  upload_scalar_buffer(buffer, scalar_gpu_type);

  vao_dirty = true;

  threshold_scalar_buffers.push_back(vertexbuffer);
  buffer.clear();
  tck_count = 0;

  GL::assert_context_is_current();
}

} // namespace MR::GUI::MRView::Tool
