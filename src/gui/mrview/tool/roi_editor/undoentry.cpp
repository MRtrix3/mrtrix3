/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */

#include <limits>
#include <vector>

#include "gui/mrview/window.h"
#include "gui/mrview/tool/roi_editor/item.h"
#include "gui/mrview/tool/roi_editor/undoentry.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {


        std::unique_ptr<ROI_UndoEntry::Shared> ROI_UndoEntry::shared;
            

        ROI_UndoEntry::Shared::Shared() :
            count (1)
        {
          MRView::GrabContext context;
          GL::Shader::Vertex vertex_shader (
              "layout(location = 0) in ivec3 vertpos;\n"
              "void main() {\n"
              "  gl_Position = vec4 (vertpos,1);\n"
              "}\n");
          GL::Shader::Fragment fragment_shader (
              "uniform isampler3D tex;\n"
              "uniform ivec3 position;\n"
              "uniform ivec2 axes;\n"
              "layout (location = 0) out vec3 color0;\n"
              "void main() {\n"
              "  ivec3 pos = position;\n"
              "  pos[axes.x] = int(gl_FragCoord.x);\n"
              "  pos[axes.y] = int(gl_FragCoord.y);\n"
              "  color0.r = texelFetch (tex, pos, 0).r;\n"
              "}\n");

          program.attach (vertex_shader);
          program.attach (fragment_shader);
          program.link();

          vertex_buffer.gen();
          vertex_array_object.gen();

          vertex_buffer.bind (gl::ARRAY_BUFFER);
          vertex_array_object.bind();

          gl::EnableVertexAttribArray (0);
          gl::VertexAttribIPointer (0, 3, gl::INT, 3*sizeof(GLint), (void*)0);

          GLint vertices[12] = {
            -1, -1, 0,
            -1, 1, 0,
            1, 1, 0,
            1, -1, 0,
          };
          gl::BufferData (gl::ARRAY_BUFFER, sizeof(vertices), vertices, gl::STREAM_DRAW);
        }



        ROI_UndoEntry::Shared::~Shared()
        {
          assert (!count);
          MRView::GrabContext context;
          program.clear();
          vertex_buffer.clear();
          vertex_array_object.clear();
        }



        void ROI_UndoEntry::Shared::operator++ ()
        {
          ++count;
        }

        bool ROI_UndoEntry::Shared::operator-- ()
        {
          return --count;
        }




        ROI_UndoEntry::ROI_UndoEntry (ROI_Item& roi, int current_axis, int current_slice)
        {
          from = { { 0, 0, 0 } };
          from[current_axis] = current_slice;
          size = { { GLint(roi.header().size(0)), GLint(roi.header().size(1)), GLint(roi.header().size(2)) } };
          size[current_axis] = 1;

          if (current_axis == 0) { slice_axes[0] = 1; slice_axes[1] = 2; }
          else if (current_axis == 1) { slice_axes[0] = 0; slice_axes[1] = 2; }
          else { slice_axes[0] = 0; slice_axes[1] = 1; }
          tex_size = { { size[slice_axes[0]], size[slice_axes[1]] } };

          MRView::GrabContext context;
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;

          if (!shared)
            shared.reset (new Shared());
          else
            ++(*shared);
          shared->vertex_array_object.bind();

          // set up 2D texture to store slice:
          GL::Texture tex;
          tex.gen (gl::TEXTURE_2D, gl::NEAREST);
          gl::PixelStorei (gl::UNPACK_ALIGNMENT, 1);
          gl::TexImage2D (gl::TEXTURE_2D, 0, gl::R8, tex_size[0], tex_size[1], 0, gl::RED, gl::UNSIGNED_BYTE, nullptr);
          GL_CHECK_ERROR;

          // set up off-screen framebuffer to map textures onto:
          GL::FrameBuffer framebuffer;
          framebuffer.gen();
          tex.set_interp_on (false);
          framebuffer.attach_color (tex, 0);
          framebuffer.draw_buffers (0);
          framebuffer.check();
          GL_CHECK_ERROR;

          // render slice onto framebuffer:
          gl::Disable (gl::DEPTH_TEST);
          gl::Disable (gl::BLEND);
          gl::DepthMask (gl::FALSE_);
          gl::Viewport (0, 0, tex_size[0], tex_size[1]);
          roi.texture().bind();
          shared->program.start();
          gl::Uniform3iv (gl::GetUniformLocation (shared->program, "position"), 1, from.data());
          gl::Uniform2iv (gl::GetUniformLocation (shared->program, "axes"), 1, slice_axes.data());

          gl::DrawArrays (gl::TRIANGLE_FAN, 0, 4);
          shared->program.stop();
          framebuffer.unbind();
          GL_CHECK_ERROR;

          // retrieve texture contents to main memory:
          before.resize (tex_size[0]*tex_size[1]);
          tex.bind();
          gl::PixelStorei (gl::PACK_ALIGNMENT, 1);

          gl::GetTexImage (gl::TEXTURE_2D, 0, gl::RED, gl::UNSIGNED_BYTE, (void*)(&before[0]));
          after = before;
          GL_CHECK_ERROR;
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }



        ROI_UndoEntry::ROI_UndoEntry (ROI_UndoEntry&& r) :
          from (r.from),
          size (r.size),
          tex_size (r.tex_size),
          slice_axes (r.slice_axes),
          before (std::move (r.before)),
          after (std::move (r.after))
        {
          assert (shared);
          ++(*shared);
        }



        ROI_UndoEntry::~ROI_UndoEntry()
        {
          assert (shared);
          if (!--(*shared))
            delete shared.release();
        }



        ROI_UndoEntry& ROI_UndoEntry::operator= (ROI_UndoEntry&& r)
        {
          from = r.from;
          size = r.size;
          tex_size = r.tex_size;
          slice_axes = r.slice_axes;
          before = std::move (r.before);
          after = std::move (r.after);
          return *this;
        }




        void ROI_UndoEntry::draw_line (ROI_Item& roi, const Eigen::Vector3f& prev_pos, const Eigen::Vector3f& pos, const bool insert_mode_value)
        {
          const GLubyte value = insert_mode_value ? 1 : 0;
          Eigen::Vector3f p = roi.transform().scanner2voxel.cast<float>() * prev_pos;
          const Eigen::Vector3f final_pos = roi.transform().scanner2voxel.cast<float>() * pos;
          const Eigen::Vector3f dir ((final_pos - p).normalized());
          Eigen::Array3i v (int(std::round (p[0])), int(std::round (p[1])), int(std::round (p[2])));
          const Eigen::Array3i final_vox (int(std::round (final_pos[0])), int(std::round (final_pos[1])), int(std::round (final_pos[2])));
          do {
            if (v[0] >= 0 && v[0] < roi.header().size(0) && v[1] >= 0 && v[1] < roi.header().size(1) && v[2] >= 0 && v[2] < roi.header().size(2))
              after[v[0]-from[0] + size[0] * (v[1]-from[1] + size[1] * (v[2]-from[2]))] = value;
            if ((v - final_vox).abs().maxCoeff()) {
              Eigen::Array3i step (0, 0, 0);
              float min_multiplier = std::numeric_limits<float>::infinity();
              for (size_t axis = 0; axis != 3; ++axis) {
                float this_multiplier;
                if (dir[axis] > 0.0f)
                  this_multiplier = ((v[axis]+0.5f) - p[axis]) / dir[axis];
                else
                  this_multiplier = ((v[axis]-0.5f) - p[axis]) / dir[axis];
                if (std::isfinite (this_multiplier) && this_multiplier < min_multiplier) {
                  min_multiplier = this_multiplier;
                  step = { 0, 0, 0 };
                  step[axis] = (dir[axis] > 0.0f) ? 1 : -1;
                }
              }
              v += step;
              p += dir * min_multiplier;
            }
          } while ((v - final_vox).abs().maxCoeff());

          MRView::GrabContext context;
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
          roi.texture().bind();
          gl::TexSubImage3D (gl::TEXTURE_3D, 0, from[0], from[1], from[2], size[0], size[1], size[2], gl::RED, gl::UNSIGNED_BYTE, (void*) (&after[0]));
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }




        void ROI_UndoEntry::draw_thick_line (ROI_Item& roi, const Eigen::Vector3f& prev_pos, const Eigen::Vector3f& pos, const bool insert_mode_value, const float diameter)
        {
          roi.brush_size = diameter;
          const float radius = 0.5f * diameter;
          const float radius_sq = Math::pow2 (radius);
          const GLubyte value = insert_mode_value ? 1 : 0;
          const Eigen::Vector3f start = roi.transform().scanner2voxel.cast<float>() * prev_pos;
          const Eigen::Vector3f end = roi.transform().scanner2voxel.cast<float>() * pos;
          const Eigen::Vector3f offset (end - start);
          const float offset_norm (offset.norm());
          const Eigen::Vector3f dir (Eigen::Vector3f(offset).normalized());

          std::array<int,3> a = { { int(std::lround (std::min (start[0], end[0]))),   int(std::lround (std::min (start[1], end[1]))),   int(std::lround (std::min (start[2], end[2])))   } };
          std::array<int,3> b = { { int(std::lround (std::max (start[0], end[0])))+1, int(std::lround (std::max (start[1], end[1])))+1, int(std::lround (std::max (start[2], end[2])))+1 } };

          int rad[2] = { int(std::ceil (radius/roi.header().spacing (slice_axes[0]))), int(std::ceil (radius/roi.header().spacing (slice_axes[1]))) };
          a[slice_axes[0]] = std::max (0, a[slice_axes[0]]-rad[0]);
          a[slice_axes[1]] = std::max (0, a[slice_axes[1]]-rad[1]);
          b[slice_axes[0]] = std::min (int(roi.header().size (slice_axes[0])), b[slice_axes[0]]+rad[0]);
          b[slice_axes[1]] = std::min (int(roi.header().size (slice_axes[1])), b[slice_axes[1]]+rad[1]);

          for (int k = a[2]; k < b[2]; ++k) {
            for (int j = a[1]; j < b[1]; ++j) {
              for (int i = a[0]; i < b[0]; ++i) {

                const Eigen::Vector3f p { float(i), float(j), float(k) };
                const Eigen::Vector3f v (p - start);
                if ((v.dot (dir) > 0.0f) && (v.dot (dir) < offset_norm)) {
                  Eigen::Vector3f d (v - (dir * v.dot (dir)));
                  d[0] *= roi.header().spacing(0); d[1] *= roi.header().spacing(1); d[2] *= roi.header().spacing(2);
                  if (d.squaredNorm() < radius_sq)
                    after[i-from[0] + size[0] * (j-from[1] + size[1] * (k-from[2]))] = value;
                }

          } } }

          MRView::GrabContext context;
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
          roi.texture().bind();
          gl::TexSubImage3D (gl::TEXTURE_3D, 0, from[0], from[1], from[2], size[0], size[1], size[2], gl::RED, gl::UNSIGNED_BYTE, (void*) (&after[0]));
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }




        void ROI_UndoEntry::draw_circle (ROI_Item& roi, const Eigen::Vector3f& pos, const bool insert_mode_value, const float diameter)
        {
          Eigen::Vector3f vox = roi.transform().scanner2voxel.cast<float>() * pos;
          roi.brush_size = diameter;
          const float radius = 0.5f * diameter;
          const float radius_sq = Math::pow2 (radius);
          const GLubyte value = insert_mode_value ? 1 : 0;

          std::array<int,3> a = { { int(std::lround (vox[0])), int(std::lround (vox[1])), int(std::lround (vox[2])) } };
          std::array<int,3> b = { { a[0]+1, a[1]+1, a[2]+1 } };

          int rad[2] = { int(std::ceil (radius/roi.header().spacing (slice_axes[0]))), int(std::ceil (radius/roi.header().spacing (slice_axes[1]))) };
          a[slice_axes[0]] = std::max (0, a[slice_axes[0]]-rad[0]);
          a[slice_axes[1]] = std::max (0, a[slice_axes[1]]-rad[1]);
          b[slice_axes[0]] = std::min (int(roi.header().size (slice_axes[0])), b[slice_axes[0]]+rad[0]);
          b[slice_axes[1]] = std::min (int(roi.header().size (slice_axes[1])), b[slice_axes[1]]+rad[1]);

          for (int k = a[2]; k < b[2]; ++k)
            for (int j = a[1]; j < b[1]; ++j)
              for (int i = a[0]; i < b[0]; ++i)
                if (Math::pow2 (roi.header().spacing(0) * (vox[0]-i)) +
                    Math::pow2 (roi.header().spacing(1) * (vox[1]-j)) +
                    Math::pow2 (roi.header().spacing(2) * (vox[2]-k)) < radius_sq)
                  after[i-from[0] + size[0] * (j-from[1] + size[1] * (k-from[2]))] = value;

          MRView::GrabContext context;
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
          roi.texture().bind();
          gl::TexSubImage3D (gl::TEXTURE_3D, 0, from[0], from[1], from[2], size[0], size[1], size[2], gl::RED, gl::UNSIGNED_BYTE, (void*) (&after[0]));
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }




        void ROI_UndoEntry::draw_rectangle (ROI_Item& roi, const Eigen::Vector3f& from_pos, const Eigen::Vector3f& to_pos, const bool insert_mode_value)
        {
          Eigen::Vector3f vox = roi.transform().scanner2voxel.cast<float>() * from_pos;
          const GLubyte value = insert_mode_value ? 1 : 0;
          std::array<int,3> a = { { int(std::lround (vox[0])), int(std::lround (vox[1])), int(std::lround (vox[2])) } };
          vox = roi.transform().scanner2voxel.cast<float>() * to_pos;
          std::array<int,3> b = { { int(std::lround (vox[0])), int(std::lround (vox[1])), int(std::lround (vox[2])) } };

          if (a[0] > b[0]) std::swap (a[0], b[0]);
          if (a[1] > b[1]) std::swap (a[1], b[1]);
          if (a[2] > b[2]) std::swap (a[2], b[2]);

          a[slice_axes[0]] = std::max (0, a[slice_axes[0]]);
          a[slice_axes[1]] = std::max (0, a[slice_axes[1]]);
          b[slice_axes[0]] = std::min (int(roi.header().size (slice_axes[0])-1), b[slice_axes[0]]);
          b[slice_axes[1]] = std::min (int(roi.header().size (slice_axes[1])-1), b[slice_axes[1]]);

          after = before;
          for (int k = a[2]; k <= b[2]; ++k)
            for (int j = a[1]; j <= b[1]; ++j)
              for (int i = a[0]; i <= b[0]; ++i)
                after[i-from[0] + size[0] * (j-from[1] + size[1] * (k-from[2]))] = value;

          MRView::GrabContext context;
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
          roi.texture().bind();
          gl::TexSubImage3D (gl::TEXTURE_3D, 0, from[0], from[1], from[2], size[0], size[1], size[2], gl::RED, gl::UNSIGNED_BYTE, (void*) (&after[0]));
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }



        void ROI_UndoEntry::draw_fill (ROI_Item& roi, const Eigen::Vector3f& pos, const bool insert_mode_value)
        {
          const Eigen::Vector3f vox = roi.transform().scanner2voxel.cast<float>() * pos;
          const std::array<int,3> seed_voxel = { { int(std::lround (vox[0])), int(std::lround (vox[1])), int(std::lround (vox[2])) } };
          for (size_t axis = 0; axis != 3; ++axis) {
            if (seed_voxel[axis] < 0) return;
            if (seed_voxel[axis] >= int(roi.header().size (axis))) return;
          }
          const GLubyte fill_value = insert_mode_value ? 1 : 0;
          const size_t seed_index = seed_voxel[0]-from[0] + size[0] * (seed_voxel[1]-from[1] + size[1] * (seed_voxel[2]-from[2]));
          const bool existing_value = after[seed_index];
          if (existing_value == insert_mode_value) return;
          after[seed_index] = fill_value;
          vector<std::array<int,3>> buffer (1, seed_voxel);
          while (buffer.size()) {
            const std::array<int,3> v (buffer.back());
            buffer.pop_back();
            for (size_t i = 0; i != 4; ++i) {
              std::array<int,3> adj (v);
              switch (i) {
                case 0: adj[slice_axes[0]] -= 1; break;
                case 1: adj[slice_axes[0]] += 1; break;
                case 2: adj[slice_axes[1]] -= 1; break;
                case 3: adj[slice_axes[1]] += 1; break;
              }
              if (adj[0] >= 0 && adj[0] < int(roi.header().size (0)) && adj[1] >= 0 && adj[1] < int(roi.header().size (1)) && adj[2] >= 0 && adj[2] < int(roi.header().size (2))) {
                const size_t adj_index = adj[0]-from[0] + size[0] * (adj[1]-from[1] + size[1] * (adj[2]-from[2]));
                const bool adj_value = after[adj_index];
                if (adj_value != insert_mode_value) {
                  after[adj_index] = fill_value;
                  buffer.push_back (adj);
                }
              }
            }
          }
          MRView::GrabContext context;
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
          roi.texture().bind();
          gl::TexSubImage3D (gl::TEXTURE_3D, 0, from[0], from[1], from[2], size[0], size[1], size[2], gl::RED, gl::UNSIGNED_BYTE, (void*) (&after[0]));
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }





        void ROI_UndoEntry::undo (ROI_Item& roi) 
        {
          MRView::GrabContext context;
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
          roi.texture().bind();
          gl::TexSubImage3D (gl::TEXTURE_3D, 0, from[0], from[1], from[2], size[0], size[1], size[2], gl::RED, gl::UNSIGNED_BYTE, (void*) (&before[0]));
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }

        void ROI_UndoEntry::redo (ROI_Item& roi) 
        {
          MRView::GrabContext context;
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
          roi.texture().bind();
          gl::TexSubImage3D (gl::TEXTURE_3D, 0, from[0], from[1], from[2], size[0], size[1], size[2], gl::RED, gl::UNSIGNED_BYTE, (void*) (&after[0]));
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }

        void ROI_UndoEntry::copy (ROI_Item& roi, ROI_UndoEntry& source) 
        {
          MRView::GrabContext context;
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
          after = source.before;
          roi.texture().bind();
          gl::TexSubImage3D (gl::TEXTURE_3D, 0, from[0], from[1], from[2], size[0], size[1], size[2], gl::RED, gl::UNSIGNED_BYTE, (void*) (&after[0]));
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }




      }
    }
  }
}




