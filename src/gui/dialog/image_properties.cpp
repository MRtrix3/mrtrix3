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

#include "image/header.h"
#include "gui/dialog/file.h"
#include "gui/dialog/list.h"
#include "gui/dialog/image_properties.h"


namespace MR
{
  namespace GUI
  {
    namespace Dialog
    {

      ImageProperties::ImageProperties (QWidget* parent, const Image::Header& header) :
        QDialog (parent), H (header), save_target (NULL)
      {
        model = new TreeModel (this);

        TreeItem* root = model->rootItem;

        root->appendChild (new TreeItem ("File", H.name(), root));
        root->appendChild (new TreeItem ("Format", H.format(), root));

        if (H.comments().size()) {
          TreeItem* comments = new TreeItem ("Comments", std::string(), root);
          root->appendChild (comments);
          for (size_t n = 0; n < H.comments().size(); ++n)
            comments->appendChild (new TreeItem (std::string(), H.comments() [n], comments));
        }

        std::string text;
        text = str (H.dim (0));
        for (size_t n = 1; n < H.ndim(); ++n)
          text += " x " + str (H.dim (n));
        root->appendChild (new TreeItem ("Dimensions", text, root));

        text = str (H.vox (0));
        for (size_t n = 1; n < H.ndim(); ++n)
          text += " x " + str (H.vox (n));
        root->appendChild (new TreeItem ("Voxel size", text, root));

        root->appendChild (new TreeItem ("Data type", H.datatype().description(), root));

        text = str (H.stride (0));
        for (size_t n = 1; n < H.ndim(); ++n)
          text += ", " + str (H.stride (n));
        root->appendChild (new TreeItem ("Strides", text, root));

        root->appendChild (new TreeItem ("Data scaling",
                                         "offset: " + str (H.intensity_offset()) + ", multiplier = " + str (H.intensity_scale()), root));

        if (H.transform().rows() != 4 || H.transform().columns() != 4) {
          root->appendChild (new TreeItem ("Transform", "(invalid)", root));
        }
        else {
          TreeItem* transform = new TreeItem ("Transform", std::string(), root);
          root->appendChild (transform);
          for (size_t n = 0; n < 4; ++n)
            transform->appendChild (new TreeItem (std::string(),
                                                  str (H.transform() (n,0)) + ", " +
                                                  str (H.transform() (n,1)) + ", " +
                                                  str (H.transform() (n,2)) + ", " +
                                                  str (H.transform() (n,3)),
                                                  transform));
        }

        if (H.DW_scheme().is_set()) {
          if (H.DW_scheme().rows() == 0 || H.DW_scheme().columns() != 4) {
            root->appendChild (new TreeItem ("Diffusion scheme", "(invalid)", root));
          }
          else {
            TreeItem* scheme = new TreeItem ("Diffusion scheme", std::string(), root);
            root->appendChild (scheme);
            for (size_t n = 0; n < H.DW_scheme().rows(); ++n)
              scheme->appendChild (new TreeItem (std::string(),
                                                 str (H.DW_scheme() (n,0)) + ", " +
                                                 str (H.DW_scheme() (n,1)) + ", " +
                                                 str (H.DW_scheme() (n,2)) + ", " +
                                                 str (H.DW_scheme() (n,3)),
                                                 scheme));
          }
        }


        view = new QTreeView;
        view->setModel (model);
        view->resizeColumnToContents (0);
        view->resizeColumnToContents (1);
        view->setMinimumSize (500, 200);
        view->setContextMenuPolicy (Qt::CustomContextMenu);
        connect (view, SIGNAL (customContextMenuRequested (const QPoint&)),
                 this, SLOT (context_menu (const QPoint&)));

        QDialogButtonBox* buttonBox = new QDialogButtonBox (QDialogButtonBox::Ok);
        connect (buttonBox, SIGNAL (accepted()), this, SLOT (accept()));

        QVBoxLayout* layout = new QVBoxLayout (this);
        layout->addWidget (view);
        layout->addWidget (buttonBox);
        setLayout (layout);

        setWindowTitle (tr ("Image Properties"));
        setSizeGripEnabled (true);
        adjustSize();
      }

      void ImageProperties::context_menu (const QPoint& point)
      {
        QModelIndex k = view->indexAt (point);
        if (!k.isValid()) return;
        k = k.sibling (k.row(), 0);

        while (k.parent().isValid()) k = k.parent();
        std::string text = k.data().toString().toAscii().constData();

        if (text == "Transform") save_target = &H.transform();
        else if (text == "Diffusion scheme") save_target = &H.DW_scheme();
        else {
          save_target = NULL;
          return;
        }

        QAction* save_action = new QAction (tr ("&Save as..."), this);
        connect (save_action, SIGNAL (triggered()), this, SLOT (write_to_file()));

        QMenu menu (this);
        menu.addAction (save_action);
        menu.exec (view->viewport()->mapToGlobal (point));
      }




      void ImageProperties::write_to_file ()
      {
        assert (save_target);
        std::string name = File::get_save_name (this, "Save as...");
        if (name.size())
          save_target->save (name);
      }

    }
  }
}


