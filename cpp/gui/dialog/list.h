/* Copyright (c) 2008-2026 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#pragma once

#include "gui.h"
#include "opengl/glutils.h"

namespace MR::GUI::Dialog {

class TreeItem {
public:
  TreeItem(std::string_view key, std::string_view value, TreeItem *parent = 0) {
    parentItem = parent;
    itemData << qstr(key) << qstr(value);
  }
  ~TreeItem() { qDeleteAll(childItems); }
  void appendChild(TreeItem *child) { childItems.append(child); }
  TreeItem *child(int row) { return childItems.value(row); }
  [[nodiscard]] int childCount() const { return childItems.count(); }
  [[nodiscard]] int columnCount() const { return itemData.count(); }
  [[nodiscard]] QVariant data(int column) const { return itemData.value(column); }
  [[nodiscard]] int row() const {
    if (parentItem != nullptr)
      return parentItem->childItems.indexOf(const_cast<TreeItem *>(this));
    return 0;
  }
  TreeItem *parent() { return parentItem; }

private:
  QList<TreeItem *> childItems;
  QList<QVariant> itemData;
  TreeItem *parentItem;
};

class TreeModel : public QAbstractItemModel {
public:
  TreeModel(QObject *parent) : QAbstractItemModel(parent) { rootItem = new TreeItem("Parameter", "Value"); }
  ~TreeModel() { delete rootItem; }
  [[nodiscard]] QVariant data(const QModelIndex &index, int role) const;
  [[nodiscard]] Qt::ItemFlags flags(const QModelIndex &index) const;
  [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
  [[nodiscard]] QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
  [[nodiscard]] QModelIndex parent(const QModelIndex &index) const;
  [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const;
  [[nodiscard]] int columnCount(const QModelIndex &parent = QModelIndex()) const;
  TreeItem *rootItem;
};

} // namespace MR::GUI::Dialog
