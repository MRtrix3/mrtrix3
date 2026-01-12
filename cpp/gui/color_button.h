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

#include "opengl/glutils.h"

class QColorButton : public QPushButton {
  Q_OBJECT
  Q_PROPERTY(QColor color READ color WRITE setColor)

public:
  QColorButton(QWidget *parent = nullptr, const char *name = nullptr);                  // check_syntax off
  QColorButton(const QColor &c, QWidget *parent = nullptr, const char *name = nullptr); // check_syntax off
  virtual ~QColorButton() {}

  QColor color() const { return (col); }
  void setColor(const QColor &c);
  QSize sizeHint() const;

signals:
  void changed(const QColor &newColor);

protected slots:
  void chooseColor();

protected:
  virtual void paintEvent(QPaintEvent *p);

private:
  QColor col;
  QPoint mPos;
};
