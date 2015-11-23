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





