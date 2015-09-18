/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 2014.

   This file is part of MRtrix.

   MRtrix is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   MRtrix is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <limits>

#include "image/transform.h"

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

            
        GL::Shader::Program   ROI_UndoEntry::copy_program;
        GL::VertexBuffer      ROI_UndoEntry::copy_vertex_buffer;
        GL::VertexArrayObject ROI_UndoEntry::copy_vertex_array_object;



        ROI_UndoEntry::ROI_UndoEntry (ROI_Item& roi, int current_axis, int current_slice)
        {
          from = { { 0, 0, 0 } };
          from[current_axis] = current_slice;
          size = { { roi.info().dim(0), roi.info().dim(1), roi.info().dim(2) } };
          size[current_axis] = 1;

          if (current_axis == 0) { slice_axes[0] = 1; slice_axes[1] = 2; }
          else if (current_axis == 1) { slice_axes[0] = 0; slice_axes[1] = 2; }
          else { slice_axes[0] = 0; slice_axes[1] = 1; }
          tex_size = { { size[slice_axes[0]], size[slice_axes[1]] } };

          Window::GrabContext context;
          if (!copy_program) {
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

            copy_program.attach (vertex_shader);
            copy_program.attach (fragment_shader);
            copy_program.link();
          }

          if (!copy_vertex_array_object) {
            copy_vertex_buffer.gen();
            copy_vertex_array_object.gen();

            copy_vertex_buffer.bind (gl::ARRAY_BUFFER);
            copy_vertex_array_object.bind();

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
          else copy_vertex_array_object.bind();


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
          copy_program.start();
          gl::Uniform3iv (gl::GetUniformLocation (copy_program, "position"), 1, from.data());
          gl::Uniform2iv (gl::GetUniformLocation (copy_program, "axes"), 1, slice_axes.data());

          gl::DrawArrays (gl::TRIANGLE_FAN, 0, 4);
          copy_program.stop();
          framebuffer.unbind();
          GL_CHECK_ERROR;

          // retrieve texture contents to main memory:
          before.resize (tex_size[0]*tex_size[1]);
          tex.bind();
          gl::PixelStorei (gl::PACK_ALIGNMENT, 1);

          gl::GetTexImage (gl::TEXTURE_2D, 0, gl::RED, gl::UNSIGNED_BYTE, (void*)(&before[0]));
          after = before;
          GL_CHECK_ERROR;
        }




        void ROI_UndoEntry::draw_line (ROI_Item& roi, const Point<>& prev_pos, const Point<>& pos, bool insert_mode_value)
        {
          const GLubyte value = insert_mode_value ? 1 : 0;
          Point<> p = roi.transform().scanner2voxel (prev_pos);
          const Point<> final_pos = roi.transform().scanner2voxel (pos);
          const Point<> dir ((final_pos - p).normalise());
          Point<int> v (int(std::round (p[0])), int(std::round (p[1])), int(std::round (p[2])));
          const Point<int> final_vox (int(std::round (final_pos[0])), int(std::round (final_pos[1])), int(std::round (final_pos[2])));
          do {
            if (v[0] >= 0 && v[0] < roi.info().dim(0) && v[1] >= 0 && v[1] < roi.info().dim(1) && v[2] >= 0 && v[2] < roi.info().dim(2))
              after[v[0]-from[0] + size[0] * (v[1]-from[1] + size[1] * (v[2]-from[2]))] = value;
            if (v != final_vox) {
              Point<int> step (0, 0, 0);
              float min_multiplier = std::numeric_limits<float>::infinity();
              for (size_t axis = 0; axis != 3; ++axis) {
                float this_multiplier;
                if (dir[axis] > 0.0f)
                  this_multiplier = ((v[axis]+0.5f) - p[axis]) / dir[axis];
                else
                  this_multiplier = ((v[axis]-0.5f) - p[axis]) / dir[axis];
                if (std::isfinite (this_multiplier) && this_multiplier < min_multiplier) {
                  min_multiplier = this_multiplier;
                  step.set (0, 0, 0);
                  step[axis] = (dir[axis] > 0.0f) ? 1 : -1;
                }
              }
              v += step;
              p += dir * min_multiplier;
            }
          } while (v != final_vox);

          Window::GrabContext context;
          roi.texture().bind();
          gl::TexSubImage3D (gl::TEXTURE_3D, 0, from[0], from[1], from[2], size[0], size[1], size[2], gl::RED, gl::UNSIGNED_BYTE, (void*) (&after[0]));
        }




        void ROI_UndoEntry::draw_thick_line (ROI_Item& roi, const Point<>& prev_pos, const Point<>& pos, bool insert_mode_value, float diameter)
        {
          roi.brush_size = diameter;
          const float radius = 0.5f * diameter;
          const float radius_sq = Math::pow2 (radius);
          const GLubyte value = insert_mode_value ? 1 : 0;
          const Point<> start = roi.transform().scanner2voxel (prev_pos);
          const Point<> end = roi.transform().scanner2voxel (pos);
          const Point<> offset (end - start);
          const float offset_norm (offset.norm());
          const Point<> dir (Point<>(offset).normalise());

          std::array<int,3> a = { { int(std::lround (std::min (start[0], end[0]))),   int(std::lround (std::min (start[1], end[1]))),   int(std::lround (std::min (start[2], end[2])))   } };
          std::array<int,3> b = { { int(std::lround (std::max (start[0], end[0])))+1, int(std::lround (std::max (start[1], end[1])))+1, int(std::lround (std::max (start[2], end[2])))+1 } };

          int rad[2] = { int(std::ceil (radius/roi.info().vox(slice_axes[0]))), int(std::ceil (radius/roi.info().vox(slice_axes[1]))) };
          a[slice_axes[0]] = std::max (0, a[slice_axes[0]]-rad[0]);
          a[slice_axes[1]] = std::max (0, a[slice_axes[1]]-rad[1]);
          b[slice_axes[0]] = std::min (roi.info().dim(slice_axes[0]), b[slice_axes[0]]+rad[0]);
          b[slice_axes[1]] = std::min (roi.info().dim(slice_axes[1]), b[slice_axes[1]]+rad[1]);

          for (int k = a[2]; k < b[2]; ++k) {
            for (int j = a[1]; j < b[1]; ++j) {
              for (int i = a[0]; i < b[0]; ++i) {

                const Point<> p (i, j, k);
                const Point<> v (p - start);
                if ((v.dot (dir) > 0.0f) && (v.dot (dir) < offset_norm)) {
                  Point<> d (v - (dir * v.dot (dir)));
                  d[0] *= roi.info().vox(0); d[1] *= roi.info().vox(1); d[2] *= roi.info().vox(2);
                  if (d.norm2() < radius_sq)
                    after[i-from[0] + size[0] * (j-from[1] + size[1] * (k-from[2]))] = value;
                }

          } } }

          Window::GrabContext context;
          roi.texture().bind();
          gl::TexSubImage3D (gl::TEXTURE_3D, 0, from[0], from[1], from[2], size[0], size[1], size[2], gl::RED, gl::UNSIGNED_BYTE, (void*) (&after[0]));
        }




        void ROI_UndoEntry::draw_circle (ROI_Item& roi, const Point<>& pos, bool insert_mode_value, float diameter)
        {
          Point<> vox = roi.transform().scanner2voxel (pos);
          roi.brush_size = diameter;
          const float radius = 0.5f * diameter;
          const float radius_sq = Math::pow2 (radius);
          const GLubyte value = insert_mode_value ? 1 : 0;

          std::array<int,3> a = { { int(std::lround (vox[0])), int(std::lround (vox[1])), int(std::lround (vox[2])) } };
          std::array<int,3> b = { { a[0]+1, a[1]+1, a[2]+1 } };

          int rad[2] = { int(std::ceil (radius/roi.info().vox(slice_axes[0]))), int(std::ceil (radius/roi.info().vox(slice_axes[1]))) };
          a[slice_axes[0]] = std::max (0, a[slice_axes[0]]-rad[0]);
          a[slice_axes[1]] = std::max (0, a[slice_axes[1]]-rad[1]);
          b[slice_axes[0]] = std::min (roi.info().dim(slice_axes[0]), b[slice_axes[0]]+rad[0]);
          b[slice_axes[1]] = std::min (roi.info().dim(slice_axes[1]), b[slice_axes[1]]+rad[1]);

          for (int k = a[2]; k < b[2]; ++k)
            for (int j = a[1]; j < b[1]; ++j)
              for (int i = a[0]; i < b[0]; ++i)
                if (Math::pow2 (roi.info().vox(0) * (vox[0]-i)) +
                    Math::pow2 (roi.info().vox(1) * (vox[1]-j)) +
                    Math::pow2 (roi.info().vox(2) * (vox[2]-k)) < radius_sq)
                  after[i-from[0] + size[0] * (j-from[1] + size[1] * (k-from[2]))] = value;

          Window::GrabContext context;
          roi.texture().bind();
          gl::TexSubImage3D (gl::TEXTURE_3D, 0, from[0], from[1], from[2], size[0], size[1], size[2], gl::RED, gl::UNSIGNED_BYTE, (void*) (&after[0]));
        }




        void ROI_UndoEntry::draw_rectangle (ROI_Item& roi, const Point<>& from_pos, const Point<>& to_pos, bool insert_mode_value)
        {
          Point<> vox = roi.transform().scanner2voxel (from_pos);
          const GLubyte value = insert_mode_value ? 1 : 0;
          std::array<int,3> a = { { int(std::lround (vox[0])), int(std::lround (vox[1])), int(std::lround (vox[2])) } };
          vox = roi.transform().scanner2voxel (to_pos);
          std::array<int,3> b = { { int(std::lround (vox[0])), int(std::lround (vox[1])), int(std::lround (vox[2])) } };

          if (a[0] > b[0]) std::swap (a[0], b[0]);
          if (a[1] > b[1]) std::swap (a[1], b[1]);
          if (a[2] > b[2]) std::swap (a[2], b[2]);

          a[slice_axes[0]] = std::max (0, a[slice_axes[0]]);
          a[slice_axes[1]] = std::max (0, a[slice_axes[1]]);
          b[slice_axes[0]] = std::min (roi.info().dim(slice_axes[0])-1, b[slice_axes[0]]);
          b[slice_axes[1]] = std::min (roi.info().dim(slice_axes[1])-1, b[slice_axes[1]]);

          after = before;
          for (int k = a[2]; k <= b[2]; ++k)
            for (int j = a[1]; j <= b[1]; ++j)
              for (int i = a[0]; i <= b[0]; ++i)
                after[i-from[0] + size[0] * (j-from[1] + size[1] * (k-from[2]))] = value;

          Window::GrabContext context;
          roi.texture().bind();
          gl::TexSubImage3D (gl::TEXTURE_3D, 0, from[0], from[1], from[2], size[0], size[1], size[2], gl::RED, gl::UNSIGNED_BYTE, (void*) (&after[0]));
        }



        void ROI_UndoEntry::draw_fill (ROI_Item& roi, const Point<> pos, bool insert_mode_value)
        {
          const Point<> vox = roi.transform().scanner2voxel (pos);
          const Point<int> seed_voxel (std::lround (vox[0]), std::lround (vox[1]), std::lround (vox[2]));
          for (int axis = 0; axis != 3; ++axis) {
            if (seed_voxel[axis] < 0) return;
            if (seed_voxel[axis] >= roi.info().dim (axis)) return;
          }
          const GLubyte fill_value = insert_mode_value ? 1 : 0;
          const size_t seed_index = seed_voxel[0]-from[0] + size[0] * (seed_voxel[1]-from[1] + size[1] * (seed_voxel[2]-from[2]));
          const bool existing_value = after[seed_index];
          if (existing_value == insert_mode_value) return;
          after[seed_index] = fill_value;
          std::vector< Point<int> > buffer (1, seed_voxel);
          while (buffer.size()) {
            const Point<int> v (buffer.back());
            buffer.pop_back();
            for (size_t i = 0; i != 4; ++i) {
              Point<int> adj (v);
              switch (i) {
                case 0: adj[slice_axes[0]] -= 1; break;
                case 1: adj[slice_axes[0]] += 1; break;
                case 2: adj[slice_axes[1]] -= 1; break;
                case 3: adj[slice_axes[1]] += 1; break;
              }
              if (adj[0] >= 0 && adj[0] < roi.info().dim (0) && adj[1] >= 0 && adj[1] < roi.info().dim (1) && adj[2] >= 0 && adj[2] < roi.info().dim (2)) {
                const size_t adj_index = adj[0]-from[0] + size[0] * (adj[1]-from[1] + size[1] * (adj[2]-from[2]));
                const bool adj_value = after[adj_index];
                if (adj_value != insert_mode_value) {
                  after[adj_index] = fill_value;
                  buffer.push_back (adj);
                }
              }

            }
          }
          Window::GrabContext context;
          roi.texture().bind();
          gl::TexSubImage3D (gl::TEXTURE_3D, 0, from[0], from[1], from[2], size[0], size[1], size[2], gl::RED, gl::UNSIGNED_BYTE, (void*) (&after[0]));
        }





        void ROI_UndoEntry::undo (ROI_Item& roi) 
        {
          Window::GrabContext context;
          roi.texture().bind();
          gl::TexSubImage3D (gl::TEXTURE_3D, 0, from[0], from[1], from[2], size[0], size[1], size[2], gl::RED, gl::UNSIGNED_BYTE, (void*) (&before[0]));
        }

        void ROI_UndoEntry::redo (ROI_Item& roi) 
        {
          roi.texture().bind();
          gl::TexSubImage3D (gl::TEXTURE_3D, 0, from[0], from[1], from[2], size[0], size[1], size[2], gl::RED, gl::UNSIGNED_BYTE, (void*) (&after[0]));
        }

        void ROI_UndoEntry::copy (ROI_Item& roi, ROI_UndoEntry& source) 
        {
          Window::GrabContext context;
          after = source.before;
          roi.texture().bind();
          gl::TexSubImage3D (gl::TEXTURE_3D, 0, from[0], from[1], from[2], size[0], size[1], size[2], gl::RED, gl::UNSIGNED_BYTE, (void*) (&after[0]));
        }




      }
    }
  }
}




