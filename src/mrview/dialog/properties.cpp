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

#include <gtkmm/stock.h>

#include "image/object.h"
#include "mrview/dialog/properties.h"
#include "mrview/pane.h"
#include "mrview/window.h"


namespace MR {
  namespace Viewer {

    PropertiesDialog::PropertiesDialog (const MR::Image::Object& image) : Gtk::Dialog ("Image Properties", true, false)
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

      const MR::Image::Header& H (image.header());

      Gtk::TreeModel::Row row = *(model->append());
      row[columns.key] = "File";
      row[columns.value] = H.name;

      row = *(model->append());
      row[columns.key] = "Format";
      row[columns.value] = H.format;

      if (H.comments.size()) {
        row = *(model->append());
        row[columns.key] = "Comments";
        row[columns.value] = H.comments[0];
        for (uint n = 1; n < H.comments.size(); n++) {
          Gtk::TreeModel::Row childrow = *(model->append (row.children()));
          childrow[columns.value] = H.comments[n];
        }
      }

      row = *(model->append());
      row[columns.key] = "Dimensions";
      if (!H.axes.ndim()) row[columns.value] = "none";
      else {
        std::string s (str (H.axes.dim[0]));
        for (int n = 1; n < H.axes.ndim(); n++) s += " x " + str (H.axes.dim[n]);
        row[columns.value] = s;
      }

      row = *(model->append());
      row[columns.key] = "Voxel size";
      if (H.axes.ndim()) {
        std::string s (str (H.axes.vox[0]));
        for (int n = 1; n < H.axes.ndim(); n++) s += " x " + str (H.axes.vox[n]);
        row[columns.value] = s;
      }


      row = *(model->append());
      row[columns.key] = "Dimension labels";
      for (int n = 0; n < H.axes.ndim(); n++) {
        Gtk::TreeModel::Row childrow = *(model->append (row.children()));
        childrow[columns.key] = "axis " + str (n);
        childrow[columns.value] = ( H.axes.desc[n].size() ? H.axes.desc[n] : "undefined" ) + " (" + ( H.axes.units[n].size() ? H.axes.units[n] : "?" ) + ")";
      }

      row = *(model->append());
      row[columns.key] = "Data type";
      row[columns.value] = H.data_type.description();

      row = *(model->append());
      row[columns.key] = "Data layout";
      if (H.axes.ndim()) {
        std::string s;
        for (int n = 0; n < H.axes.ndim(); n++) s += H.axes.axis[n] == MR::Image::Axis::undefined ? "? " : ( H.axes.forward[n] ? '+' : '-' ) + str (H.axes.axis[n]) + " ";
        row[columns.value] = s;
      }

      row = *(model->append());
      row[columns.key] = "Data scaling";
      row[columns.value] = "offset: " + str (H.offset) + ", multiplier = " + str (H.scale);


      row = *(model->append());
      row[columns.key] = "Transform";
      if (!H.transform().is_valid()) row[columns.value] = "unspecified";
      else {
        row[columns.value] = "4 x 4";
        for (uint i = 0; i < H.transform().rows(); i++) {
          Gtk::TreeModel::Row childrow = *(model->append (row.children()));
          std::string s;
          for (uint j = 0; j < H.transform().columns(); j++) s += str (H.transform()(i,j)) + " ";
          childrow[columns.value] = s;
        }
      }

      if (H.DW_scheme.is_valid()) {
        row = *(model->append());
        row[columns.key] = "DW scheme";
        row[columns.value] = str (H.DW_scheme.rows()) + " x " + str (H.DW_scheme.columns());
      }

      show_all_children();
    }


  }
}

