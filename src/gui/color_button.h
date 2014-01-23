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

#ifndef __gui_color_button_h__
#define __gui_color_button_h__

#include "gui/opengl/gl.h"

class QColorButton : public QPushButton
{
  Q_OBJECT
  Q_PROPERTY (QColor color READ color WRITE setColor)

  public:
    QColorButton (QWidget *parent = NULL, const char *name = NULL);
    QColorButton (const QColor &c, QWidget *parent = NULL, const char *name = NULL);
    virtual ~QColorButton () {}

    QColor color () const { return (col); }
    void setColor (const QColor &c);
    QSize sizeHint () const;

  signals:
    void changed (const QColor &newColor);

  protected slots:
    void chooseColor();

  protected:
    virtual void paintEvent (QPaintEvent *p);

  private:
    QColor col;
    QPoint mPos;
};

#endif
