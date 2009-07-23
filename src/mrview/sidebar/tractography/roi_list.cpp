/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

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

#include "mrview/sidebar/tractography/roi_list.h"
#include "mrview/sidebar/tractography/track_list.h"
#include "mrview/sidebar/tractography.h"
#include "mrview/window.h"
#include "dialog/file.h"
#include <GL/glext.h>



namespace MR {
  namespace Viewer {
    namespace SideBar {




      ROIList::ROIList (const Tractography& sidebar) : 
        parent (sidebar) 
      {
        /*
           using namespace Gtk::Menu_Helpers;
           popup_menu.items().push_back (StockMenuElem(Gtk::Stock::OPEN, sigc::mem_fun(*this, &ROIList::on_open) ) );
           popup_menu.items().push_back (StockMenuElem(Gtk::Stock::CLOSE, sigc::mem_fun(*this, &ROIList::on_close) ) );
           popup_menu.items().push_back (SeparatorElem());
           popup_menu.items().push_back (StockMenuElem(Gtk::Stock::CLEAR, sigc::mem_fun(*this, &ROIList::on_clear) ) );
           popup_menu.accelerate (*this);
           */
        model = Gtk::ListStore::create (columns);
        set_model (model);

        append_column ("type", columns.type);
        append_column ("spec", columns.spec);

        set_headers_visible (false);
        get_selection()->signal_changed().connect (sigc::mem_fun (*this, &ROIList::on_selection));

        compile_circle();
        compile_sphere();
      }






      ROIList::~ROIList()  { }





      void ROIList::set (std::vector<RefPtr<DWI::Tractography::ROI> >& rois)
      {
        using namespace DWI::Tractography;
        for (std::vector<RefPtr<ROI> >::const_iterator i = rois.begin(); i != rois.end(); ++i) {
          Gtk::TreeModel::Row row = *(model->append());
          ROI& roi (**i);
          row[columns.type] = roi.type_description(); 
          row[columns.spec] = roi.mask.empty() ? str(roi.position) + ", rad " + str(roi.radius) : roi.mask;
          row[columns.roi] = *i;
        }
      }






      void ROIList::draw ()
      {
        Gtk::TreeModel::Children rois = model->children();
        if (rois.size() == 0) return;

        Pane& pane (Window::Main->pane());

        glDepthMask (GL_TRUE);
        glDisable (GL_BLEND);
        glBlendColor (1.0, 1.0, 1.0, 1.0);
        glEnableClientState (GL_VERTEX_ARRAY);

        if (pane.mode->type() == 0) glVertexPointer (2, GL_FLOAT, 2*sizeof(float), circle_vertices);

        for (Gtk::TreeModel::Children::iterator i = rois.begin(); i != rois.end(); ++i) {
          RefPtr<DWI::Tractography::ROI> roi = (*i)[columns.roi];

          glLineWidth (get_selection()->is_selected (*i) ? 2.0 : 1.0);
          //roi_pos[n].x = roi_pos[n].y = roi_pos[n].rad_x = -1;

          switch (roi->type) {
            case DWI::Tractography::ROI::Seed:    glColor3f (1.0, 1.0, 1.0); break;
            case DWI::Tractography::ROI::Include: glColor3f (0.0, 1.0, 0.3); break;
            case DWI::Tractography::ROI::Exclude: glColor3f (1.0, 0.0, 0.0); break;
            case DWI::Tractography::ROI::Mask:    glColor3f (1.0, 1.0, 0.0); break;
            default: assert (0);
          }

          const Slice::Current S (pane);

          if (pane.mode->type() == 0) {
            if (roi->mask.empty()) {
              const GLdouble* modelview = pane.get_modelview();

              Point normal (-modelview[2], -modelview[6], -modelview[10]);
              float dist = normal.dot (S.focus);
              dist -= normal.dot (roi->position);
              normal = roi->position + dist*normal;

              float radius2 = Math::pow2 (roi->radius);
              if (dist*dist < radius2) {
                //float thickness = 3.0;

                glEnable (GL_DEPTH_TEST);
                glMatrixMode (GL_MODELVIEW);
                glPushMatrix ();
                glTranslatef (normal[0], normal[1], normal[2]);

                GLdouble mv[16];
                glGetDoublev (GL_MODELVIEW_MATRIX, mv);
                mv[1] = mv[2] = mv[4] = mv[6] = mv[8] = mv[9] = 0.0;
                mv[0] = mv[5] = mv[10] = sqrt (radius2 - dist*dist);
                glLoadMatrixd (mv);
                glDrawArrays (GL_LINE_LOOP, 0, NUM_CIRCLE);
                glPopMatrix ();
                glDisable (GL_DEPTH_TEST);

                /*
                   if (editable && fabs(dist) < thickness) {
                   Point pos = pane.model_to_screen (roi->pos);
                   roi_pos[n].x = (long) (pos[0]+0.5);
                   roi_pos[n].y = (long) (pos[1]+0.5);
                   pos[0] += 1.0;
                   window().pane().screen_to_model (pos);
                   pos[0] -= track.roi[n].sphere().pos[0];
                   pos[1] -= track.roi[n].sphere().pos[1];
                   pos[2] -= track.roi[n].sphere().pos[2];
                   Math::normalise (pos);
                   pos[0] = track.roi[n].sphere().pos[0] + track.roi[n].sphere().radius * pos[0];
                   pos[1] = track.roi[n].sphere().pos[1] + track.roi[n].sphere().radius * pos[1];
                   pos[2] = track.roi[n].sphere().pos[2] + track.roi[n].sphere().radius * pos[2];
                   window().pane().model_to_screen (pos);
                   roi_pos[n].rad_x = (long) (pos[0]+0.5);

                   int w, h;
                   window().pane().GetClientSize (&w, &h);

                   glMatrixMode (GL_PROJECTION);
                   glPushMatrix (); 
                   glLoadIdentity (); 
                   glOrtho (0, w, 0, h, -1.0, 1.0);

                   glMatrixMode (GL_MODELVIEW);
                   glPushMatrix (); 
                   glLoadIdentity (); 

                   glDisable (GL_DEPTH_TEST);

                   glBegin (GL_LINE_STRIP);
                   glVertex2i (roi_pos[n].x-ROI_HANDLE_SIZE, roi_pos[n].y-ROI_HANDLE_SIZE);
                   glVertex2i (roi_pos[n].x-ROI_HANDLE_SIZE, roi_pos[n].y+ROI_HANDLE_SIZE);
                   glVertex2i (roi_pos[n].x+ROI_HANDLE_SIZE, roi_pos[n].y+ROI_HANDLE_SIZE);
                   glVertex2i (roi_pos[n].x+ROI_HANDLE_SIZE, roi_pos[n].y-ROI_HANDLE_SIZE);
                   glVertex2i (roi_pos[n].x-ROI_HANDLE_SIZE, roi_pos[n].y-ROI_HANDLE_SIZE);
                   glEnd();

                   glPopMatrix ();
                   glMatrixMode (GL_PROJECTION);
                   glPopMatrix ();
                   glMatrixMode (GL_MODELVIEW);
                   }
                   */
              }
            }
          }
          else {
            if (roi->mask.empty()) {
              glMatrixMode (GL_MODELVIEW);
              glPushMatrix ();
              glTranslatef (roi->position[0], roi->position[1], roi->position[2]);
              glScalef (roi->radius, roi->radius, roi->radius);
              //if (GL_sphere_list_id == 0) compile_sphere_list();
              //glCallList (GL_sphere_list_id);
              glPopMatrix ();
            }
          }
        }


        glDisableClientState (GL_VERTEX_ARRAY);
      }










      /*
         bool ROIList::on_button_press_event (GdkEventButton* event)
         {
         if ((event->type == GDK_BUTTON_PRESS) && (event->button == 3)) {
         Gtk::TreeModel::Path path;
         Gtk::TreeViewColumn* col;
         int x, y;
         bool is_row = get_path_at_pos ((int) event->x, (int) event->y, path, col, x, y);
         if (is_row) {
         if (!get_selection()->is_selected (path)) 
         TreeView::on_button_press_event(event);
         }
         else get_selection()->unselect_all();

         popup_menu.items()[1].set_sensitive (is_row);
         popup_menu.popup (event->button, event->time);
         return (true);
         }
         return (TreeView::on_button_press_event(event));
         }
         */






      void ROIList::on_selection () 
      {
        Gtk::TreeModel::iterator iter = get_selection()->get_selected ();
        if (!iter) return;
           
        RefPtr<DWI::Tractography::ROI> roi = (*iter)[columns.roi];
        if (roi->mask.empty()) {
          Slice::Current S (Window::Main->pane());
          S.focus = roi->position;
          Window::Main->update();
        }
      }





      void ROIList::compile_circle ()
      {
        for (uint x = 0; x < NUM_CIRCLE; x++) {
          float a = 2.0*M_PI*(float)x / (float)NUM_CIRCLE;
          circle_vertices[2*x]   = cos (a);
          circle_vertices[2*x+1] = sin (a);
        }
      }




      void ROIList::compile_sphere ()
      {
        /*
        for (uint x = 0; x < NUM_SPHERE; x++) {
          float cx = cos (M_PI*(float)x/(float)NUM_SPHERE);
          float sx = sin (M_PI*(float)x/(float)NUM_SPHERE);
          glBegin (GL_LINE_LOOP);
          for (uint y = 0; y < 2*NUM_SPHERE; y++) {
            float cy = cos (M_PI*(float)y/(float)NUM_SPHERE);
            float sy = sin (M_PI*(float)y/(float)NUM_SPHERE);
            glVertex3f (cx*sy, sx*sy, cy);
          }
          glEnd();
        }

        for (uint y = 1; y < NUM_SPHERE; y++) {
          float cy = cos (M_PI*(float)y/(float)NUM_SPHERE);
          float sy = sin (M_PI*(float)y/(float)NUM_SPHERE);
          glBegin (GL_LINE_LOOP);
          for (uint x = 0; x < 2*NUM_SPHERE; x++) {
            float cx = cos (M_PI*(float)x/(float)NUM_SPHERE);
            float sx = sin (M_PI*(float)x/(float)NUM_SPHERE);
            glVertex3f (cx*sy, sx*sy, cy);
          }
          glEnd();
        }
        */
      }







    }
  }
}



