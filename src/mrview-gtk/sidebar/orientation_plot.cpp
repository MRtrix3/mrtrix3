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


    24-07-2008 J-Donald Tournier <d.tournier@brain.org.au>
    * added support for overlay of orientation plot on main window

    09-07-2008 J-Donald Tournier <d.tournier@brain.org.au>
    * set color using config file

    08-07-2008 J-Donald Tournier <d.tournier@brain.org.au>
    * add option to disable tri-linear interpolation

    01-12-2008 J-Donald Tournier <d.tournier@brain.org.au>
    * fix problems with invalid focus position when the tool is first initialised.

*/

#include "file/config.h"
#include "dialog/file.h"
#include "mrview/window.h"
#include "mrview/sidebar/orientation_plot.h"

namespace MR {
  namespace Viewer {
    namespace SideBar {

      OrientationPlot::OrientationPlot () : 
        Base (5),
        source_frame ("source data"),
        source_button ("browse..."),
        lmax_label (" lmax: "),
        lod_label (" LoD: "),
        lmax_lod_table (2,2),
        align_with_viewer ("auto align with main window"),
        interpolate ("tri-linear interpolation"),
        show_axes ("show axes"),
        colour_by_direction ("colour by direction"),
        use_lighting ("use lighting"),
        hide_neg_lobes ("hide negative lobes"),
        show_overlay ("overlay"),
        lmax_adjustment (8, 2, 16, 2, 2),
        lod_adjustment (5, 2, 7, 1, 1),
        lmax (lmax_adjustment),
        lod (lod_adjustment),
        azimuth (NAN),
        elevation (NAN)
      { 
        
        frame.add (render);
        frame.set_shadow_type (Gtk::SHADOW_IN);

        settings_frame.add (settings);
        settings_frame.set_shadow_type (Gtk::SHADOW_IN);

        paned.pack1 (frame, true, false);
        paned.pack2 (settings_frame, true, true);

        source_box.pack_start (source_button, Gtk::PACK_SHRINK);
        source_frame.add (source_box);

        settings.pack_start (source_frame, Gtk::PACK_SHRINK);
        settings.pack_start (align_with_viewer, Gtk::PACK_SHRINK);
        settings.pack_start (interpolate, Gtk::PACK_SHRINK);
        settings.pack_start (show_axes, Gtk::PACK_SHRINK);
        settings.pack_start (colour_by_direction, Gtk::PACK_SHRINK);
        settings.pack_start (use_lighting, Gtk::PACK_SHRINK);
        settings.pack_start (hide_neg_lobes, Gtk::PACK_SHRINK);
        settings.pack_start (lmax_lod_table, Gtk::PACK_SHRINK);
        settings.pack_start (show_overlay, Gtk::PACK_SHRINK);

        lmax_lod_table.attach (lmax_label, 0, 1, 0, 1, Gtk::SHRINK | Gtk::FILL);
        lmax_lod_table.attach (lmax, 1, 2, 0, 1, Gtk::SHRINK | Gtk::FILL);
        lmax_lod_table.attach (lod_label, 0, 1, 1, 2, Gtk::SHRINK | Gtk::FILL);
        lmax_lod_table.attach (lod, 1, 2, 1, 2, Gtk::SHRINK | Gtk::FILL);

        source_button.set_tooltip_text ("set the image that contains the source data");
        source_button.set_tooltip_text ("set the image that contains the source data");
        lmax_label.set_tooltip_text ("maximum spherical harmonic order");
        lod_label.set_tooltip_text ("level of detail");

        pack_start (paned);

        show_all();

        Window::Main->pane().activate (this);

        align_with_viewer.signal_toggled().connect (sigc::mem_fun (*this, &OrientationPlot::set_projection));
        interpolate.signal_toggled().connect (sigc::mem_fun (*this, &OrientationPlot::set_values));
        show_axes.signal_toggled().connect (sigc::mem_fun (*this, &OrientationPlot::on_show_axes));
        colour_by_direction.signal_toggled().connect (sigc::mem_fun (*this, &OrientationPlot::on_colour_by_direction));
        use_lighting.signal_toggled().connect (sigc::mem_fun (*this, &OrientationPlot::on_use_lighting));
        hide_neg_lobes.signal_toggled().connect (sigc::mem_fun (*this, &OrientationPlot::on_hide_negative_lobes));
        show_overlay.signal_toggled().connect (sigc::mem_fun (*this, &OrientationPlot::on_show_overlay));

        lmax.signal_value_changed().connect (sigc::mem_fun (*this, &OrientationPlot::on_lmax));
        lod.signal_value_changed().connect (sigc::mem_fun (*this, &OrientationPlot::on_lod));

        source_button.signal_clicked().connect (sigc::mem_fun (*this, &OrientationPlot::on_source_browse));

        align_with_viewer.set_active (true);
        interpolate.set_active (true);
        show_axes.set_active (true);
        colour_by_direction.set_active (true);
        use_lighting.set_active (true);
        hide_neg_lobes.set_active (true);
        show_overlay.set_active (false);
        lod.set_value (5);

        std::string string;
        string = File::Config::get ("OrientationPlot.Color");
        if (string.size()) { 
          try {
            std::vector<float> V (parse_floats (string));
            if (V.size() < 3) throw Exception ("invalid configuration key \"SurfacePlot.Color\" - ignored");
            render.color[0] = V[0];
            render.color[1] = V[1];
            render.color[2] = V[2];
          }
          catch (Exception) { }
        }

        if (align_with_viewer.get_active()) set_projection ();
        Slice::Current S (Window::Main->pane());
        focus = S.focus;
        set_values();
      }

      

      OrientationPlot::~OrientationPlot () {  }



      void OrientationPlot::draw ()
      { 
        Slice::Current S (Window::Main->pane());
        if (align_with_viewer.get_active()) set_projection ();
        on_show_overlay();
        if (focus == S.focus) return;
        focus = S.focus;
        set_values();
      }


      void OrientationPlot::on_show_axes () { render.set_show_axes (show_axes.get_active()); }
      void OrientationPlot::on_colour_by_direction () { render.set_color_by_dir (colour_by_direction.get_active()); refresh_overlay(); }
      void OrientationPlot::on_use_lighting () { render.set_use_lighting (use_lighting.get_active()); refresh_overlay(); }

      void OrientationPlot::on_hide_negative_lobes () { render.set_hide_neg_lobes (hide_neg_lobes.get_active()); refresh_overlay(); }
      void OrientationPlot::on_lod () { render.set_LOD (int (lod.get_value())); refresh_overlay(); }
      void OrientationPlot::on_lmax () { render.set_lmax (int (lmax.get_value())); refresh_overlay(); }


      void OrientationPlot::on_show_overlay () 
      {  
        overlay_pane = &Window::Main->pane();
        const Slice::Current S (*overlay_pane);

        if (!image_object || !show_overlay.get_active() || S.orientation || !S.image) {
          idle_connection.disconnect();
          return;
        }

        MR::Image::Interp& interp (*S.image->interp);
        int ix, iy;
        Slice::get_fixed_axes (S.projection, ix, iy);

        Point f = overlay_pane->model_to_screen (S.focus);
        f[0] = 0.0; 
        f[1] = 0.0;
        Point pos (interp.R2P (overlay_pane->screen_to_model (f)));
        overlay_bounds[0][0] = round (pos[ix]);
        overlay_bounds[0][1] = round (pos[iy]);
        overlay_slice = round (pos[S.projection]);

        f[0] = overlay_pane->width(); 
        f[1] = overlay_pane->height();
        pos = interp.R2P (overlay_pane->screen_to_model (f));
        overlay_bounds[1][0] = round (pos[ix]);
        overlay_bounds[1][1] = round (pos[iy]);

        overlay_pos[0] = overlay_bounds[0][0];
        overlay_pos[1] = overlay_bounds[0][1];

        overlay_render.precompute (int(lmax.get_value()), int(lod.get_value()), get_toplevel()->get_window());
        idle_connection = Glib::signal_idle().connect (sigc::mem_fun (*this, &OrientationPlot::on_idle));
      }



      bool OrientationPlot::on_idle ()
      {
        if (!image_object) return (false);
        const Slice::Current S (*overlay_pane);
        if (!S.image) return (false);

        int ix, iy;
        Slice::get_fixed_axes (S.projection, ix, iy);
        Point pos;
        pos[S.projection] = overlay_slice;
        pos[ix] = overlay_pos[0];
        pos[iy] = overlay_pos[1];

        Point spos (S.image->interp->P2R (pos));

        std::vector<float> values;
        get_values (values, spos);

        if (values.size()) {
          overlay_pane->gl_start();
          glPushAttrib (GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);

          glDrawBuffer (GL_FRONT);
          glDepthMask (GL_TRUE);
          glEnable (GL_DEPTH_TEST);
          glDisable (GL_BLEND);

          glPushMatrix();
          glLoadIdentity();
          render.do_reset_lighting();
          glPopMatrix();
          glPushMatrix();
          overlay_render.calculate (values, int (lmax.get_value()), render.get_hide_neg_lobes());

          if (render.get_use_lighting()) glEnable (GL_LIGHTING);
          GLfloat v[] = { 0.9, 0.9, 0.9, 1.0 }; 
          glMaterialfv (GL_BACK, GL_AMBIENT_AND_DIFFUSE, v);

          glTranslatef (spos[0], spos[1], spos[2]);
          glScalef (render.get_scale(), render.get_scale(), render.get_scale()); 

          overlay_render.draw (render.get_use_lighting(), render.get_color_by_dir() ? NULL : render.color);

          glPopAttrib ();
          glPopMatrix();
          glFlush();
          overlay_pane->gl_end();
        }

        overlay_pos[0] += overlay_bounds[1][0] > overlay_bounds[0][0] ? 1 : -1;
        if ((overlay_pos[0] - overlay_bounds[0][0]) * (overlay_pos[0] - overlay_bounds[1][0]) > 0) {
          overlay_pos[0] = overlay_bounds[0][0];
          overlay_pos[1] += overlay_bounds[1][1] > overlay_bounds[0][1] ? 1 : -1;
          if ((overlay_pos[1] - overlay_bounds[0][1]) * (overlay_pos[1] - overlay_bounds[1][1]) > 0) 
            return (false);
        }
        return (true);
      }



      void OrientationPlot::on_source_browse ()
      {
        Dialog::File dialog ("Select source data", false, true);

        if (dialog.run() == Gtk::RESPONSE_OK) {
          std::vector<RefPtr<MR::Image::Object> > selection = dialog.get_images();
          if (selection.size()) {
            image_object = selection[0];
            source_button.set_label (Glib::path_get_basename (image_object->name()));
            source_button.set_tooltip_text ("set the image that contains the source data\n(currently set to:\n\"" + image_object->name() + "\")");
            interp = new MR::Image::Interp (*image_object);
            lmax.set_value (DWI::SH::LforN (interp->dim(3)));
            set_values();
          }
        }
      }



      void OrientationPlot::set_projection () 
      {
        const GLdouble* mv (Window::Main->pane().get_modelview());

        memset (rotation, 0, 16*sizeof(GLfloat));
        rotation[0] = mv[0]; rotation[1] = mv[1]; rotation[2] = mv[2];
        rotation[4] = mv[4]; rotation[5] = mv[5]; rotation[6] = mv[6];
        rotation[8] = mv[8]; rotation[9] = mv[9]; rotation[10] = mv[10];
        rotation[15] = 1.0;

        if (align_with_viewer.get_active()) render.set_rotation (rotation);
        else render.set_rotation ();
      }


      void OrientationPlot::get_values (std::vector<float>& values, const Point& position)
      {
        if (!position) { values.clear(); return; }

        if (interpolate.get_active()) {
          interp->R (position);
          if (! (!*interp)) {
            values.resize (interp->dim(3));
            for (interp->set(3,0); (*interp)[3] < interp->dim(3); interp->inc(3)) values[(*interp)[3]] = interp->value();
          }
        }
        else {
          MR::Image::Position& pos (*interp);
          Point P (interp->R2P (position));
          pos.set (0, round (P[0]));
          pos.set (1, round (P[1]));
          pos.set (2, round (P[2]));
          pos.set (3, 0);
          if (! (!pos)) {
            values.resize (pos.dim(3));
            for (; pos[3] < pos.dim(3); pos.inc(3)) values[pos[3]] = pos.value();
          }
        }
      }



      void OrientationPlot::set_values () 
      {
        if (!image_object) return;
        std::vector<float> values;
        get_values (values, focus);

        render.set_show_axes (show_axes.get_active());
        render.set_hide_neg_lobes (hide_neg_lobes.get_active());
        render.set_color_by_dir (colour_by_direction.get_active());
        render.set_use_lighting (use_lighting.get_active());
        render.set_LOD (int (lod.get_value()));
        render.set_lmax (int (lmax.get_value()));
        set_projection ();
        render.set (values);
      }


    }
  }
}

