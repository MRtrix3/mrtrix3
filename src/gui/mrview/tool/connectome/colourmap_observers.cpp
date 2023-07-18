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

#include "gui/mrview/tool/connectome/colourmap_observers.h"

#include "gui/mrview/tool/connectome/connectome.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {




        void NodeColourObserver::selected_colourmap (size_t index, const ColourMapButton&)
        {
          master.node_colourmap_index = index;
          master.calculate_node_colours();
          master.window().updateGL();
        }
        void NodeColourObserver::selected_custom_colour (const QColor& c, const ColourMapButton&)
        {
          master.node_fixed_colour = { c.red() / 255.0f, c.green() / 255.0f, c.blue() / 255.0f };
          master.calculate_node_colours();
          master.window().updateGL();
        }
        void NodeColourObserver::toggle_show_colour_bar (bool show, const ColourMapButton&)
        {
          master.show_node_colour_bar = show;
          master.window().updateGL();
        }
        void NodeColourObserver::toggle_invert_colourmap (bool invert, const ColourMapButton&)
        {
          master.node_colourmap_invert = invert;
          master.calculate_node_colours();
          master.window().updateGL();
        }
        void NodeColourObserver::reset_colourmap (const ColourMapButton&)
        {
          assert (master.node_values_from_file_colour.size());
          master.node_colour_lower_button->setValue (master.node_values_from_file_colour.get_min());
          master.node_colour_upper_button->setValue (master.node_values_from_file_colour.get_max());
          master.calculate_node_colours();
          master.window().updateGL();
        }





        void EdgeColourObserver::selected_colourmap (size_t index, const ColourMapButton&)
        {
          master.edge_colourmap_index = index;
          master.calculate_edge_colours();
          master.window().updateGL();
        }
        void EdgeColourObserver::selected_custom_colour (const QColor& c, const ColourMapButton&)
        {
          master.edge_fixed_colour = { c.red() / 255.0f, c.green() / 255.0f, c.blue() / 255.0f };
          master.calculate_edge_colours();
          master.window().updateGL();
        }
        void EdgeColourObserver::toggle_show_colour_bar (bool show, const ColourMapButton&)
        {
          master.show_edge_colour_bar = show;
          master.window().updateGL();
        }
        void EdgeColourObserver::toggle_invert_colourmap (bool invert, const ColourMapButton&)
        {
          master.edge_colourmap_invert = invert;
          master.calculate_edge_colours();
          master.window().updateGL();
        }
        void EdgeColourObserver::reset_colourmap (const ColourMapButton&)
        {
          assert (master.edge_values_from_file_colour.size());
          master.edge_colour_lower_button->setValue (master.edge_values_from_file_colour.get_min());
          master.edge_colour_upper_button->setValue (master.edge_values_from_file_colour.get_max());
          master.calculate_edge_colours();
          master.window().updateGL();
        }





      }
    }
  }
}





