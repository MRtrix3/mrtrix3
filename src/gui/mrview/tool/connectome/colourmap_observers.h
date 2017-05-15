/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
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
      { MEMALIGN(NodeColourObserver)
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
      { MEMALIGN(EdgeColourObserver)
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




