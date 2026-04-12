/* Copyright (c) 2017-2019 Daan Christiaens
 *
 * MRtrix and this add-on module are distributed in the hope
 * that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 */

#pragma once

#include <Eigen/Dense>
#include <algorithm>
#include <atomic>

#include "adapter/base.h"
#include "algo/loop.h"
#include "dwi/shells.h"
#include "math/SH.h"
#include "types.h"

#include "dwi/svr/param.h"

namespace MR::Adapter {
template <class ImageType> class ReadCache : public Adapter::Base<ReadCache<ImageType>, ImageType> {
public:
  using base_type = Adapter::Base<ReadCache<ImageType>, ImageType>;
  using value_type = typename ImageType::value_type;

  using base_type::parent;

  ReadCache(const ImageType &parent) : base_type(parent) {
    Header hdr(parent);
    buffer = Image<value_type>::scratch(hdr, "temporary buffer");
  }

  ReadCache(const ReadCache &other) : ReadCache(other.parent()) {}

  FORCE_INLINE ssize_t get_index(size_t axis) const { return buffer.get_index(axis); }
  FORCE_INLINE void move_index(size_t axis, ssize_t increment) { buffer.move_index(axis, increment); }
  FORCE_INLINE void reset() { buffer.reset(); }

  void flush() {
    // clear buffer
    reset();
    std::fill_n(buffer.address(), voxel_count(buffer), value_type(NAN));
  }

  FORCE_INLINE value_type value() {
    value_type val = *buffer.address();
    if (!std::isfinite(val))
      load(val);
    return val;
  }

  FORCE_INLINE void set_shotidx(size_t idx) {
    flush();
    parent().set_shotidx(idx);
  }

private:
  Image<value_type> buffer;

  FORCE_INLINE void load(value_type &val) {
    assign_pos_of(buffer).to(parent());
    *buffer.address() = val = parent().value();
  }
};

template <class ImageType> class WriteCache : public Adapter::Base<WriteCache<ImageType>, ImageType> {
public:
  using base_type = Adapter::Base<WriteCache<ImageType>, ImageType>;
  using value_type = typename ImageType::value_type;

  using base_type::parent;

  WriteCache(const ImageType &parent) : base_type(parent) {
    Header hdr(parent);
    buffer = Image<value_type>::scratch(hdr, "temporary buffer");
    // initialise lock image
    static_assert(sizeof(std::atomic_flag) == sizeof(uint8_t), "std::atomic_flag expected to be 1 byte");
    lock = Image<uint8_t>::scratch(hdr, "temporary buffer lock");
  }

  WriteCache(const WriteCache &other) : base_type(other.parent()), lock(other.lock) {
    Header hdr(other.parent());
    buffer = Image<value_type>::scratch(hdr, "temporary buffer");
  }

  FORCE_INLINE ssize_t get_index(size_t axis) const { return buffer.get_index(axis); }
  FORCE_INLINE void move_index(size_t axis, ssize_t increment) { buffer.move_index(axis, increment); }
  FORCE_INLINE void reset() { buffer.reset(); }

  void flush() {
    // delayed write back
    for (auto l = Loop()(buffer); l; l++) {
      if (buffer.value()) {
        assign_pos_of(buffer).to(parent(), lock);
        std::atomic_flag *flag = reinterpret_cast<std::atomic_flag *>(lock.address());
        while (flag->test_and_set(std::memory_order_acquire))
          ;
        parent().adjoint_add(buffer.value());
        flag->clear(std::memory_order_release);
      }
    }
    // clear buffer
    reset();
    std::fill_n(buffer.address(), voxel_count(buffer), value_type(0));
  }

  FORCE_INLINE void adjoint_add(value_type val) { *buffer.address() += val; }

  FORCE_INLINE void set_shotidx(size_t idx) {
    flush();
    parent().set_shotidx(idx);
  }

private:
  Image<value_type> buffer;
  Image<uint8_t> lock;
};

template <template <class ImageType> class AdapterType, class ImageType, typename... Args>
inline ReadCache<AdapterType<ImageType>> makecached(const ImageType &parent, Args &&...args) {
  return {{parent, std::forward<Args>(args)...}};
}

template <template <class ImageType> class AdapterType, class ImageType, typename... Args>
inline WriteCache<AdapterType<ImageType>> makecached_add(const ImageType &parent, Args &&...args) {
  return {{parent, std::forward<Args>(args)...}};
}
} // namespace MR::Adapter

namespace MR::DWI::SVR {
class QSpaceBasis {
public:
  typedef Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> RowMatrixXf;

  QSpaceBasis(const Eigen::MatrixXf &grad,
              const int lmax,
              const std::vector<Eigen::MatrixXf> &rf,
              const Eigen::MatrixXf &rigid)
      : lmax(lmax), nv(grad.rows()), ne(rigid.rows() / nv), nc(get_ncoefs(rf)), shellbasis(init_shellbasis(grad, rf)) {
    init_Y(grad, rigid);
  }

  QSpaceBasis(const Eigen::MatrixXf &grad, const int lmax, const std::vector<Eigen::MatrixXf> &rf)
      : lmax(lmax), nv(grad.rows()), ne(1), nc(get_ncoefs(rf)), shellbasis(init_shellbasis(grad, rf)) {
    RowMatrixXf zero(nv, 6);
    init_Y(grad, zero);
  }

  FORCE_INLINE Eigen::Ref<const Eigen::VectorXf> get_projection(size_t idx) const { return Y.row(idx); }
  FORCE_INLINE Eigen::Ref<const Eigen::VectorXf> get_projection(size_t v, size_t z) const { return Y.row(v * ne + z); }

  FORCE_INLINE size_t get_ncoefs() const { return nc; }
  FORCE_INLINE const RowMatrixXf &getY() const { return Y; }
  FORCE_INLINE const Eigen::MatrixXf &getShellBasis(const int shellidx) const { return shellbasis[shellidx]; }

private:
  const int lmax;
  const size_t nv, ne, nc;
  const std::vector<Eigen::MatrixXf> shellbasis;
  RowMatrixXf Y;

  std::vector<Eigen::MatrixXf> init_shellbasis(const Eigen::MatrixXf &grad,
                                               const std::vector<Eigen::MatrixXf> &rf) const {
    Shells shells(grad.template cast<double>());
    std::vector<Eigen::MatrixXf> basis;

    for (size_t s = 0; s < shells.count(); s++) {
      Eigen::MatrixXf B;
      if (rf.empty()) {
        B.setIdentity(Math::SH::NforL(lmax), Math::SH::NforL(lmax));
      } else {
        B.setZero(nc, Math::SH::NforL(lmax));
        size_t j = 0;
        for (auto &r : rf) {
          for (size_t l = 0; l < r.cols() and 2 * l <= lmax; l++)
            for (size_t i = l * (2 * l - 1); i < (l + 1) * (2 * l + 1); i++, j++)
              B(j, i) = r(s, l);
        }
      }
      basis.push_back(B);
    }

    return basis;
  }

  void init_Y(const Eigen::MatrixXf &grad, const Eigen::MatrixXf &motion) {
    DEBUG("initialise Y");
    assert(grad.rows() == nv); // one gradient per volume

    std::vector<size_t> idx = get_shellidx(grad);
    Y.resize(nv * ne, nc);

    Eigen::Vector3f vec;
    Eigen::Matrix3f rot;
    rot.setIdentity();
    Eigen::VectorXf delta;

    for (size_t v = 0; v < nv; v++) {
      vec = {grad(v, 0), grad(v, 1), grad(v, 2)};
      for (size_t e = 0; e < ne; e++) {
        // rotate vector with motion parameters
        rot = get_rotation(motion.row(v * ne + e));
        // evaluate basis functions
        Math::SH::delta(delta, rot * vec, lmax);
        Y.row(v * ne + e) = shellbasis[idx[v]] * delta;
      }
    }
  }

  inline Eigen::Matrix3f get_rotation(const Eigen::VectorXf &p) const {
    Eigen::Matrix3f m = se3exp(p).topLeftCorner<3, 3>();
    return m;
  }

  inline size_t get_ncoefs(const std::vector<Eigen::MatrixXf> &rf) const {
    size_t n = 0;
    if (rf.empty()) {
      n = Math::SH::NforL(lmax);
    } else {
      for (auto &r : rf)
        n += Math::SH::NforL(std::min(2 * (int(r.cols()) - 1), lmax));
    }
    return n;
  }

  std::vector<size_t> get_shellidx(const Eigen::MatrixXf &grad) const {
    Shells shells(grad.template cast<double>());
    std::vector<size_t> idx(shells.volumecount());

    for (size_t s = 0; s < shells.count(); s++) {
      for (auto v : shells[s].get_volumes())
        idx[v] = s;
    }
    return idx;
  }
};

template <class ImageType> class QSpaceMapping : public Adapter::Base<QSpaceMapping<ImageType>, ImageType> {
public:
  using base_type = Adapter::Base<QSpaceMapping<ImageType>, ImageType>;
  using value_type = typename ImageType::value_type;
  using vector_type = typename Eigen::Matrix<value_type, Eigen::Dynamic, 1>;

  using base_type::parent;

  QSpaceMapping(const ImageType &parent, const QSpaceBasis &basis) : base_type(parent), basis(basis) {
    assert(parent.ndim() == 4);
    assert(parent.size(3) == basis.get_ncoefs());
    assert(parent.stride(3) == 1);
    set_shotidx(0);
  }

  FORCE_INLINE size_t ndim() const { return 3; }

  FORCE_INLINE ssize_t get_index(size_t axis) const { return parent().get_index(axis); }
  FORCE_INLINE void move_index(size_t axis, ssize_t increment) { parent().move_index(axis, increment); }

  FORCE_INLINE value_type value() const {
    assert(parent().index(3) == 0);
    Eigen::Map<vector_type> c = {parent().address(), qr.size()};
    return qr.dot(c);
  }

  FORCE_INLINE void adjoint_add(value_type val) {
    assert(parent().index(3) == 0);
    Eigen::Map<vector_type> c = {parent().address(), qr.size()};
    c += val * qr;
  }

  FORCE_INLINE void set_shotidx(size_t idx) { qr = basis.get_projection(idx); }

private:
  const QSpaceBasis &basis;
  vector_type qr;
};

} // namespace MR::DWI::SVR
