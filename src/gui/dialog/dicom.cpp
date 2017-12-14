/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#include "file/dicom/tree.h"
#include "gui/dialog/list.h"
#include "gui/dialog/dicom.h"
#include "gui/gui.h"

namespace MR
{
  namespace GUI
  {
    namespace Dialog
    {

      namespace
      {

        class Item
        { NOMEMALIGN
          public:
            Item () : parentItem (NULL) { }
            Item (Item* parent, const std::shared_ptr<Patient>& p) :
              parentItem (parent) {
              itemData = (p->name + " " + format_ID (p->ID) + " " + format_date (p->DOB)).c_str();
            }
            Item (Item* parent, const std::shared_ptr<Study>& p) :
              parentItem (parent) {
              itemData = ( (p->name.size() ? p->name : std::string ("unnamed")) + " " +
                           format_ID (p->ID) + " " + format_date (p->date) + " " + format_time (p->time)).c_str();
            }
            Item (Item* parent, const std::shared_ptr<Series>& p) :
              parentItem (parent), dicom_series (p) {
              itemData = (str (p->size()) + " " + (p->modality.size() ? p->modality : std::string()) + " images " +
                          format_time (p->time) + " " + (p->name.size() ? p->name : std::string ("unnamed")) + " (" +
                          ( (*p) [0]->sequence_name.size() ? (*p) [0]->sequence_name : std::string ("?")) +
                          ") [" + str (p->number) + "] " + p->image_type).c_str();
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
            const std::shared_ptr<Series>& series () const {
              return (dicom_series);
            }

          private:
            QList<Item*> childItems;
            QVariant itemData;
            Item* parentItem;
            std::shared_ptr<Series> dicom_series;
        };


        class Model : public QAbstractItemModel
        { NOMEMALIGN
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

            QVariant headerData (int, Qt::Orientation orientation, int role = Qt::DisplayRole) const {
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
              (void) parent; // to suppress warnings about unused parameters
              return (1);
            }
            Item* rootItem;
        };




        class DicomSelector : public QDialog
        { NOMEMALIGN
          public:
            DicomSelector (const Tree& tree) : QDialog (GUI::App::main_window) {
              Model* model = new Model (this);

              Item* root = model->rootItem;
              for (size_t i = 0; i < tree.size(); ++i) {
                Item* patient_root = new Item (root, tree[i]);
                root->appendChild (patient_root);
                const Patient patient (*tree[i]);
                for (size_t j = 0; j < patient.size(); ++j) {
                  Item* study_root = new Item (patient_root, patient[j]);
                  patient_root->appendChild (study_root);
                  const Study study (*patient[j]);
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



      vector<std::shared_ptr<Series>> select_dicom (const Tree& tree)
      {
        vector<std::shared_ptr<Series> > ret;
        if (tree.size() == 1) {
          if (tree[0]->size() == 1) {
            if ((*tree[0])[0]->size() == 1) {
              ret.push_back ((*(*tree[0])[0])[0]);
              return ret;
            }
          }
        }
            
        DicomSelector selector (tree);
        if (selector.exec()) {
          QModelIndexList indexes = selector.view->selectionModel()->selectedIndexes();
          if (indexes.size()) {
            QModelIndex index;
            Q_FOREACH (index, indexes) {
              Item* item = static_cast<Item*> (index.internalPointer());
              if (item->series()) 
                ret.push_back (item->series());
            }
          }
        }
        return ret;
      }




    }
  }
}


