/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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

#ifndef __gui_mrview_tool_roi_editor_undoentry_h__
#define __gui_mrview_tool_roi_editor_undoentry_h__

#include <array>
#include <atomic>

#include "types.h"

#include "gui/opengl/shader.h"
#include "gui/opengl/gl.h"


namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {


        class ROI_Item;


        struct ROI_UndoEntry { MEMALIGN(ROI_UndoEntry)

          ROI_UndoEntry (ROI_Item&, int, int);
          ROI_UndoEntry (const ROI_UndoEntry&) = delete;
          ROI_UndoEntry (ROI_UndoEntry&&);
          ~ROI_UndoEntry();

          ROI_UndoEntry& operator= (const ROI_UndoEntry&) = delete;
          ROI_UndoEntry& operator= (ROI_UndoEntry&&);

          void draw_line (ROI_Item&, const Eigen::Vector3f&, const Eigen::Vector3f&, const bool);
          void draw_thick_line (ROI_Item&, const Eigen::Vector3f&, const Eigen::Vector3f&, const bool, const float);
          void draw_circle (ROI_Item&, const Eigen::Vector3f&, const bool, const float);
          void draw_rectangle (ROI_Item&, const Eigen::Vector3f&, const Eigen::Vector3f&, const bool);
          void draw_fill (ROI_Item&, const Eigen::Vector3f&, const bool);

          void undo (ROI_Item& roi);
          void redo (ROI_Item& roi);
          void copy (ROI_Item& roi, ROI_UndoEntry& source);

          std::array<GLint,3> from, size;
          std::array<GLint,2> tex_size, slice_axes;
          vector<GLubyte> before, after;

          class Shared
          { MEMALIGN(Shared)
            public:
              Shared();
              ~Shared();
              GL::Shader::Program program;
              GL::VertexBuffer vertex_buffer;
              GL::VertexArrayObject vertex_array_object;
              void operator++ ();
              bool operator-- ();
            private:
              std::atomic<uint32_t> count;
          };
          static std::unique_ptr<Shared> shared;

        };





      }
    }
  }
}

#endif


