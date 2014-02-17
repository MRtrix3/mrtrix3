/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


#ifndef __gui_dialog_list_h__
#define __gui_dialog_list_h__

#include "gui/opengl/gl.h"

namespace MR
{
  namespace GUI
  {
    namespace Dialog
    {

      class TreeItem
      {
        public:
          TreeItem (const std::string& key, const std::string& value, TreeItem* parent = 0) {
            parentItem = parent;
            itemData << key.c_str() << value.c_str();
          }
          ~TreeItem() {
            qDeleteAll (childItems);
          }
          void appendChild (TreeItem* child)  {
            childItems.append (child);
          }
          TreeItem* child (int row)  {
            return childItems.value (row);
          }
          int childCount () const  {
            return childItems.count();
          }
          int columnCount () const  {
            return itemData.count();
          }
          QVariant data (int column) const  {
            return itemData.value (column);
          }
          int row () const  {
            if (parentItem) 
              return parentItem->childItems.indexOf (const_cast<TreeItem*> (this));
            return 0;
          }
          TreeItem* parent ()  {
            return parentItem;
          }

        private:
          QList<TreeItem*> childItems;
          QList<QVariant> itemData;
          TreeItem* parentItem;
      };


      class TreeModel : public QAbstractItemModel
      {
        public:
          TreeModel (QObject* parent) : QAbstractItemModel (parent) {
            rootItem = new TreeItem ("Parameter", "Value");
          }
          ~TreeModel () {
            delete rootItem;
          }
          QVariant data (const QModelIndex& index, int role) const;
          Qt::ItemFlags flags (const QModelIndex& index) const;
          QVariant headerData (int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
          QModelIndex index (int row, int column, const QModelIndex& parent = QModelIndex()) const;
          QModelIndex parent (const QModelIndex& index) const;
          int rowCount (const QModelIndex& parent = QModelIndex()) const;
          int columnCount (const QModelIndex& parent = QModelIndex()) const;
          TreeItem* rootItem;
      };

    }
  }
}

#endif



