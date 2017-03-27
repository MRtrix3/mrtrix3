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
#include "gui/mrview/colourmap_button.h"
#include "gui/mrview/colourmap.h"
#include "math/rng.h"


namespace MR
{
namespace GUI
{
namespace MRView
{


// FIXME Why is this duplicating colourmap code?
using Entry = ColourMap::Entry;
const std::vector<Entry> ColourMapButton::core_colourmaps_entries{{
    Entry ("Gray",
        "color.rgb = vec3 (amplitude);\n",
        [] (float amplitude) { return Eigen::Vector3f (amplitude, amplitude, amplitude); }),

    Entry ("RedBlackGreen",
        "color.rgb = scale*vec3 (offset-amplitude, amplitude-offset, 0.0);\n",
         Entry::basic_map_fn(), "color.r", true),

    Entry ("Hot",
        "color.rgb = vec3 (2.7213 * amplitude, 2.7213 * amplitude - 1.0, 3.7727 * amplitude - 2.7727);\n",
        [] (float amplitude) { return Eigen::Vector3f (2.7213 * amplitude, 2.7213 * amplitude - 1.0, 3.7727 * amplitude - 2.7727); }),

    Entry ("Cool",
        "color.rgb = 1.0 - (vec3 (2.7213 * (1.0 - amplitude), 2.7213 * (1.0 - amplitude) - 1.0, 3.7727 * (1.0 - amplitude) - 2.7727));\n",
        [] (float amplitude) { return Eigen::Vector3f (1.0 - (2.7213 * (1.0 - amplitude)), 1.0 - (2.7213 * (1.0 - amplitude) - 1.0), 1.0 - (3.7727 * (1.0 - amplitude) - 2.7727)); }),

    Entry ("Jet",
        "color.rgb = 1.5 - 4.0 * abs (1.0 - amplitude - vec3(0.25, 0.5, 0.75));\n",
        [] (float amplitude) { return Eigen::Vector3f (1.5 - 4.0 * std::abs (1.0 - amplitude - 0.25), 1.5 - 4.0 * std::abs (1.0 - amplitude - 0.5), 1.5 - 4.0 * std::abs (1.0 - amplitude - 0.75)); })
}};


const std::vector<Entry> ColourMapButton::special_colourmaps_entries{{
    Entry ("RGB",
           "color.rgb = scale * (abs(color.rgb) - offset);\n",
           Entry::basic_map_fn(),
           "length (color.rgb)", true),

    Entry ("Complex",
           "float phase = atan (color.r, color.g) * 0.954929658551372;\n"
           "color.rgb = phase + vec3 (-2.0, 0.0, 2.0);\n"
           "if (phase > 2.0) color.b -= 6.0;\n"
           "if (phase < -2.0) color.r += 6.0;\n"
           "color.rgb = clamp (scale * (amplitude - offset), 0.0, 1.0) * (2.0 - abs (color.rgb));\n",
           Entry::basic_map_fn(),
           "length (color.rg)", true)
}};


ColourMapButton::ColourMapButton(QWidget* parent, ColourMapButtonObserver& obs,
                                 bool use_shortcuts,
                                 bool use_special_colourmaps,
                                 bool use_customise_state_items) :
    QToolButton(parent),
    colourmap_actions(ColourMapButton::core_colourmaps_entries.size()),
    observer(obs),
    core_colourmaps_actions(new QActionGroup(parent))
{
    setToolTip(tr("Colourmap menu"));
    setIcon(QIcon(":/colourmap.svg"));
    setPopupMode(QToolButton::InstantPopup);

    init_menu(use_shortcuts, use_special_colourmaps, use_customise_state_items);
}


void ColourMapButton::init_core_menu_items(bool create_shortcuts)
{
    core_colourmaps_actions->setExclusive(true);

    size_t n = 0;
    for(const auto& colourmap_entry : ColourMapButton::core_colourmaps_entries)
    {
      QAction* action = new QAction(colourmap_entry.name, this);
      action->setCheckable(true);
      core_colourmaps_actions->addAction(action);

      colourmap_menu->addAction(action);
      addAction(action);

      if(create_shortcuts)
        action->setShortcut(QObject::tr(std::string ("Ctrl+" + str (n+1)).c_str()));

      colourmap_actions[n] = action;
      n++;
    }

    connect(core_colourmaps_actions, SIGNAL(triggered (QAction*)), this, SLOT(select_colourmap_slot(QAction*)));
    colourmap_actions[1]->setChecked(true);
}


void ColourMapButton::init_custom_colour_menu_items()
{
    custom_colour_action = new QAction(tr("Custom colour..."), this);
    custom_colour_action->setCheckable(true);
    connect(custom_colour_action, SIGNAL(triggered ()), this, SLOT(select_colour_slot()));

    core_colourmaps_actions->addAction(custom_colour_action);
    colourmap_menu->addAction(custom_colour_action);
    addAction(custom_colour_action);
    colourmap_actions.push_back(custom_colour_action);

    auto choose_random_colour = new QAction(tr("Random colour"), this);
    connect(choose_random_colour, SIGNAL(triggered ()), this, SLOT(select_random_colour_slot()));

    colourmap_menu->addAction(choose_random_colour);
    addAction(choose_random_colour);
}


void ColourMapButton::init_special_colour_menu_items(bool create_shortcuts)
{
    size_t n = colourmap_actions.size();
    for(const auto& colourmap_entry : ColourMapButton::special_colourmaps_entries)
    {
      QAction* action = new QAction(colourmap_entry.name, this);
      action->setCheckable(true);
      core_colourmaps_actions->addAction(action);

      colourmap_menu->addAction(action);
      addAction(action);

      if(create_shortcuts)
        action->setShortcut(QObject::tr(std::string ("Ctrl+" + str (n+1)).c_str()));

      colourmap_actions.push_back(action);
      n++;
    }
}

void ColourMapButton::init_customise_state_menu_items()
{
    auto show_colour_bar = colourmap_menu->addAction(tr("Show colour bar"), this, SLOT(show_colour_bar_slot(bool)));
    show_colour_bar->setCheckable(true);
    show_colour_bar->setChecked(true);
    addAction(show_colour_bar);

    auto invert_scale = colourmap_menu->addAction(tr("Invert"), this, SLOT(invert_colourmap_slot(bool)));
    invert_scale->setCheckable(true);
    addAction(invert_scale);

    QAction* reset_intensity = colourmap_menu->addAction(tr("Reset intensity"), this, SLOT(reset_intensity_slot()));
    addAction(reset_intensity);
}

void ColourMapButton::init_menu(bool create_shortcuts, bool use_special, bool customise_state)
{
    colourmap_menu = new QMenu(tr("Colourmap menu"), this);

    init_core_menu_items(create_shortcuts);
    init_custom_colour_menu_items();

    colourmap_menu->addSeparator();

    if(use_special)
    {
        init_special_colour_menu_items(create_shortcuts);
        colourmap_menu->addSeparator();
    }

    if(customise_state)
        init_customise_state_menu_items();

    setMenu(colourmap_menu);
}


void ColourMapButton::set_colourmap_index(size_t index)
{
    if(index < colourmap_actions.size())
    {
        QAction* action = colourmap_actions[index];
        action->setChecked(true);
        select_colourmap_slot(action);
    }
}


void ColourMapButton::select_colourmap_slot(QAction* action)
{
    auto begin = colourmap_actions.cbegin();
    auto end = colourmap_actions.cend();
    auto it = std::find(begin, end, action);

    if(it != end)
        observer.selected_colourmap(std::distance(begin, it), *this);
}


void ColourMapButton::select_colour_slot()
{
    QColor colour = QColorDialog::getColor(Qt::red, this, "Select Color", QColorDialog::DontUseNativeDialog);

    if (colour.isValid())
        observer.selected_custom_colour(colour, *this);
}


void ColourMapButton::select_random_colour_slot()
{
    size_t colour[3];
    Math::RNG rng;
    std::uniform_int_distribution<unsigned char> uniform_int;
    constexpr size_t max_half = std::numeric_limits<unsigned char>::max()/2;
    do {
      colour[0] = uniform_int(rng);
      colour[1] = uniform_int(rng);
      colour[2] = uniform_int(rng);
    } while (colour[0] < max_half && colour[1] < max_half && colour[2] < max_half);

    custom_colour_action->setChecked(true);

    select_colourmap_slot(custom_colour_action);
    observer.selected_custom_colour(QColor(colour[0],colour[1],colour[2]), *this);
}


void ColourMapButton::show_colour_bar_slot(bool visible)
{
    observer.toggle_show_colour_bar(visible, *this);
}


void ColourMapButton::invert_colourmap_slot(bool inverted)
{
    observer.toggle_invert_colourmap(inverted, *this);
}


void ColourMapButton::reset_intensity_slot()
{
    observer.reset_colourmap(*this);
}


}
}
}
