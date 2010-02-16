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

#include <QModelIndex>
#include <QVariant>
#include <QList>

#include "dialog/opengl.h"
#include "opengl/gl.h"

namespace MR {
  namespace Dialog {

    namespace GL {

      class TreeItem
      {
        public:
          TreeItem (const std::string& key, const std::string& value, TreeItem *parent = 0) { 
            parentItem = parent;
            itemData << key.c_str() << value.c_str();
          }
          ~TreeItem() { qDeleteAll(childItems); }
          void appendChild (TreeItem *child)  { childItems.append (child); }
          TreeItem *child (int row)  { return (childItems.value(row)); }
          int childCount () const  { return (childItems.count()); } 
          int columnCount () const  { return (itemData.count()); }
          QVariant data (int column) const  { return (itemData.value (column)); } 
          int row () const  { if (parentItem) return (parentItem->childItems.indexOf(const_cast<TreeItem*>(this))); return (0); }
          TreeItem *parent ()  { return (parentItem); }

        private:
          QList<TreeItem*> childItems;
          QList<QVariant> itemData;
          TreeItem *parentItem;
      };


      TreeModel::TreeModel (QObject *parent) : QAbstractItemModel (parent) { 
        QList<QVariant> rootData;
        rootItem = new TreeItem ("Parameter", "Value");
      }
      TreeModel::~TreeModel () { delete rootItem; }

      QVariant TreeModel::data (const QModelIndex &index, int role) const {
        if (!index.isValid()) return (QVariant());
        if (role != Qt::DisplayRole) return (QVariant());
        return (static_cast<TreeItem*>(index.internalPointer())->data(index.column()));
      }

      Qt::ItemFlags TreeModel::flags (const QModelIndex &index) const {
        if (!index.isValid()) return (0);
        return (Qt::ItemIsEnabled | Qt::ItemIsSelectable);
      }

      QVariant TreeModel::headerData (int section, Qt::Orientation orientation, int role) const {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole) return (rootItem->data(section));
        return (QVariant());
      }

      QModelIndex TreeModel::index (int row, int column, const QModelIndex &parent) const {
        if (!hasIndex(row, column, parent)) return (QModelIndex()); 
        TreeItem *parentItem;
        if (!parent.isValid()) parentItem = rootItem;
        else parentItem = static_cast<TreeItem*>(parent.internalPointer());
        TreeItem *childItem = parentItem->child(row);
        if (childItem) return (createIndex(row, column, childItem));
        else return (QModelIndex());
      }

      QModelIndex TreeModel::parent (const QModelIndex &index) const {
        if (!index.isValid()) return (QModelIndex());
        TreeItem *childItem = static_cast<TreeItem*>(index.internalPointer());
        TreeItem *parentItem = childItem->parent();
        if (parentItem == rootItem) return (QModelIndex());
        return (createIndex(parentItem->row(), 0, parentItem));
      }

      int TreeModel::rowCount (const QModelIndex &parent) const {
        if (parent.column() > 0) return (0);
        TreeItem *parentItem;
        if (!parent.isValid()) parentItem = rootItem;
        else parentItem = static_cast<TreeItem*>(parent.internalPointer());
        return (parentItem->childCount());
      }

      int TreeModel::columnCount (const QModelIndex &parent) const {
        if (parent.isValid()) return static_cast<TreeItem*>(parent.internalPointer())->columnCount();
        else return rootItem->columnCount();
      }

    }

    OpenGL::OpenGL (QWidget* parent) : QDialog (parent) 
    {
      GL::TreeModel* model = new GL::TreeModel (this);

      GL::TreeItem* root = model->rootItem;

      std::string text;
      if (GLEE_VERSION_3_0) text = "3.0";
      else if (GLEE_VERSION_2_1) text = "2.1";
      else if (GLEE_VERSION_2_0) text = "2.0";
      else if (GLEE_VERSION_1_5) text = "1.5";
      else if (GLEE_VERSION_1_4) text = "1.4";
      else if (GLEE_VERSION_1_3) text = "1.3";
      else if (GLEE_VERSION_1_2) text = "1.2";
      else text = "1.1";

      root->appendChild (new GL::TreeItem ("API version", text, root));
      root->appendChild (new GL::TreeItem ("Renderer", (const char*) glGetString (GL_RENDERER), root));
      root->appendChild (new GL::TreeItem ("Vendor", (const char*) glGetString (GL_VENDOR), root));
      root->appendChild (new GL::TreeItem ("Version", (const char*) glGetString (GL_VERSION), root));

      GL::TreeItem* extensions = new GL::TreeItem ("Extensions", std::string(), root);
      root->appendChild (extensions);

      std::vector<std::string> ext = split ((const char*) glGetString (GL_EXTENSIONS));
      for (size_t n = 0; n < ext.size(); ++n)
        extensions->appendChild (new GL::TreeItem (std::string(), ext[n], extensions));

      GL::TreeItem* bit_depths = new GL::TreeItem ("Bit depths", std::string(), root);
      root->appendChild (bit_depths);

      GLint i;
      glGetIntegerv (GL_RED_BITS, &i);
      bit_depths->appendChild (new GL::TreeItem ("red", str(i), bit_depths));
      glGetIntegerv (GL_GREEN_BITS, &i);
      bit_depths->appendChild (new GL::TreeItem ("green", str(i), bit_depths));
      glGetIntegerv (GL_BLUE_BITS, &i);
      bit_depths->appendChild (new GL::TreeItem ("blue", str(i), bit_depths));
      glGetIntegerv (GL_ALPHA_BITS, &i);
      bit_depths->appendChild (new GL::TreeItem ("alpha", str(i), bit_depths));
      glGetIntegerv (GL_DEPTH_BITS, &i);
      bit_depths->appendChild (new GL::TreeItem ("depth", str(i), bit_depths));
      glGetIntegerv (GL_STENCIL_BITS, &i);
      bit_depths->appendChild (new GL::TreeItem ("stencil", str(i), bit_depths));

      GL::TreeItem* buffers = new GL::TreeItem ("Buffers", std::string(), root);
      root->appendChild (buffers);
      glGetIntegerv (GL_DOUBLEBUFFER, &i);
      buffers->appendChild (new GL::TreeItem ("Double buffering", i ? "on" : "off", buffers));
      glGetIntegerv (GL_STEREO, &i);
      buffers->appendChild (new GL::TreeItem ("Stereo buffering", i ? "on" : "off", buffers));
      glGetIntegerv (GL_AUX_BUFFERS, &i);
      buffers->appendChild (new GL::TreeItem ("Auxilliary buffers", str(i), buffers));


      glGetIntegerv (GL_MAX_TEXTURE_SIZE, &i);
      root->appendChild (new GL::TreeItem ("Maximum texture size", str(i), root));
      glGetIntegerv (GL_MAX_LIGHTS, &i);
      root->appendChild (new GL::TreeItem ("Maximum number of lights", str(i), root));
      glGetIntegerv (GL_MAX_CLIP_PLANES, &i);
      root->appendChild (new GL::TreeItem ("Maximum number of clip planes", str(i), root));


      QTreeView* view = new QTreeView;
      view->setModel (model);
      view->resizeColumnToContents (0);
      view->resizeColumnToContents (1);
      view->setMinimumSize (500, 200);

      QDialogButtonBox* buttonBox = new QDialogButtonBox (QDialogButtonBox::Ok);
      connect (buttonBox, SIGNAL(accepted()), this, SLOT(accept()));

      QVBoxLayout* layout = new QVBoxLayout (this);
      layout->addWidget (view);
      layout->addWidget (buttonBox);
      setLayout (layout);

      setWindowTitle (tr("OpenGL information"));
      setSizeGripEnabled (true);
      adjustSize();
    }


  }
}


