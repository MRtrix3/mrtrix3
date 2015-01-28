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


#include <iomanip>
#include <string>
#include <vector>

#include "progressbar.h"
#include "file/config.h"
#include "image/buffer.h"
#include "image/loop.h"

#include "gui/dialog/file.h"
#include "gui/mrview/tool/roi_analysis/item.h"


namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {



        int ROI_Item::number_of_undos = MR::File::Config::get_int ("NumberOfUndos", 16);
        int ROI_Item::current_preset_colour = 0;
        int ROI_Item::new_roi_counter = 0;




        ROI_Item::ROI_Item (const MR::Image::Info& src) :
            Volume (src),
            saved (true),
            current_undo (-1)
        {
          type = gl::UNSIGNED_BYTE;
          format = gl::RED;
          internal_format = gl::R8;
          set_allowed_features (false, true, false);
          set_interpolate (false);
          set_use_transparency (true);
          set_min_max (0.0, 1.0);
          set_windowing (-1.0f, 0.0f);
          alpha = 1.0f;
          colour = preset_colours[current_preset_colour++];
          if (current_preset_colour >= 6)
            current_preset_colour = 0;
          transparent_intensity = 0.4f;
          opaque_intensity = 0.6f;
          colourmap = ColourMap::index ("Colour");
          float voxsize = std::min (src.vox(0), std::min (src.vox(1), src.vox(2)));
          brush_size = min_brush_size = voxsize;
          max_brush_size = 100.0f*min_brush_size;

          std::stringstream name;
          name << "ROI" << std::setfill('0') << std::setw(5) << new_roi_counter++ << ".mif";
          filename = name.str();

          bind();
          allocate();
        }



        void ROI_Item::zero () {
          bind();
          std::vector<GLubyte> data (info().dim(0)*info().dim(1));
          for (int n = 0; n < info().dim(2); ++n)
            upload_data ({ { 0, 0, n } }, { { info().dim(0), info().dim(1), 1 } }, reinterpret_cast<void*> (&data[0]));
        }



        void ROI_Item::load (const MR::Image::Header& header) {
          bind();
          MR::Image::Buffer<bool> buffer (header);
          auto vox = buffer.voxel();
          std::vector<GLubyte> data (vox.dim(0)*vox.dim(1));
          ProgressBar progress ("loading ROI image \"" + header.name() + "\"...");
          for (auto outer = MR::Image::Loop(2,3) (vox); outer; ++outer) {
            auto p = data.begin();
            for (auto inner = MR::Image::Loop (0,2) (vox); inner; ++inner)
              *(p++) = vox.value();
            upload_data ({ { 0, 0, vox[2] } }, { { vox.dim(0), vox.dim(1), 1 } }, reinterpret_cast<void*> (&data[0]));
            ++progress;
          }
          filename = header.name();
        }



        void ROI_Item::start (ROI_UndoEntry&& entry) {
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

        void ROI_Item::undo () {
          if (has_undo()) {
            undo_list[current_undo].undo (*this);
            --current_undo;
          }
        }

        void ROI_Item::redo () {
          if (has_redo()) {
            ++current_undo;
            undo_list[current_undo].redo (*this);
          }
        }




      }
    }
  }
}




