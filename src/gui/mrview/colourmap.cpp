/*
   Copyright 2010 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 13/11/09.

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

#include "gui/mrview/colourmap.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace ColourMap
      {

        const char* Entry::default_amplitude = "color.r";



        const Entry maps[] = {
          Entry ("Gray", 
              "color.rgb = vec3 (amplitude);\n"),

          Entry ("Hot", 
              "color.rgb = vec3 (2.7213 * amplitude, 2.7213 * amplitude - 1.0, 3.7727 * amplitude - 2.7727);\n"),

          Entry ("Jet", 
              "color.rgb = 1.5 - 4.0 * abs (amplitude - vec3(0.25, 0.5, 0.75));\n"),

          Entry ("RGB", 
              "color.rgb = scale * (abs(color.rgb) - offset);\n",
              "length (color.rgb)", 
              true),

          Entry ("Complex", 
              "float phase = atan (color.r, color.g) * 0.954929658551372;\n"
              "color.rgb = phase + vec3 (-2.0, 0.0, 2.0);\n"
              "if (phase > 2.0) color.b -= 6.0;\n"
              "if (phase < -2.0) color.r += 6.0;\n"
              "color.rgb = clamp (scale * (amplitude - offset), 0.0, 1.0) * (2.0 - abs (color.rgb));\n", 
              "length (color.rg)", 
              true),

          Entry (NULL, NULL, NULL, true)
        };





        void create_menu (QWidget* parent, QActionGroup*& group, QMenu* menu, QAction** & actions, bool create_shortcuts)
        {
          group = new QActionGroup (parent);
          group->setExclusive (true);
          actions = new QAction* [num()];
          bool in_scalar_section = true;

          for (size_t n = 0; maps[n].name; ++n) {
            QAction* action = new QAction (maps[n].name, parent);
            action->setCheckable (true);
            group->addAction (action);

            if (maps[n].special && in_scalar_section) {
              menu->addSeparator();
              in_scalar_section = false;
            }

            menu->addAction (action);
            parent->addAction (action);

            if (create_shortcuts)
              action->setShortcut (QObject::tr (std::string ("Ctrl+" + str (n+1)).c_str()));

            actions[n] = action;
          }

          actions[0]->setChecked (true);
        }


      }
    }
  }
}

