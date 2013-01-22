#ifndef __gui_mrview_transform_h__
#define __gui_mrview_transform_h__

#include "gui/opengl/gl.h"
#include "gui/opengl/font.h"
#include "point.h"
#include "math/matrix.h"
#include "math/LU.h"

#include <QGLWidget>

#ifdef USE_OPENGL3
#include <glm/gtc/matrix_transform.hpp>
#endif

namespace MR
{
  namespace GUI
  {

    const int TopEdge = 0x00000001;
    const int BottomEdge = 0x00000002;
    const int LeftEdge = 0x00000004;
    const int RightEdge = 0x00000008;




    class Projection
    {
      public:
        Projection (QGLWidget* parent, const GL::Font& font) : 
          glarea (parent), 
          font (font),
          M (4,4), 
          invM (4,4) {
            M.identity();
            invM.identity();
            viewport[0] = viewport[1] = viewport[2] = viewport[3] = 0;
          }


        void update () {
          float modelview[16];
          float projection[16];

          glGetIntegerv (GL_VIEWPORT, viewport);
          glGetFloatv (GL_MODELVIEW_MATRIX, modelview);
          glGetFloatv (GL_PROJECTION_MATRIX, projection);

          Math::Matrix<float> MV (modelview, 4, 4);
          Math::Matrix<float> P (projection, 4, 4);

          Math::mult (M, 1.0f, CblasTrans, P, CblasTrans, MV);
          Math::LU::inv (invM, M);
        }

        GLint x_position () const {
          return viewport[0];
        }

        GLint y_position () const {
          return viewport[1];
        }

        GLint width () const {
          return viewport[2];
        }

        GLint height () const {
          return viewport[3];
        }

        float depth_of (const Point<>& x) const {
          float d = M(2,0)*x[0] + M(2,1)*x[1] + M(2,2)*x[2] + M(2,3);
          if (M(3,2)) d /= M(3,0)*x[0] + M(3,1)*x[1] + M(3,2)*x[2] + M(3,3);
          return d;
        }

        Point<> model_to_screen (const Point<>& x) const {
          Point<> S (
              M(0,0)*x[0] + M(0,1)*x[1] + M(0,2)*x[2] + M(0,3),
              M(1,0)*x[0] + M(1,1)*x[1] + M(1,2)*x[2] + M(1,3),
              M(2,0)*x[0] + M(2,1)*x[1] + M(2,2)*x[2] + M(2,3));
          if (M(3,2))
            S /= M(3,0)*x[0] + M(3,1)*x[1] + M(3,2)*x[2] + M(3,3);
          S[0] = viewport[0] + 0.5*viewport[2]*(1.0+S[0]); 
          S[1] = viewport[1] + 0.5*viewport[3]*(1.0+S[1]); 
          return S;
        }

        Point<> model_to_screen_direction (const Point<>& dir) const {
          Point<> S (
              M(0,0)*dir[0] + M(0,1)*dir[1] + M(0,2)*dir[2],
              M(1,0)*dir[0] + M(1,1)*dir[1] + M(1,2)*dir[2],
              M(2,0)*dir[0] + M(2,1)*dir[1] + M(2,2)*dir[2]);
          S[0] *= 0.5*viewport[2];
          S[1] *= 0.5*viewport[3];
          return S;
        }

        Point<> screen_to_model (float x, float y, float depth) const {
          x = 2.0*(x-viewport[0])/viewport[2] - 1.0;
          y = 2.0*(y-viewport[1])/viewport[3] - 1.0;
          Point<> S (
              invM(0,0)*x + invM(0,1)*y + invM(0,2)*depth + invM(0,3),
              invM(1,0)*x + invM(1,1)*y + invM(1,2)*depth + invM(1,3),
              invM(2,0)*x + invM(2,1)*y + invM(2,2)*depth + invM(2,3));
          if (M(3,2)) 
            S /= invM(3,0)*x + invM(3,1)*y + invM(3,2)*depth + invM(3,3);
          return S;
        }

        Point<> screen_to_model (const Point<>& x) const {
          return screen_to_model (x[0], x[1], x[2]);
        }

        Point<> screen_to_model (const Point<>& x, float depth) const {
          return screen_to_model (x[0], x[1], depth);
        }

        Point<> screen_to_model (const Point<>& x, const Point<>& depth) const {
          return screen_to_model (x, depth_of (depth));
        }

        Point<> screen_to_model (const QPoint& x, float depth) const {
          return screen_to_model (x.x(), x.y(), depth);
        }

        Point<> screen_to_model (const QPoint& x, const Point<>& depth) const {
          return screen_to_model (x, depth_of (depth));
        }

        Point<> screen_normal () const {
          return Point<> (invM(0,2), invM(1,2), invM(2,2)).normalise();
        }

        Point<> screen_to_model_direction (float x, float y, float depth) const {
          x *= 2.0/viewport[2];
          y *= 2.0/viewport[3];
          Point<> S (invM(0,0)*x + invM(0,1)*y, invM(1,0)*x + invM(1,1)*y, invM(2,0)*x + invM(2,1)*y);
          if (M(3,2)) 
            S /= invM(3,2)*depth + invM(3,3);
          return S;
        }

        Point<> screen_to_model_direction (const Point<>& dx, float x) const {
          return screen_to_model_direction (dx[0], dx[1], x);
        }

        Point<> screen_to_model_direction (const Point<>& dx, const Point<>& x) const {
          return screen_to_model_direction (dx, depth_of (x));
        }

        Point<> screen_to_model_direction (const QPoint& dx, float x) const {
          return screen_to_model_direction (dx.x(), dx.y(), x);
        }

        Point<> screen_to_model_direction (const QPoint& dx, const Point<>& x) const {
          return screen_to_model_direction (dx, depth_of (x));
        }


        void render_crosshairs (const Point<>& focus) const;

        void setup_render_text (float red = 1.0, float green = 1.0, float blue = 0.0) const { 
          font.setupGL (width(), height(), red, green, blue); 
        }
        void done_render_text () const { font.resetGL(); }

        void render_text (int x, int y, const std::string& text) {
          font.render (text, x, y);
        }

        void render_text_inset (int x, int y, const std::string& text, int inset = -1) {
          QString s (text.c_str());
          if (inset < 0) 
            inset = font.metric.height() / 2;
          if (x < inset) 
            x = inset;
          if (x + font.metric.width (s) + inset > width()) 
            x = width() - font.metric.width (s) - inset;
          if (y < inset) 
            y = inset;
          if (y + font.metric.height() + inset > height())
            y = height() - font.metric.height() - inset;
          render_text (x, y, text);
        }

        void render_text (const std::string& text, int position, int line = 0) {
          QString s (text.c_str());
          int x, y;

          if (position & RightEdge) x = width() - font.metric.height() / 2 - font.metric.width (s);
          else if (position & LeftEdge) x = font.metric.height() / 2;
          else x = (width() - font.metric.width (s)) / 2;

          if (position & TopEdge) y = height() - 1.5 * font.metric.height() - line * font.metric.lineSpacing();
          else if (position & BottomEdge) y = font.metric.height() / 2 + line * font.metric.lineSpacing();
          else y = (height() - font.metric.height()) / 2 - line * font.metric.lineSpacing();

          render_text (x, y, text);
        }

        const Math::Matrix<float>& get_MVP () const {
          return M;
        }

      protected:
        QGLWidget* glarea;
        const GL::Font& font;
        Math::Matrix<float> M, invM;
        GLint viewport[4];
    };




  }
}

#endif
