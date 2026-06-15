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

#include "eigen_plugins/eigen_plugins.h"
#include <Eigen/Eigenvalues>
#include <QOpenGLWidget>

#include "dwi/directions/set.h"
#include "gui.h"
#include "math/SH.h"
#include "opengl/glutils.h"
#include "opengl/shader.h"
#include "shapes/halfsphere.h"

namespace MR::GUI {

class Projection;

namespace GL {
class Lighting;
class mat4;
} // namespace GL

namespace DWI {

class Renderer {

  using matrix_t = Eigen::MatrixXf;
  using vector_t = Eigen::VectorXf;
  using tensor_t = Eigen::Matrix3f;

public:
  enum class mode_t { SH, TENSOR, DIXEL };

  Renderer(QOpenGLWidget *);

  bool ready() const { return shader != 0U; }

  void initGL() {
    sh.initGL();
    tensor.initGL();
    dixel.initGL();
  }

  void set_mode(const mode_t i) { mode = i; }

  void start(const Projection &projection,
             const GL::Lighting &lighting,
             float scale,
             bool use_lighting,
             bool color_by_direction,
             bool hide_neg_lobes,
             bool orthographic = false,
             const GL::mat4 *colour_relative_to_projection = nullptr);

  void draw(const Eigen::Vector3f &origin, int buffer_ID = 0) const;

  void stop() const { shader.stop(); }

  QColor get_colour() const {
    return QColor(object_color[0] * 255.0F, object_color[1] * 255.0F, object_color[2] * 255.0F);
  }

  void set_colour(const QColor &c) {
    object_color[0] = c.red() / 255.0F;
    object_color[1] = c.green() / 255.0F;
    object_color[2] = c.blue() / 255.0F;
  }

protected:
  mode_t mode{mode_t::SH};
  Eigen::Array3f object_color;
  mutable GLuint reverse_ID{0}, origin_ID{0};

  class Shader : public GL::Shader::Program {
  public:
    Shader() = default;
    void start(mode_t mode,
               bool use_lighting,
               bool colour_by_direction,
               bool hide_neg_values,
               bool orthographic,
               bool colour_relative_to_projection);

  protected:
    mode_t mode_{mode_t::SH};
    bool use_lighting_{true}, colour_by_direction_{true}, hide_neg_values_{true}, orthographic_{false},
        colour_relative_to_projection_;
    [[nodiscard]] std::string vertex_shader_source() const;
    [[nodiscard]] std::string geometry_shader_source() const;
    [[nodiscard]] std::string fragment_shader_source() const;
  } shader;

  void half_draw() const {
    const GLuint num_indices =
        (mode == mode_t::SH ? sh.num_indices() : (mode == mode_t::TENSOR ? tensor.num_indices() : dixel.num_indices()));
    gl::DrawElements(gl::TRIANGLES, num_indices, gl::UNSIGNED_INT, nullptr);
  }

private:
  class ModeBase {
  public:
    ModeBase(Renderer &parent) : parent(parent) {}
    virtual ~ModeBase() = default;

    virtual void initGL() = 0;
    virtual void bind() = 0;
    virtual void set_data(const vector_t &, int buffer_ID = 0) const = 0;
    [[nodiscard]] virtual GLuint num_indices() const = 0;

  protected:
    Renderer &parent;
  };

public:
  class SH : public ModeBase {
  public:
    SH(Renderer &parent) : ModeBase(parent) {}
    ~SH();

    void initGL() override;
    void bind() override;
    void set_data(const vector_t &r_del_daz, int buffer_ID = 0) const override;
    [[nodiscard]] GLuint num_indices() const override { return half_sphere.num_indices; }

    void update_mesh(const size_t, const int);

    void compute_r_del_daz(matrix_t &r_del_daz, const matrix_t &SH) const {
      if ((SH.rows() == 0) || (SH.cols() == 0))
        return;
      assert(transform.rows());
      r_del_daz.noalias() = SH * transform.transpose();
    }

    void compute_r_del_daz(vector_t &r_del_daz, const vector_t &SH) const {
      if (SH.size() == 0)
        return;
      assert(transform.rows());
      r_del_daz.noalias() = transform * SH;
    }

    [[nodiscard]] int get_LOD() const { return LOD; }

  private:
    int LOD{0};
    matrix_t transform;
    Shapes::HalfSphere half_sphere;
    GL::VertexBuffer surface_buffer;
    GL::VertexArrayObject VAO;

    void update_transform(const std::vector<Shapes::HalfSphere::Vertex> &, int);

  } sh;

  class Tensor : public ModeBase {
  public:
    Tensor(Renderer &parent) : ModeBase(parent) {}
    ~Tensor();

    void initGL() override;
    void bind() override;
    void set_data(const vector_t &data, int buffer_ID = 0) const override;
    GLuint num_indices() const override { return half_sphere.num_indices; }

    void update_mesh(const size_t);

    int get_LOD() const { return LOD; }

  private:
    int LOD{0};
    Shapes::HalfSphere half_sphere;
    GL::VertexArrayObject VAO;

    mutable Eigen::SelfAdjointEigenSolver<tensor_t> eig;

  } tensor;

  class Dixel : public ModeBase {

    using dir_t = MR::DWI::Directions::index_type;

  public:
    Dixel(Renderer &parent) : ModeBase(parent) {}
    ~Dixel();

    void initGL() override;
    void bind() override;
    void set_data(const vector_t &, int buffer_ID = 0) const override;
    [[nodiscard]] GLuint num_indices() const override { return index_count; }

    void update_mesh(const MR::DWI::Directions::Set &);

  private:
    GL::VertexBuffer vertex_buffer, value_buffer;
    GL::IndexBuffer index_buffer;
    GL::VertexArrayObject VAO;
    GLuint vertex_count{0}, index_count{0};

    void update_dixels(const MR::DWI::Directions::Set &);

  } dixel;

private:
  QOpenGLWidget *context_;
};

} // namespace DWI
} // namespace MR::GUI
