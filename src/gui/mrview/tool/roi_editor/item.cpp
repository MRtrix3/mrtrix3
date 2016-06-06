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


#include <iomanip>
#include <string>
#include <vector>

#include "gui/mrview/tool/roi_editor/item.h"

#include "progressbar.h"
#include "file/config.h"
#include "algo/loop.h"

#include "gui/dialog/file.h"
#include "gui/mrview/window.h"


namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {



        //CONF option: NumberOfUndos
        //CONF default: 16
        //CONF The number of undo operations permitted in the MRView ROI editor tool.
        int ROI_Item::number_of_undos = MR::File::Config::get_int ("NumberOfUndos", 16);
        int ROI_Item::current_preset_colour = 0;
        int ROI_Item::new_roi_counter = 0;




        ROI_Item::ROI_Item (MR::Header&& src) :
            Volume (std::move (src)),
            saved (true),
            current_undo (-1)
        {
          type = gl::UNSIGNED_BYTE;
          format = gl::RED;
          internal_format = gl::R8;
          set_allowed_features (false, true, false);
          set_interpolate (false);
          set_use_transparency (true);
          value_min = 0.0f; value_max = 1.0f;
          set_windowing (0.0f, 1.0f);
          min_max_set();
          alpha = 1.0f;
          colour = preset_colours[current_preset_colour++];
          if (current_preset_colour >= 6)
            current_preset_colour = 0;
          transparent_intensity = 0.4f;
          opaque_intensity = 0.6f;
          colourmap = ColourMap::index ("Colour");
          float spacing = std::min ( { header().spacing(0), header().spacing(1), header().spacing(2) } );
          brush_size = min_brush_size = spacing;
          max_brush_size = 100.0f*min_brush_size;

          std::stringstream name;
          name << "ROI" << std::setfill('0') << std::setw(5) << new_roi_counter++ << ".mif";
          filename = name.str();

          MRView::GrabContext context;
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
          bind();
          allocate();
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }



        void ROI_Item::zero ()
        {
          MRView::GrabContext context;
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
          bind();
          std::vector<GLubyte> data (header().size(0)*header().size(1));
          for (int n = 0; n < header().size(2); ++n)
            upload_data ({ { 0, 0, n } }, { { header().size(0), header().size(1), 1 } }, reinterpret_cast<void*> (&data[0]));
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }


        void ROI_Item::load ()
        {
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
          MRView::GrabContext context;
          bind();
          auto image = header().get_image<bool>();
          std::vector<GLubyte> data (image.size(0)*image.size(1));
          ProgressBar progress ("loading ROI image \"" + header().name() + "\"");
          for (auto outer = MR::Loop(2) (image); outer; ++outer) {
            auto p = data.begin();
            for (auto inner = MR::Loop (0,2) (image); inner; ++inner)
              *(p++) = image.value();
            upload_data ({ { 0, 0, image.index(2) } }, { { image.size(0), image.size(1), 1 } }, reinterpret_cast<void*> (&data[0]));
            ++progress;
          }
          filename = header().name();
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }


        void ROI_Item::start (ROI_UndoEntry&& entry)
        {
          saved = false;
          if (current_undo < 0)
            current_undo = -1;
          while (current_undo+1 > int(undo_list.size()))
            undo_list.erase (undo_list.end()-1);
          undo_list.push_back (std::move (entry));
          while (undo_list.size() > size_t (number_of_undos))
            undo_list.erase (undo_list.begin());
          current_undo = undo_list.size()-1;
        }

        void ROI_Item::undo ()
        {
          if (has_undo()) {
            undo_list[current_undo].undo (*this);
            --current_undo;
          }
        }

        void ROI_Item::redo ()
        {
          if (has_redo()) {
            ++current_undo;
            undo_list[current_undo].redo (*this);
          }
        }




      }
    }
  }
}




