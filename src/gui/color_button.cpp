/* 
 This file is part of the KDE libraries
 Copyright (C) 1997 Martin Jones (mjones@kde.org)
 
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License as published by the Free Software Foundation; either
 version 2 of the License, or (at your option) any later version.
 
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.
 
 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA  02110-1301, USA. 

 Modified by Donald Tournier (d.tournier@brain.org.au) 23/01/2009
 
*/

#include "gui/color_button.h"
#include <QPainter>
#include <QApplication>
#include <QStyle>
#include <QColorDialog>
#include <QStyleOptionButton>

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
    QRect focusRect = style()->subElementRect (QStyle::SE_PushButtonFocusRect, &option, this);
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

