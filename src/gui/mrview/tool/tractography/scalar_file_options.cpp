/*
    Copyright 2013 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt, 01/02/13.

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

//#include <QPushButton>
//#include <QLabel>
//#include <QLayout>
//#include <QSlider>
//#include <QStyle>
#include <QGroupBox>
#include <QGridLayout>

#include "gui/mrview/tool/tractography/scalar_file_options.h"
#include "math/vector.h"
#include "gui/dialog/lighting.h"
#include "gui/mrview/colourmap.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {

        ScalarFileOptions::ScalarFileOptions (Window& main_window, Dock* parent) :
          Base (main_window, parent)
        {
          main_box = new QVBoxLayout (this);
          main_box->setContentsMargins (5, 5, 5, 5);
          main_box->setSpacing (5);

          QHBoxLayout* hlayout = new QHBoxLayout;
          hlayout->setContentsMargins (0, 0, 0, 0);
          hlayout->setSpacing (0);

          button = new QPushButton (this);
          button->setToolTip (tr ("Open scalar track file"));
          connect (button, SIGNAL (clicked()), this, SLOT (open_track_scalar_file_slot ()));
          hlayout->addWidget (button);


          // Colourmap menu:

          colourmap_menu = new QMenu (tr ("Colourmap menu"), this);

          ColourMap::create_menu (this, colourmap_group, colourmap_menu, colourmap_actions);
          connect (colourmap_group, SIGNAL (triggered (QAction*)), this, SLOT (select_colourmap_slot()));
          colourmap_actions[1]->setChecked (true);

          colourmap_menu->addSeparator();

          show_colour_bar = colourmap_menu->addAction (tr ("Show colour bar"), this, SLOT (show_colour_bar_slot()));
          show_colour_bar->setCheckable (true);
          show_colour_bar->setChecked (true);
          addAction (show_colour_bar);

//            colourmap_menu->addSeparator();
//
//            invert_scale_action = colourmap_menu->addAction (tr ("Invert scaling"), this, SLOT (invert_scaling_slot()));
//            invert_scale_action->setCheckable (true);
//            invert_scale_action->setShortcut (tr("U"));
//            addAction (invert_scale_action);
//
//
//            invert_colourmap_action = colourmap_menu->addAction (tr ("Invert Colourmap"), this, SLOT (invert_colourmap_slot()));
//            invert_colourmap_action->setCheckable (true);
//            invert_colourmap_action->setShortcut (tr("Shift+U"));
//            addAction (invert_colourmap_action);
//
//            colourmap_menu->addSeparator();
//
//            reset_windowing_action = colourmap_menu->addAction (tr ("Reset brightness/contrast"), this, SLOT (image_reset_slot()));
//            reset_windowing_action->setShortcut (tr ("Esc"));
//            addAction (reset_windowing_action);

          colourmap_button = new QToolButton (this);
          colourmap_button->setToolTip (tr ("Colourmap menu"));
          colourmap_button->setIcon (QIcon (":/colourmap.svg"));
          colourmap_button->setPopupMode (QToolButton::InstantPopup);
          colourmap_button->setMenu (colourmap_menu);
          hlayout->addWidget (colourmap_button);

          main_box->addLayout (hlayout);

          QGroupBox* group_box = new QGroupBox ("Scaling");
          QGridLayout* layout = new QGridLayout;
          main_box->addWidget (group_box);
          group_box->setLayout (layout);

          layout->addWidget (new QLabel ("min"), 0, 0);
          min_entry = new AdjustButton (this);
          connect (min_entry, SIGNAL (valueChanged()), this, SLOT (on_set_scaling_slot()));
          layout->addWidget (min_entry, 0, 1);

          layout->addWidget (new QLabel ("max"), 1, 0);
          max_entry = new AdjustButton (this);
          connect (max_entry, SIGNAL (valueChanged()), this, SLOT (on_set_scaling_slot()));
          layout->addWidget (max_entry, 1, 1);

          threshold_box = new QGroupBox ("Thresholds");
          threshold_box->setCheckable (true);
          threshold_box->setChecked (false);
          connect (threshold_box, SIGNAL (toggled(bool)), this, SLOT (toggle_threshold_slot()));
          layout = new QGridLayout;
          main_box->addWidget (threshold_box);
          threshold_box->setLayout (layout);

          layout->addWidget (new QLabel (">"), 0, 0);
          lessthan = new AdjustButton (this);
          connect (lessthan, SIGNAL (valueChanged()), this, SLOT (on_set_threshold_slot()));
          layout->addWidget (lessthan, 0, 1);

          layout->addWidget (new QLabel ("<"), 1, 0);
          greaterthan = new AdjustButton (this);
          connect (greaterthan, SIGNAL (valueChanged()), this, SLOT (on_set_threshold_slot()));
          layout->addWidget (greaterthan, 1, 1);

          main_box->addStretch ();
          setMinimumSize (main_box->minimumSize());
        }


        void ScalarFileOptions::set_tractogram (Tractogram* selected_tractogram) {
          tractogram = selected_tractogram;
          if (tractogram && tractogram->color_type == ScalarFile) {
            button->setEnabled (true);
            min_entry->setEnabled (true);
            max_entry->setEnabled (true);
            min_entry->setRate (tractogram->scaling_rate());
            max_entry->setRate (tractogram->scaling_rate());
            threshold_box->setEnabled (true);
            colourmap_menu->setEnabled (true);
            greaterthan->setRate (tractogram->scaling_rate());
            lessthan->setRate (tractogram->scaling_rate());
            if (threshold_box->isChecked()) {
              greaterthan->setEnabled (true);
              lessthan->setEnabled (true);
            } else {
              lessthan->setEnabled (false);
              greaterthan->setEnabled (false);
            }
            if (tractogram->scalar_filename.length()) {
              button->setText (shorten (tractogram->scalar_filename, 35, 0).c_str());
              min_entry->setValue (tractogram->scaling_min());
              max_entry->setValue (tractogram->scaling_max());
              toggle_threshold_slot ();
            } else {
              button->setText ("");
            }
          } else {
            button->setText ("");
            button->setEnabled (false);
            min_entry->setEnabled (false);
            max_entry->setEnabled (false);
            min_entry->clear();
            max_entry->clear();
            threshold_box->setEnabled (false);
            greaterthan->clear();
            lessthan->clear();
            colourmap_menu->setEnabled (false);
          }
        }


        bool ScalarFileOptions::open_track_scalar_file_slot ()
        {
          Dialog::File dialog (this, "Select track scalar to open", false, false);
          if (dialog.exec()) {
            std::vector<std::string> list;
            dialog.get_selection (list);
            try {
              tractogram->load_track_scalars (list[0]);
              set_tractogram (tractogram);
            } catch (Exception& E) {
              E.display();
              return false;
            }
            return true;
          }
          return false;
        }


        void ScalarFileOptions::show_colour_bar_slot ()
        {
          if (tractogram) {
            tractogram->show_colour_bar = show_colour_bar->isChecked();
            window.updateGL();
          }
        }


        void ScalarFileOptions::select_colourmap_slot ()
        {
          if (tractogram) {
            QAction* action = colourmap_group->checkedAction();
            size_t n = 0;
            while (action != colourmap_actions[n])
              ++n;
            tractogram->set_colourmap (n);
            window.updateGL();
          }
        }



        void ScalarFileOptions::on_set_scaling_slot ()
        {
          if (tractogram) {
            tractogram->set_windowing (min_entry->value(), max_entry->value());
            window.updateGL();
          }
        }


        void ScalarFileOptions::toggle_threshold_slot() {
          if (threshold_box->isChecked()) {
            greaterthan->setEnabled (true);
            lessthan->setEnabled (true);
            tractogram->do_threshold = true;
          } else {
            greaterthan->setEnabled (false);
            lessthan->setEnabled (false);
            tractogram->do_threshold = false;
          }
          greaterthan->setValue (tractogram->greaterthan);
          lessthan->setValue (tractogram->lessthan);
          tractogram->recompile();
          window.updateGL();
        }


        void ScalarFileOptions::on_set_threshold_slot ()
        {
          tractogram->set_thresholds (lessthan->value(), greaterthan->value());
          window.updateGL();
        }


//        void ScalarFileOptions::on_scaling_changed_slot ()
//        {
//          if (window.image()) {
//            min_entry->setValue (window.image()->scaling_min());
//            max_entry->setValue (window.image()->scaling_max());
//            set_scaling_rate();
//          }
//        }
      }
    }
  }
}
