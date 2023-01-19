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

#ifndef __gui_mrview_colourmap_button_h__
#define __gui_mrview_colourmap_button_h__

#include "mrtrix.h"

#include "colourmap.h"
#include "gui/opengl/gl.h"

namespace MR
{
namespace GUI
{
namespace MRView
{

class ColourMapButton;

class ColourMapButtonObserver
{ NOMEMALIGN
public:
    virtual void selected_colourmap(size_t, const ColourMapButton&) {}
    virtual void selected_custom_colour(const QColor&, const ColourMapButton&) {}
    virtual void toggle_show_colour_bar(bool, const ColourMapButton&) {}
    virtual void toggle_invert_colourmap(bool, const ColourMapButton&) {}
    virtual void reset_colourmap(const ColourMapButton&) {}
};


class ColourMapButton : public QToolButton
{ MEMALIGN(ColourMapButton)
    Q_OBJECT
public:
    ColourMapButton(QWidget* parent, ColourMapButtonObserver& obs,
                    bool use_shortcuts = false,
                    bool use_special_colourmaps = true,
                    bool use_customise_state_items = true);
    void set_colourmap_index(size_t index);
    void set_scale_inverted(bool yesno);
    void set_fixed_colour();
    vector<QAction*> colourmap_actions;
    void open_menu (const QPoint& p) { colourmap_menu->exec (p); }
private:
    void init_menu(bool create_shortcuts, bool use_special, bool customise_state);
    void init_core_menu_items(bool create_shortcuts);
    void init_custom_colour_menu_items();
    void init_special_colour_menu_items(bool create_shortcuts);
    void init_customise_state_menu_items();

    static const vector<ColourMap::Entry> core_colourmaps_entries;
    static const vector<ColourMap::Entry> special_colourmaps_entries;

    ColourMapButtonObserver& observer;
    QActionGroup *core_colourmaps_actions;

    QMenu* colourmap_menu;
    QAction* custom_colour_action;
    QAction* invert_scale_action;

    size_t fixed_colour_index;


private slots:
    void select_colourmap_slot(QAction* action);
    void select_colour_slot();
    void select_random_colour_slot();
    void show_colour_bar_slot(bool visible);
    void invert_colourmap_slot(bool inverted);
    void reset_intensity_slot();
};

}
}
}

#endif // __gui_mrview_colourmap_button_h__
