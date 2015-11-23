/*
   Copyright 2014 Brain Research Institute, Melbourne, Australia

   Written by Robert E. Smith, 2015.

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

#ifndef __gui_mrview_tool_connectome_colourmap_observers_h__
#define __gui_mrview_tool_connectome_colourmap_observers_h__

#include "gui/mrview/colourmap_button.h"
#include "gui/opengl/gl.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {

      class Connectome;

      // Classes to receive input from the colourmap buttons and act accordingly
      class NodeColourObserver : public ColourMapButtonObserver
      {
        public:
          NodeColourObserver (Connectome& connectome) : master (connectome) { }
          void selected_colourmap (size_t, const ColourMapButton&) override;
          void selected_custom_colour (const QColor&, const ColourMapButton&) override;
          void toggle_show_colour_bar (bool, const ColourMapButton&) override;
          void toggle_invert_colourmap (bool, const ColourMapButton&) override;
          void reset_colourmap (const ColourMapButton&) override;
        private:
          Connectome& master;
      };
      class EdgeColourObserver : public ColourMapButtonObserver
      {
        public:
          EdgeColourObserver (Connectome& connectome) : master (connectome) { }
          void selected_colourmap (size_t, const ColourMapButton&) override;
          void selected_custom_colour (const QColor&, const ColourMapButton&) override;
          void toggle_show_colour_bar (bool, const ColourMapButton&) override;
          void toggle_invert_colourmap (bool, const ColourMapButton&) override;
          void reset_colourmap (const ColourMapButton&) override;
        private:
          Connectome& master;
      };

      }
    }
  }
}

#endif




