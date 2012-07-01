#ifndef __gui_mrview_transform_h__
#define __gui_mrview_transform_h__

#include "gui/opengl/gl.h"
#include "point.h"
#include "math/matrix.h"
#include "math/LU.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {

      class Transform
      {
        public:
          Transform () : M (4,4), invM (4,4) {
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
            Math::Matrix<float> V  (4,4);

            V.zero();
            V(0,0) = 0.5*viewport[2];
            V(0,3) = viewport[0]+0.5*viewport[2];
            V(1,1) = 0.5*viewport[3];
            V(1,3) = viewport[1]+0.5*viewport[3];
            V(2,2) = V(2,3) = 0.5;
            V(3,3) = 1.0;

            Math::Matrix<float> T (4,4);
            Math::mult (T, 1.0f, CblasNoTrans, V, CblasTrans, P);
            Math::mult (M, 1.0f, CblasNoTrans, T, CblasTrans, MV);

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
            return M(2,0)*x[0] + M(2,1)*x[1] + M(2,2)*x[2] + M(2,3);
          }

          Point<> model_to_screen (const Point<>& x) const {
            return Point<> (
                M(0,0)*x[0] + M(0,1)*x[1] + M(0,2)*x[2] + M(0,3),
                M(1,0)*x[0] + M(1,1)*x[1] + M(1,2)*x[2] + M(1,3),
                M(2,0)*x[0] + M(2,1)*x[1] + M(2,2)*x[2] + M(2,3));
          }

          Point<> model_to_screen_direction (const Point<>& x) const {
            return Point<> (
                M(0,0)*x[0] + M(0,1)*x[1] + M(0,2)*x[2],
                M(1,0)*x[0] + M(1,1)*x[1] + M(1,2)*x[2],
                M(2,0)*x[0] + M(2,1)*x[1] + M(2,2)*x[2]);
          }

          Point<> screen_to_model (float x, float y, float depth) const {
            return Point<> (
                invM(0,0)*x + invM(0,1)*y + invM(0,2)*depth + invM(0,3),
                invM(1,0)*x + invM(1,1)*y + invM(1,2)*depth + invM(1,3),
                invM(2,0)*x + invM(2,1)*y + invM(2,2)*depth + invM(2,3));
          }

          Point<> screen_to_model (const Point<>& x) const {
            return screen_to_model (x[0], x[1], x[2]);
          }

          Point<> screen_to_model (const Point<>& x, const Point<>& depth) const {
            return screen_to_model (x[0], x[1], depth_of (depth));
          }

          Point<> screen_to_model (const QPoint& x, const Point<>& depth) const {
            return screen_to_model (x.x(), x.y(), depth_of (depth));
          }

          Point<> screen_normal () const {
            return Point<> (invM(0,2), invM(1,2), invM(2,2)).normalise();
          }

          Point<> screen_to_model_direction (float x, float y) const {
            return Point<> (invM(0,0)*x + invM(0,1)*y, invM(1,0)*x + invM(1,1)*y, invM(2,0)*x + invM(2,1)*y);
          }

          Point<> screen_to_model_direction (const Point<>& x) const {
            return screen_to_model_direction (x[0], x[1]);
          }

          Point<> screen_to_model_direction (const QPoint& x) const {
            return screen_to_model_direction (x.x(), x.y());
          }


          void draw_focus (const Point<>& focus) const;

        protected:
          Math::Matrix<float> M, invM;
          GLint viewport[4];
      };

    }
  }
}

#endif
