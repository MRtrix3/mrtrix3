/* Copyright (c) 2008-2017 the MRtrix3 contributors
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


#include "gui/mrview/tool/tractography/track_scalar_file.h"
#include "gui/dialog/file.h"
#include "gui/mrview/colourmap.h"
#include "gui/mrview/tool/tractography/tractogram.h"


namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {

        TrackScalarFileOptions::TrackScalarFileOptions (QWidget* parent) :
            QGroupBox ("Scalar file options", parent),
            tractogram (nullptr)
        {
          main_box = new Tool::Base::VBoxLayout (this);

          colour_groupbox = new QGroupBox ("Colour map and scaling");
          Tool::Base::VBoxLayout* vlayout = new Tool::Base::VBoxLayout;
          vlayout->setContentsMargins (0, 0, 0, 0);
          vlayout->setSpacing (0);
          colour_groupbox->setLayout (vlayout);

          Tool::Base::HBoxLayout* hlayout = new Tool::Base::HBoxLayout;
          hlayout->setContentsMargins (0, 0, 0, 0);
          hlayout->setSpacing (0);

          intensity_file_button = new QPushButton (this);
          intensity_file_button->setToolTip (tr ("Open (track) scalar file for colouring streamlines"));
          connect (intensity_file_button, SIGNAL (clicked()), this, SLOT (open_intensity_track_scalar_file_slot ()));
          hlayout->addWidget (intensity_file_button);

          // Colourmap menu:
          colourmap_menu = new QMenu (tr ("Colourmap menu"), this);

          ColourMap::create_menu (this, colourmap_group, colourmap_menu, colourmap_actions, false, false);
          connect (colourmap_group, SIGNAL (triggered (QAction*)), this, SLOT (select_colourmap_slot()));
          colourmap_actions[1]->setChecked (true);

          colourmap_menu->addSeparator();

          show_colour_bar = colourmap_menu->addAction (tr ("Show colour bar"), this, SLOT (show_colour_bar_slot()));
          show_colour_bar->setCheckable (true);
          show_colour_bar->setChecked (true);
          addAction (show_colour_bar);

          invert_scale = colourmap_menu->addAction (tr ("Invert"), this, SLOT (invert_colourmap_slot()));
          invert_scale->setCheckable (true);
          addAction (invert_scale);

          colourmap_menu->addSeparator();

          QAction* reset_intensity = colourmap_menu->addAction (tr ("Reset intensity"), this, SLOT (reset_intensity_slot()));
          addAction (reset_intensity);

          colourmap_button = new QToolButton (this);
          colourmap_button->setToolTip (tr ("Colourmap menu"));
          colourmap_button->setIcon (QIcon (":/colourmap.svg"));
          colourmap_button->setPopupMode (QToolButton::InstantPopup);
          colourmap_button->setMenu (colourmap_menu);
          hlayout->addWidget (colourmap_button);

          vlayout->addLayout (hlayout);

          hlayout = new Tool::Base::HBoxLayout;
          hlayout->setContentsMargins (0, 0, 0, 0);
          hlayout->setSpacing (0);

          min_entry = new AdjustButton (this);
          connect (min_entry, SIGNAL (valueChanged()), this, SLOT (on_set_scaling_slot()));
          hlayout->addWidget (min_entry);

          max_entry = new AdjustButton (this);
          connect (max_entry, SIGNAL (valueChanged()), this, SLOT (on_set_scaling_slot()));
          hlayout->addWidget (max_entry);

          vlayout->addLayout (hlayout);

          main_box->addWidget (colour_groupbox);

          QGroupBox* threshold_box = new QGroupBox ("Thresholds");
          vlayout = new Tool::Base::VBoxLayout;
          vlayout->setContentsMargins (0, 0, 0, 0);
          vlayout->setSpacing (0);
          threshold_box->setLayout (vlayout);

          threshold_file_combobox = new QComboBox (this);
          threshold_file_combobox->addItem ("None");
          threshold_file_combobox->addItem ("Use colour scalar file");
          threshold_file_combobox->addItem ("Separate scalar file");
          connect (threshold_file_combobox, SIGNAL (activated(int)), this, SLOT (threshold_scalar_file_slot (int)));
          vlayout->addWidget (threshold_file_combobox);

          hlayout = new Tool::Base::HBoxLayout;
          hlayout->setContentsMargins (0, 0, 0, 0);
          hlayout->setSpacing (0);

          threshold_lower_box = new QCheckBox (this);
          connect (threshold_lower_box, SIGNAL (stateChanged(int)), this, SLOT (threshold_lower_changed(int)));
          hlayout->addWidget (threshold_lower_box);
          threshold_lower = new AdjustButton (this, 0.1);
          connect (threshold_lower, SIGNAL (valueChanged()), this, SLOT (threshold_lower_value_changed()));
          hlayout->addWidget (threshold_lower);

          threshold_upper_box = new QCheckBox (this);
          hlayout->addWidget (threshold_upper_box);
          threshold_upper = new AdjustButton (this, 0.1);
          connect (threshold_upper_box, SIGNAL (stateChanged(int)), this, SLOT (threshold_upper_changed(int)));
          connect (threshold_upper, SIGNAL (valueChanged()), this, SLOT (threshold_upper_value_changed()));
          hlayout->addWidget (threshold_upper);

          vlayout->addLayout (hlayout);

          main_box->addWidget (threshold_box);

          update_UI();
        }


        void TrackScalarFileOptions::set_tractogram (Tractogram* selected_tractogram) {
          tractogram = selected_tractogram;
        }



        void TrackScalarFileOptions::render_tractogram_colourbar(const Tractogram &tractogram) {

          float min_value = (tractogram.get_threshold_type() == TrackThresholdType::UseColourFile
                             && tractogram.use_discard_lower()) ?
              tractogram.scaling_min_thresholded() :
              tractogram.scaling_min();

          float max_value = (tractogram.get_threshold_type() == TrackThresholdType::UseColourFile
                             && tractogram.use_discard_upper()) ?
              tractogram.scaling_max_thresholded() :
              tractogram.scaling_max();

          window().colourbar_renderer.render (tractogram.colourmap, tractogram.scale_inverted(),
                                              min_value, max_value,
                                              tractogram.scaling_min(), tractogram.display_range, tractogram.colour);
        }



        void TrackScalarFileOptions::update_UI () {

          if (!tractogram) {
            setVisible (false);
            return;
          }
          setVisible (true);

          if (tractogram->get_color_type() == TrackColourType::ScalarFile) {

            colour_groupbox->setVisible (true);
            min_entry->setRate (tractogram->scaling_rate());
            max_entry->setRate (tractogram->scaling_rate());
            min_entry->setValue (tractogram->scaling_min());
            max_entry->setValue (tractogram->scaling_max());

            threshold_lower_box->setEnabled (true);
            if (tractogram->use_discard_lower()) {
              threshold_lower_box->setChecked(true);
              threshold_lower->setEnabled (true);
            } else {
              threshold_lower->setEnabled (false);
              threshold_lower_box->setChecked(false);
            }
            threshold_upper_box->setEnabled (true);
            if (tractogram->use_discard_upper()) {
              threshold_upper_box->setChecked (true);
              threshold_upper->setEnabled (true);
            } else {
              threshold_upper->setEnabled (false);
              threshold_upper_box->setChecked (false);
            }
            threshold_lower->setRate (tractogram->scaling_rate());

            colourmap_menu->setEnabled (true);
            colourmap_actions[tractogram->colourmap]->setChecked (true);
            show_colour_bar->setChecked (tractogram->show_colour_bar);
            invert_scale->setChecked (tractogram->scale_inverted());

            assert (tractogram->intensity_scalar_filename.length());
            intensity_file_button->setText (shorten (Path::basename (tractogram->intensity_scalar_filename), 35, 0).c_str());

          } else {
            colour_groupbox->setVisible (false);
          }

          threshold_file_combobox->removeItem (3);
          threshold_file_combobox->blockSignals (true);
          switch (tractogram->get_threshold_type()) {
            case TrackThresholdType::None:
              threshold_file_combobox->setCurrentIndex (0);
              break;
            case TrackThresholdType::UseColourFile:
              threshold_file_combobox->setCurrentIndex (1);
              break;
            case TrackThresholdType::SeparateFile:
              assert (tractogram->threshold_scalar_filename.length());
              threshold_file_combobox->addItem (shorten (Path::basename (tractogram->threshold_scalar_filename), 35, 0).c_str());
              threshold_file_combobox->setCurrentIndex (3);
              break;
          }
          threshold_file_combobox->blockSignals (false);

          const bool show_threshold_controls = (tractogram->get_threshold_type() != TrackThresholdType::None);
          threshold_lower_box->setVisible (show_threshold_controls);
          threshold_lower    ->setVisible (show_threshold_controls);
          threshold_upper_box->setVisible (show_threshold_controls);
          threshold_upper    ->setVisible (show_threshold_controls);

          if (show_threshold_controls) {
            threshold_lower_box->setChecked (tractogram->use_discard_lower());
            threshold_lower    ->setEnabled (tractogram->use_discard_lower());
            threshold_upper_box->setChecked (tractogram->use_discard_upper());
            threshold_upper    ->setEnabled (tractogram->use_discard_upper());
            threshold_lower->setRate  (tractogram->scaling_rate());
            threshold_lower->setValue (tractogram->lessthan);
            threshold_upper->setRate  (tractogram->scaling_rate());
            threshold_upper->setValue (tractogram->greaterthan);
          }
        }



        bool TrackScalarFileOptions::open_intensity_track_scalar_file_slot ()
        {
          std::string scalar_file = Dialog::File::get_file (this, "Select scalar text file or Track Scalar file (.tsf) to open", "");
          if (!scalar_file.empty()) {
            try {
              tractogram->load_intensity_track_scalars (scalar_file);
              tractogram->set_color_type (TrackColourType::ScalarFile);
            }
            catch (Exception& E) {
              E.display();
              scalar_file.clear();
            }
          }
          update_UI();
          window().updateGL();
          return scalar_file.size();
        }


        void TrackScalarFileOptions::show_colour_bar_slot ()
        {
          if (tractogram) {
            tractogram->show_colour_bar = show_colour_bar->isChecked();
            window().updateGL();
          }
        }


        void TrackScalarFileOptions::select_colourmap_slot ()
        {
          if (tractogram) {
            QAction* action = colourmap_group->checkedAction();
            size_t n = 0;
            while (action != colourmap_actions[n])
              ++n;
            tractogram->colourmap = n;
            window().updateGL();
          }
        }


        void TrackScalarFileOptions::on_set_scaling_slot ()
        {
          if (tractogram) {
            tractogram->set_windowing (min_entry->value(), max_entry->value());
            window().updateGL();
          }
        }


        bool TrackScalarFileOptions::threshold_scalar_file_slot (int /*unused*/)
        {

          std::string file_path;
          switch (threshold_file_combobox->currentIndex()) {
            case 0:
              tractogram->set_threshold_type (TrackThresholdType::None);
              tractogram->erase_threshold_scalar_data();
              tractogram->set_use_discard_lower (false);
              tractogram->set_use_discard_upper (false);
              break;
            case 1:
              if (tractogram->get_color_type() == TrackColourType::ScalarFile) {
                tractogram->set_threshold_type (TrackThresholdType::UseColourFile);
                tractogram->erase_threshold_scalar_data();
              } else {
                QMessageBox::warning (QApplication::activeWindow(),
                                      tr ("Tractogram threshold error"),
                                      tr ("Can only threshold based on scalar file used for streamline colouring if that colour mode is active"),
                                      QMessageBox::Ok,
                                      QMessageBox::Ok);
                threshold_file_combobox->blockSignals (true);
                switch (tractogram->get_threshold_type()) {
                  case TrackThresholdType::None: threshold_file_combobox->setCurrentIndex (0); break;
                  case TrackThresholdType::UseColourFile: assert (0);
                  case TrackThresholdType::SeparateFile: threshold_file_combobox->setCurrentIndex (3); break;
                }
                threshold_file_combobox->blockSignals (false);
                return false;
              }
              break;
            case 2:
              file_path = Dialog::File::get_file (this, "Select scalar text file or Track Scalar file (.tsf) to open", "");
              if (!file_path.empty()) {
                try {
                  tractogram->load_threshold_track_scalars (file_path);
                  tractogram->set_threshold_type (TrackThresholdType::SeparateFile);
                } catch (Exception& E) {
                  E.display();
                  file_path.clear();
                }
              }
              if (file_path.empty()) {
                threshold_file_combobox->blockSignals (true);
                switch (tractogram->get_threshold_type()) {
                  case TrackThresholdType::None:
                    threshold_file_combobox->setCurrentIndex (0);
                    break;
                  case TrackThresholdType::UseColourFile:
                    threshold_file_combobox->setCurrentIndex (1);
                    break;
                  case TrackThresholdType::SeparateFile:
                    // Should still be an entry in the combobox corresponding to the old file
                    threshold_file_combobox->setCurrentIndex (3);
                    break;
                }
                threshold_file_combobox->blockSignals (false);
                return false;
              }
              break;
            case 3: // Re-selected the same file as used previously; do nothing
              assert (tractogram->get_threshold_type() == TrackThresholdType::SeparateFile);
              break;
            default:
              assert (0);
          }
          update_UI();
          window().updateGL();
          return true;
        }


        void TrackScalarFileOptions::threshold_lower_changed (int)
        {
          if (tractogram) {
            threshold_lower->setEnabled (threshold_lower_box->isChecked());
            tractogram->set_use_discard_lower (threshold_lower_box->isChecked());
            window().updateGL();
          }
        }


        void TrackScalarFileOptions::threshold_upper_changed (int)
        {
          if (tractogram) {
            threshold_upper->setEnabled (threshold_upper_box->isChecked());
            tractogram->set_use_discard_upper (threshold_upper_box->isChecked());
            window().updateGL();
          }
        }


        void TrackScalarFileOptions::threshold_lower_value_changed ()
        {
          if (tractogram && threshold_lower_box->isChecked()) {
            tractogram->lessthan = threshold_lower->value();
            window().updateGL();
          }
        }


        void TrackScalarFileOptions::threshold_upper_value_changed ()
        {
          if (tractogram && threshold_upper_box->isChecked()) {
            tractogram->greaterthan = threshold_upper->value();
            window().updateGL();
          }
        }


        void TrackScalarFileOptions::reset_intensity_slot ()
        {
          if (tractogram) {
            tractogram->reset_windowing();
            update_UI ();
            window().updateGL();
          }
        }


        void TrackScalarFileOptions::invert_colourmap_slot ()
        {
          if (tractogram) {
            tractogram->set_invert_scale (invert_scale->isChecked());
            window().updateGL();
          }
        }

      }
    }
  }
}
