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


    15-10-2008 J-Donald Tournier <d.tournier@brain.org.au>
    * remove MR::DICOM_DW_gradients_PRS flag

*/

#include <gdkmm/cursor.h>
#include <gtkmm/stock.h>
#include <gtkmm/dialog.h>

#include "dialog/file.h"
#include "image/format/list.h"
#include "image/mapper.h"
#include "image/object.h"
#include "file/dicom/quick_scan.h"
#include "file/dicom/image.h"
#include "file/dicom/series.h"
#include "file/dicom/study.h"
#include "file/dicom/patient.h"
#include "file/dicom/mapper.h"

// TODO: volumes under Windows

namespace MR {
  namespace Dialog {


    std::string File::cwd;
    int File::window_position_x = -1; 
    int File::window_position_y = -1; 
    int File::window_size_x = 500; 
    int File::window_size_y = 500; 




    File::File (const std::string& message, bool multiselection, bool images_only) :
      Gtk::Dialog (message, true, false),
      up_button (Gtk::Stock::GO_UP),
      home_button (Gtk::Stock::HOME),
      refresh_button (Gtk::Stock::REFRESH),
      path_label ("Path"),
      selection_label ("Selection"),
      dir (NULL),
      filter_images (images_only),
      updating_selection (false)
    {
      add_button (Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
      add_button (Gtk::Stock::OK, Gtk::RESPONSE_OK);

      button_box.set_spacing (GUI_SPACING);
      button_box.set_homogeneous ();
      button_box.pack_start (up_button);
      button_box.pack_start (home_button);
      button_box.pack_start (refresh_button);

      path_box.set_spacing (GUI_SPACING);
      path_box.pack_start (path_label, Gtk::PACK_SHRINK);
      path_box.pack_start (path_entry);

      folder_list = Gtk::ListStore::create (folder_columns);
      folder_list->set_sort_column (folder_columns.name, Gtk::SORT_ASCENDING);
      folders.set_model (folder_list);
      folders.append_column ("Name", folder_columns.name);

      folders_viewport.add (folders);
      folders_viewport.set_policy (Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
      folders_viewport.set_shadow_type (Gtk::SHADOW_IN);

      file_list = Gtk::ListStore::create (file_columns);
      file_list->set_sort_column (file_columns.name, Gtk::SORT_ASCENDING);
      files.set_model (file_list);
      files.set_has_tooltip();
      if (multiselection) files.get_selection()->set_mode(Gtk::SELECTION_MULTIPLE);
      files.append_column ("Name", file_columns.name);
      files_viewport.add (files);
      files_viewport.set_shadow_type (Gtk::SHADOW_IN);
      files_viewport.set_policy (Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

      splitter.pack1 (folders_viewport, true, true);
      splitter.pack2 (files_viewport, true, true);

      selection_box.set_spacing (GUI_SPACING);
      selection_box.pack_start (selection_label, Gtk::PACK_SHRINK);
      selection_box.pack_start (selection_entry);

      main_box.set_border_width (GUI_SPACING);
      main_box.set_spacing (GUI_SPACING);
      main_box.pack_start (button_box, Gtk::PACK_SHRINK);
      main_box.pack_start (path_box, Gtk::PACK_SHRINK);
      main_box.pack_start (splitter);
      main_box.pack_start (selection_box, Gtk::PACK_SHRINK);

      get_vbox()->pack_start (main_box);

      up_button.signal_clicked().connect (sigc::mem_fun (*this, &File::on_up));
      home_button.signal_clicked().connect (sigc::mem_fun (*this, &File::on_home));
      refresh_button.signal_clicked().connect (sigc::mem_fun (*this, &File::on_refresh));
      path_entry.signal_activate().connect (sigc::mem_fun (*this, &File::on_path));

      folders.signal_row_activated().connect (sigc::mem_fun (*this, &File::on_folder_selected));
      files.signal_row_activated().connect (sigc::mem_fun (*this, &File::on_file_selected));
      files.signal_query_tooltip().connect (sigc::mem_fun (*this, &File::on_file_tooltip));
      files.get_selection()->signal_changed().connect (sigc::mem_fun (*this, &File::on_file_highlighted));
      selection_entry.signal_changed().connect (sigc::mem_fun (*this, &File::on_selection_entry));
      selection_entry.signal_activate().connect (sigc::mem_fun (*this, &File::on_selection_activated));

      if (!cwd.size()) cwd = Path::cwd();
      show_all_children();
      realize();
      move (window_position_x, window_position_y);
      resize (window_size_x, window_size_y);

      update();
    }





    File::~File ()
    {
      get_position (window_position_x, window_position_y);
      get_size (window_size_x, window_size_y);
    }




    void File::update () 
    {
      folder_list->clear();
      file_list->clear();
      selection_entry.set_text ("");
      dicom_tree.clear();
      idle_connection.disconnect();
      path_entry.set_text (cwd);
      folders_read = false;

      try {
        if (dir) delete dir;
        dir = new Path::Dir (cwd);
      }
      catch (...) {
        error ("error reading folder \"" + cwd + "\"");
        return;
      }

      get_window()->set_cursor (Gdk::Cursor (Gdk::WATCH));
      idle_connection = Glib::signal_idle().connect (sigc::mem_fun (*this, &File::on_idle));
    }





    void File::on_up ()
    {
      cwd = Path::dirname (cwd);
      update();
    }



    void File::on_home ()
    {
      cwd = Path::home();
      update();
    }



    void File::on_refresh ()
    {
      update();
    }



    void File::on_path ()
    {
      if (Path::is_file (path_entry.get_text()))
        cwd = path_entry.get_text();
      update();
    }





    void File::on_folder_selected (const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column)
    {
      Gtk::TreeModel::iterator iter = folder_list->get_iter (path);
      if (iter) {
        cwd = Path::join (cwd, (*iter)[folder_columns.name]);
        update();
      }
    }




    void File::on_file_selected (const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column)
    {
      response (Gtk::RESPONSE_OK);
    }


    void File::on_selection_activated ()
    {
      response (Gtk::RESPONSE_OK);
    }



    void File::on_file_highlighted ()
    {
      if (updating_selection) updating_selection = false;
      else {
        if (files.get_selection()->get_mode() == Gtk::SELECTION_MULTIPLE) {
          std::vector<Gtk::TreePath> rows (files.get_selection()->get_selected_rows());
          if (rows.size()) {
            updating_selection = true;
            if (rows.size() == 1) {
              std::string name ((*file_list->get_iter (rows[0]))[file_columns.name]);
              selection_entry.set_text (name);
            }
            else selection_entry.set_text ("");
          }
        }
        else {
          Gtk::TreeModel::iterator iter = files.get_selection()->get_selected();
          if (file_list->iter_is_valid (iter)) { 
            updating_selection = true;
            std::string name ((*iter)[file_columns.name]);
            selection_entry.set_text (name);
          }
        }
      }
    }





    void File::on_selection_entry ()
    {
      if (updating_selection) updating_selection = false;
      else {
        updating_selection = true;
        files.get_selection()->unselect_all();
      }
    }




    bool File::on_idle ()
    {
      std::string entry;
      for (uint n = 0; n < 10; n++) {
        entry = dir->read_name();

        if (!entry.size()) {
          if (folders_read) {
            get_window()->set_cursor();
            update_dicom ();
            return (false);
          }
          else {
            folders_read = true;
            dir->rewind();
            return (true);
          }
        }

        if (entry[0] != '.') {
          std::string path (Path::join (cwd, entry));
          if (folders_read) {
            if (Path::is_file (path)) {
              if (filter_images) check_image (path, entry);
              else {
                Gtk::TreeModel::Row row = *(file_list->append());
                row[file_columns.name] = entry;
              }
            }
          }
          else if (Path::is_dir (Path::join (cwd, entry))) {
            Gtk::TreeModel::Row row = *(folder_list->append());
            row[folder_columns.name] = entry;
          }
        }
      }


      update_dicom ();

      return (true);
    }





    bool File::on_file_tooltip (int x, int y, bool keyboard_tooltip, const Glib::RefPtr<Gtk::Tooltip>& tooltip)
    {
      Gtk::TreeModel::iterator iter;
      if (!files.get_tooltip_context_iter (x, y, keyboard_tooltip, iter)) return (false);

      RefPtr<MR::File::Dicom::Series> series ((*iter)[file_columns.series]);
      if (!series) return (false);


      std::string text = "patient: " + series->study->patient->name 
        + "\n\tDOB: " + MR::File::Dicom::format_date (series->study->patient->DOB) + "\n\tID: " + series->study->patient->ID
        + "\nstudy: " + series->study->name + "\n\tdate: " + MR::File::Dicom::format_date (series->study->date) + " at "
        + MR::File::Dicom::format_time (series->study->time) +"\n\tID: " + series->study->ID
        + "\nseries " + str (series->number) + ": " + series->name + "\n\t" + str (series->size()) + " images\n\tdate: " 
        + MR::File::Dicom::format_date (series->date) + " at " + MR::File::Dicom::format_time (series->time);
      
      tooltip->set_text (text);
      return (true);
    }





    inline void File::check_image (const std::string& path, const std::string& base)
    {
      for (const char** ext = Image::Format::known_extensions; *ext; ext++) {
        if (Path::has_suffix (base, *ext)) {
          Gtk::TreeModel::Row row = *(file_list->append());
          row[file_columns.name] = base;
          return;
        }
      }
      check_dicom (path, base);
    }







    void File::check_dicom (const std::string& path, const std::string& base)
    {
      Exception::Lower _ES (1);
      MR::File::Dicom::QuickScan reader;
      if (reader.read (path)) return;

      RefPtr<MR::File::Dicom::Patient> patient = dicom_tree.find (reader.patient, reader.patient_ID, reader.patient_DOB);
      RefPtr<MR::File::Dicom::Study> study = patient->find (reader.study, reader.study_ID, reader.study_date, reader.study_time);
      RefPtr<MR::File::Dicom::Series> series = study->find (reader.series, reader.series_number, reader.modality, reader.series_date, reader.series_time);

      RefPtr<MR::File::Dicom::Image> image (new MR::File::Dicom::Image);
      image->filename = path;
      image->series = series.get();
      image->sequence_name = reader.sequence;
      series->push_back (image);

      Gtk::TreeModel::Row row;
      Gtk::TreeModel::Children children = file_list->children();
      for (Gtk::TreeModel::Children::iterator iter = children.begin(); iter != children.end(); ++iter) {
        row = *iter;
        RefPtr<MR::File::Dicom::Series> series_in_list = row[file_columns.series];
        if (series_in_list == series) return;
      }

      row = *(file_list->append());
      row[file_columns.series] = series;
    }






    void File::update_dicom ()
    {
      Gtk::TreeModel::Children children = file_list->children();
      for (Gtk::TreeModel::Children::iterator iter = children.begin(); iter != children.end(); ++iter) {
        Gtk::TreeModel::Row row = *iter;
        RefPtr<MR::File::Dicom::Series> series (row[file_columns.series]);
        if (series.get()) {
          row[file_columns.name] = "[" + str (series->number) + "] " + series->name + ": " + str (series->size()) + " images (" + series->study->patient->name 
            + " - " + MR::File::Dicom::format_date (series->date) + ")";
        }
      }
    }






    std::vector<std::string> File::get_selection ()
    {
      std::vector<std::string> sel;
      std::vector<Gtk::TreePath> rows (files.get_selection()->get_selected_rows());
      if (rows.size()) {
        for (std::vector<Gtk::TreePath>::iterator it = rows.begin(); it != rows.end(); ++it) {
          std::string name ((*file_list->get_iter (*it))[file_columns.name]);
          sel.push_back (Path::join (cwd, name));
        }
      }
      else {
        std::string name (strip (selection_entry.get_text()));
        if (name.size()) sel.push_back (Path::join (cwd, name));
      }
      return (sel);
    }





    std::vector<RefPtr<Image::Object> > File::get_images ()
    {
      std::vector<RefPtr<Image::Object> > V;
      std::vector<Gtk::TreePath> rows (files.get_selection()->get_selected_rows());
      if (rows.size()) {
        for (std::vector<Gtk::TreePath>::iterator it = rows.begin(); it != rows.end(); ++it) {
          Gtk::TreeModel::Row row = *file_list->get_iter (*it);
          RefPtr<MR::File::Dicom::Series> series (row[file_columns.series]);
          try {
            RefPtr<Image::Object> ima (new Image::Object);

            if (series.get()) {
              std::vector< RefPtr<MR::File::Dicom::Series> > series_v;
              series_v.push_back (series);
              MR::File::Dicom::dicom_to_mapper (ima->M, ima->H, series_v);
              ima->setup();
            }
            else ima->open (Path::join (cwd, row[file_columns.name]));

            V.push_back (ima);
          }
          catch (...) { }
        }
      }
      else {
        try {
          RefPtr<Image::Object> ima (new Image::Object);
          ima->open (Path::join (cwd, strip (selection_entry.get_text())));
          V.push_back (ima);
        }
        catch (...) { }
      }

      return (V);
    }



  }
}


