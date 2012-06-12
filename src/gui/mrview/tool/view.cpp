/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

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

#include <QLabel>
#include <QGridLayout>
#include <QLineEdit>
#include <QDoubleValidator>

#include "mrtrix.h"
#include "gui/mrview/window.h"
#include "gui/mrview/mode/base.h"
#include "gui/mrview/tool/view.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {


        View::View (Dock* parent) : 
          Base (parent) {
            QVBoxLayout* main_box = new QVBoxLayout (this);

            projection_combobox = new QComboBox;
            projection_combobox->insertItem (0, "Sagittal");
            projection_combobox->insertItem (1, "Coronal");
            projection_combobox->insertItem (2, "Axial");
            main_box->addWidget (projection_combobox);
            connect (projection_combobox, SIGNAL (activated(int)), this, SLOT (onSetProjection(int)));

            QGroupBox* group_box = new QGroupBox ("Focus");
            QGridLayout* layout = new QGridLayout;
            main_box->addWidget (group_box);
            group_box->setLayout (layout);

            QDoubleValidator* validator = new QDoubleValidator (this);

            layout->addWidget (new QLabel ("x"), 0, 0);
            focus_x = new QLineEdit;
            focus_x->setValidator (validator);
            connect (focus_x, SIGNAL (editingFinished()), this, SLOT (onSetFocus()));
            layout->addWidget (focus_x, 0, 1);

            layout->addWidget (new QLabel ("y"), 1, 0);
            focus_y = new QLineEdit;
            focus_y->setValidator (validator);
            connect (focus_y, SIGNAL (editingFinished()), this, SLOT (onSetFocus()));
            layout->addWidget (focus_y, 1, 1);

            layout->addWidget (new QLabel ("z"), 2, 0);
            focus_z = new QLineEdit;
            focus_z->setValidator (validator);
            connect (focus_z, SIGNAL (editingFinished()), this, SLOT (onSetFocus()));
            layout->addWidget (focus_z, 2, 1);



            group_box = new QGroupBox ("Scaling");
            layout = new QGridLayout;
            main_box->addWidget (group_box);
            group_box->setLayout (layout);

            layout->addWidget (new QLabel ("min"), 0, 0);
            min_entry = new QLineEdit;
            connect (min_entry, SIGNAL (editingFinished()), this, SLOT (onSetScaling()));
            layout->addWidget (min_entry, 0, 1);

            layout->addWidget (new QLabel ("max"), 1, 0);
            max_entry = new QLineEdit;
            connect (max_entry, SIGNAL (editingFinished()), this, SLOT (onSetScaling()));
            layout->addWidget (max_entry, 1, 1);


            main_box->addStretch ();
        }

        void View::showEvent (QShowEvent* event) 
        {
          connect (&window(), SIGNAL (focusChanged()), this, SLOT (onFocusChanged()));
          connect (&window(), SIGNAL (projectionChanged()), this, SLOT (onProjectionChanged()));
          connect (&window(), SIGNAL (scalingChanged()), this, SLOT (onScalingChanged()));
          onFocusChanged();
        }

        void View::closeEvent (QCloseEvent* event) 
        {
          window().disconnect (this);
        }

        void View::onFocusChanged () 
        {
          focus_x->setText (str(window().focus()[0]).c_str());
          focus_y->setText (str(window().focus()[1]).c_str());
          focus_z->setText (str(window().focus()[2]).c_str());
        }

        void View::onSetFocus () 
        {
          try {
            window().set_focus (Point<> (
                  to<float> (focus_x->text().toStdString()), 
                  to<float> (focus_y->text().toStdString()), 
                  to<float> (focus_z->text().toStdString())));
            window().updateGL();
          }
          catch (Exception) { }
        }


        void View::onProjectionChanged () 
        {
          projection_combobox->setCurrentIndex (window().projection());
        }


        void View::onSetProjection (int index) 
        {
          window().set_projection (index);
          window().updateGL();
        }


        void View::onSetScaling ()
        {
          if (window().image()) {
            try {
              window().image()->set_windowing (
                  to<float> (min_entry->text().toStdString()), 
                  to<float> (max_entry->text().toStdString()));
              window().updateGL();
            }
            catch (Exception) { }
          }
        }



        void View::onScalingChanged ()
        {
          if (window().image()) {
            min_entry->setText (str (window().image()->scaling_min()).c_str());
            max_entry->setText (str (window().image()->scaling_max()).c_str());
          }
        }



      }
    }
  }
}






