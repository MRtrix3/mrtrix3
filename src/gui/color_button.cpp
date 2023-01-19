/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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

#include "gui/color_button.h"

QColorButton::QColorButton (QWidget *parent, const char *name) : QPushButton (name, parent)
{
  connect (this, SIGNAL(clicked()), this, SLOT(chooseColor()));
}

QColorButton::QColorButton (const QColor &c, QWidget *parent, const char *name) : QPushButton (name, parent) 
{
  setColor (c);
  connect (this, SIGNAL(clicked()), this, SLOT(chooseColor()));
}


void QColorButton::setColor (const QColor &c)
{
  if ( col != c ) {
    col = c;
    update();
    emit changed (col);
  }
}

void QColorButton::paintEvent (QPaintEvent *p)
{
  QPushButton::paintEvent (p);
  QStyleOptionButton option;
  option.initFrom (this);

  int x, y, w, h;
  QRect r = style()->subElementRect (QStyle::SE_PushButtonContents, &option, this);
  r.getRect (&x, &y, &w, &h);

  int margin = style()->pixelMetric (QStyle::PM_ButtonMargin, &option, this);
  x += margin;
  y += margin;
  w -= 2*margin;
  h -= 2*margin;

  if (isChecked() || isDown()) {
    x += style()->pixelMetric (QStyle::PM_ButtonShiftHorizontal, &option, this );
    y += style()->pixelMetric (QStyle::PM_ButtonShiftVertical, &option, this );
  }

  QPainter painter (this);
  QColor fillCol = isEnabled() ? col : palette().brush(QPalette::Window).color();
  qDrawShadePanel (&painter, x, y, w, h, palette(), true, 1, NULL);
  if (fillCol.isValid())
    painter.fillRect (x+1, y+1, w-2, h-2, fillCol);

  if (hasFocus()) {
    style()->subElementRect (QStyle::SE_PushButtonFocusRect, &option, this);
    style()->drawPrimitive (QStyle::PE_FrameFocusRect, &option, &painter, this);
  }
}

QSize QColorButton::sizeHint() const
{
  QStyleOptionButton option;
  option.initFrom (this);
  return (style()->sizeFromContents(QStyle::CT_PushButton, &option, option.fontMetrics.size (0, "MM"), this).expandedTo(QApplication::globalStrut()));
}

void QColorButton::chooseColor()
{
  QColor c = QColorDialog::getColor (color(), this);
  if (c.isValid()) setColor (c);
}

