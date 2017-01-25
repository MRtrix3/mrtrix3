/* Copyright (c) 2008-2017 the MRtrix3 contributors
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
