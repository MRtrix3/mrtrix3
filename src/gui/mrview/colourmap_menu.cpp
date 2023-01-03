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

#include "gui/gui.h"
#include "gui/mrview/colourmap_menu.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {



      void create_colourmap_menu (QWidget* parent, QActionGroup*& group,
                                  QMenu* menu, QAction** & actions,
                                  bool create_shortcuts, bool use_special)
      {
        group = new QActionGroup (parent);
        group->setExclusive (true);
        actions = new QAction* [ColourMap::num()];
        bool in_scalar_section = true;

        for (size_t n = 0; ColourMap::maps[n].name; ++n) {
          if (ColourMap::maps[n].special && !use_special)
            continue;
          QAction* action = new QAction (ColourMap::maps[n].name, parent);
          action->setCheckable (true);
          group->addAction (action);

          if (ColourMap::maps[n].special && in_scalar_section) {
            menu->addSeparator();
            in_scalar_section = false;
          }

          menu->addAction (action);
          parent->addAction (action);

          if (create_shortcuts)
            action->setShortcut (qstr ("Ctrl+" + str (n+1)));

          actions[n] = action;
        }

        actions[0]->setChecked (true);
      }



    }
  }
}

