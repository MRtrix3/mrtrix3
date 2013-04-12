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

#include <QDialog>
#include <QModelIndex>
#include <QVariant>
#include <QList>
#include <QTreeView>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QAbstractListModel>

#include "file/dicom/tree.h"
#include "gui/dialog/list.h"
#include "gui/dialog/dicom.h"

namespace MR
{
  namespace GUI
  {
    namespace Dialog
    {

      namespace
      {

        class Item
        {
          public:
            Item () : parentItem (NULL) { }
            Item (Item* parent, const RefPtr<Patient>& p) :
              parentItem (parent) {
              itemData = (p->name + " " + format_ID (p->ID) + " " + format_date (p->DOB)).c_str();
            }
            Item (Item* parent, const RefPtr<Study>& p) :
              parentItem (parent) {
              itemData = ( (p->name.size() ? p->name : std::string ("unnamed")) + " " +
                           format_ID (p->ID) + " " + format_date (p->date) + " " + format_time (p->time)).c_str();
            }
            Item (Item* parent, const RefPtr<Series>& p) :
              parentItem (parent), dicom_series (p) {
              itemData = (str (p->size()) + " " + (p->modality.size() ? p->modality : std::string()) + " images " +
                          format_time (p->time) + " " + (p->name.size() ? p->name : std::string ("unnamed")) + " (" +
                          ( (*p) [0]->sequence_name.size() ? (*p) [0]->sequence_name : std::string ("?")) +
                          ") [" + str (p->number) + "]").c_str();
            }
            ~Item() {
              qDeleteAll (childItems);
            }
            void appendChild (Item* child)  {
              childItems.append (child);
            }
            Item* child (int row)  {
              return (childItems.value (row));
            }
            int childCount () const  {
              return (childItems.count());
            }
            QVariant data () const  {
              return (itemData);
            }
            int row () const  {
              if (parentItem) return (parentItem->childItems.indexOf (const_cast<Item*> (this)));
              return (0);
            }
            Item* parent ()  {
              return (parentItem);
            }
            const RefPtr<Series>& series () const {
              return (dicom_series);
            }

          private:
            QList<Item*> childItems;
            QVariant itemData;
            Item* parentItem;
            RefPtr<Series> dicom_series;
        };


        class Model : public QAbstractItemModel
        {
          public:
            Model (QObject* parent) : QAbstractItemModel (parent) {
              QList<QVariant> rootData;
              rootItem = new Item;
            }

            ~Model () {
              delete rootItem;
            }

            QVariant data (const QModelIndex& index, int role) const {
              if (!index.isValid()) return (QVariant());
              if (role != Qt::DisplayRole) return (QVariant());
              if (index.column() != 0) return (QVariant());
              return (static_cast<Item*> (index.internalPointer())->data());
            }

            Qt::ItemFlags flags (const QModelIndex& index) const {
              if (!index.isValid()) return (0);
              return (Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            }

            QVariant headerData (int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const {
              if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
                return ("Name");
              return (QVariant());
            }

            QModelIndex index (int row, int column, const QModelIndex& parent = QModelIndex()) const {
              if (!hasIndex (row, column, parent)) return (QModelIndex());
              Item* parentItem;
              if (!parent.isValid()) parentItem = rootItem;
              else parentItem = static_cast<Item*> (parent.internalPointer());
              Item* childItem = parentItem->child (row);
              if (childItem) return (createIndex (row, column, childItem));
              else return (QModelIndex());
            }

            QModelIndex parent (const QModelIndex& index) const {
              if (!index.isValid()) return (QModelIndex());
              Item* childItem = static_cast<Item*> (index.internalPointer());
              Item* parentItem = childItem->parent();
              if (parentItem == rootItem) return (QModelIndex());
              return (createIndex (parentItem->row(), 0, parentItem));
            }

            int rowCount (const QModelIndex& parent = QModelIndex()) const {
              if (parent.column() > 0) return (0);
              Item* parentItem;
              if (!parent.isValid()) parentItem = rootItem;
              else parentItem = static_cast<Item*> (parent.internalPointer());
              return (parentItem->childCount());
            }

            int columnCount (const QModelIndex& parent = QModelIndex()) const {
              return (1);
            }
            Item* rootItem;
        };




        class DicomSelector : public QDialog
        {
          public:
            DicomSelector (const Tree& tree) : QDialog (NULL) {
              Model* model = new Model (this);

              Item* root = model->rootItem;
              for (size_t i = 0; i < tree.size(); ++i) {
                Item* patient_root = new Item (root, tree[i]);
                root->appendChild (patient_root);
                const Patient patient (*tree[i]);
                for (size_t j = 0; j < patient.size(); ++j) {
                  Item* study_root = new Item (patient_root, patient[j]);
                  patient_root->appendChild (study_root);
                  const Study study (*patient[i]);
                  for (size_t k = 0; k < study.size(); ++k)
                    study_root->appendChild (new Item (study_root, study[k]));
                }
              }

              view = new QTreeView;
              view->setModel (model);
              view->setMinimumSize (500, 200);
              view->expandAll();

              QDialogButtonBox* buttonBox = new QDialogButtonBox (QDialogButtonBox::Ok);
              connect (buttonBox, SIGNAL (accepted()), this, SLOT (accept()));
              connect (view, SIGNAL (activated (const QModelIndex&)), this, SLOT (accept()));

              QVBoxLayout* layout = new QVBoxLayout (this);
              layout->addWidget (view);
              layout->addWidget (buttonBox);
              setLayout (layout);

              setWindowTitle (tr ("Select DICOM series"));
              setSizeGripEnabled (true);
              adjustSize();
            }
            QTreeView* view;
        };

      }



      std::vector< RefPtr<Series> > select_dicom (const Tree& tree)
      {
        DicomSelector selector (tree);
        std::vector<RefPtr<Series> > ret;
        if (selector.exec()) {
          QModelIndexList indexes = selector.view->selectionModel()->selectedIndexes();
          if (indexes.size()) {
            QModelIndex index;
            foreach (index, indexes) {
              Item* item = static_cast<Item*> (index.internalPointer());
              if (item->series()) ret.push_back (item->series());
            }
          }
        }
        return ret;
      }




    }
  }
}


