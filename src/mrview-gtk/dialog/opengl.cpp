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

#include <gdk/gdkgl.h>
#include <gtkmm/stock.h>

#include "mrview/dialog/opengl.h"
#include "mrview/pane.h"
#include "mrview/window.h"


namespace MR {
  namespace Viewer {

    OpenGLInfo::OpenGLInfo () : Gtk::Dialog ("OpenGL Info", true, false)
    {
      set_border_width (5);
      set_default_size (400, 300);
      add_button (Gtk::Stock::OK, Gtk::RESPONSE_OK);

      scrolled_window.add (tree);
      scrolled_window.set_policy (Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
      scrolled_window.set_shadow_type (Gtk::SHADOW_IN);

      get_vbox()->pack_start (scrolled_window);

      model = Gtk::TreeStore::create (columns);
      tree.set_model (model);

      tree.append_column ("Parameter", columns.key);
      tree.append_column ("Value", columns.value);


      if (!Viewer::Window::Main->pane().gl_start()) {
        int major, minor;
        gdk_gl_query_version (&major, &minor);

        Gtk::TreeModel::Row row = *(model->append());
        row[columns.key] = "API Version";
        row[columns.value] = str(major) + "." + str(minor);

        row = *(model->append());
        row[columns.key] = "Renderer";
        row[columns.value] =  (const char*) glGetString (GL_RENDERER);

        row = *(model->append());
        row[columns.key] = "Vendor";
        row[columns.value] =  (const char*) glGetString (GL_VENDOR);

        Gtk::TreeModel::Row childrow = *(model->append (row.children()));
        childrow[columns.key] = "Version";
        childrow[columns.value] =  (const char*) glGetString (GL_VERSION);

        row = *(model->append());
        row[columns.key] = "Bit depths";

        int i;
        glGetIntegerv (GL_RED_BITS, &i); 
        childrow = *(model->append (row.children()));
        childrow[columns.key] = "red";
        childrow[columns.value] = str (i);

        glGetIntegerv (GL_GREEN_BITS, &i); 
        childrow = *(model->append (row.children()));
        childrow[columns.key] = "green";
        childrow[columns.value] = str (i);

        glGetIntegerv (GL_BLUE_BITS, &i); 
        childrow = *(model->append (row.children()));
        childrow[columns.key] = "blue";
        childrow[columns.value] = str (i);

        glGetIntegerv (GL_ALPHA_BITS, &i); 
        childrow = *(model->append (row.children()));
        childrow[columns.key] = "alpha";
        childrow[columns.value] = str (i);

        glGetIntegerv (GL_DEPTH_BITS, &i); 
        childrow = *(model->append (row.children()));
        childrow[columns.key] = "depth";
        childrow[columns.value] = str (i);

        glGetIntegerv (GL_STENCIL_BITS, &i); 
        childrow = *(model->append (row.children()));
        childrow[columns.key] = "stencil";
        childrow[columns.value] = str (i);

        row = *(model->append());
        row[columns.key] = "Buffers";

        glGetIntegerv (GL_DOUBLEBUFFER, &i); 
        childrow = *(model->append (row.children()));
        childrow[columns.key] = "Double buffering";
        childrow[columns.value] = i ? "on" : "off";

        glGetIntegerv (GL_STEREO, &i); 
        childrow = *(model->append (row.children()));
        childrow[columns.key] = "Stereo buffering";
        childrow[columns.value] = i ? "on" : "off";

        glGetIntegerv (GL_AUX_BUFFERS, &i); 
        childrow = *(model->append (row.children()));
        childrow[columns.key] = "Auxilliary buffers";
        childrow[columns.value] = str(i);


        glGetIntegerv (GL_MAX_TEXTURE_SIZE, &i); 
        row = *(model->append());
        row[columns.key] = "Maximum texture size";
        row[columns.value] = str(i);

        glGetIntegerv (GL_MAX_LIGHTS, &i); 
        row = *(model->append());
        row[columns.key] = "Lights";
        row[columns.value] = str(i);

        glGetIntegerv (GL_MAX_CLIP_PLANES, &i); 
        row = *(model->append());
        row[columns.key] = "Clip planes";
        row[columns.value] = str(i);

        Viewer::Window::Main->pane().gl_end();
      }

      show_all_children();
    }


  }
}

