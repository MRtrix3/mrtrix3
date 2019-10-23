/* Copyright (c) 2017-2019 Daan Christiaens
 *
 * MRtrix and this add-on module are distributed in the hope
 * that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 */

#ifndef __dwi_svr_qspacebasis_h__
#define __dwi_svr_qspacebasis_h__


#include <atomic>
#include <Eigen/Dense>

#include "types.h"
#include "math/SH.h"
#include "dwi/shells.h"
#include "adapter/base.h"
#include "algo/loop.h"

#include "dwi/svr/param.h"


namespace MR
{
  namespace Adapter
  {
    template <class ImageType>
    class Cache : public Adapter::Base<Cache<ImageType>, ImageType>
    {
      MEMALIGN (Cache<ImageType>)
      public:
        using base_type = Adapter::Base<Cache<ImageType>, ImageType>;
        using value_type = typename ImageType::value_type;

        using base_type::parent;

        Cache (const ImageType& parent, bool readonly = true)
          : base_type (parent), readmode (readonly)
        {
          Header hdr (parent);
          buffer = Image<value_type>::scratch(hdr, "temporary buffer");
          mask = Image<uint8_t>::scratch(hdr, "temporary buffer mask");
          // initialise lock image
          static_assert (sizeof(std::atomic_flag) == sizeof(uint8_t), "std::atomic_flag expected to be 1 byte");
          if (!readmode) lock = Image<uint8_t>::scratch(hdr, "temporary buffer lock");
        }

        Cache (const Cache& other)
          : base_type (other.parent()), readmode (other.readmode), lock (other.lock)
        {
          Header hdr (other.parent());
          buffer = Image<value_type>::scratch(hdr, "temporary buffer");
          mask = Image<uint8_t>::scratch(hdr, "temporary buffer mask");
        }

        void move_index (size_t axis, ssize_t increment) {
          parent().index(axis) += increment;
          buffer.index(axis) += increment;
          mask.index(axis) += increment;
        }
        void reset () {
          parent().reset();
          buffer.reset();
          mask.reset();
        }

        void flush () {
          // delayed write back
          if (!readmode) {
            for (auto l = Loop() (mask); l; l++) {
              if (mask.value()) { assign_pos_of (mask).to (parent(), buffer, lock);
                std::atomic_flag* flag = reinterpret_cast<std::atomic_flag*> (lock.address());
                while (flag->test_and_set(std::memory_order_acquire)) ;
                parent().adjoint_add(buffer.value());
                flag->clear(std::memory_order_release);
              }
            }
          }
          // clear buffer
          reset();
          memset(mask.address(), 0, footprint<uint8_t>(voxel_count(mask)));
        }

        value_type value () {
          assert (readmode);
          if (mask.value()) {
            return buffer.value();
          } else {
            buffer.value() = parent().value();
            mask.value() = true;
            return buffer.value();
          }
        }

        void adjoint_add (value_type val) {
          assert (!readmode);
          if (mask.value()) {
            buffer.value() += val;
          } else {
            buffer.value() = val;
            mask.value() = true;
          }
        }

        FORCE_INLINE void set_shotidx (size_t idx) {
          flush();
          parent().set_shotidx(idx);
        }

      private:
        Image<value_type> buffer;
        Image<uint8_t> mask;
        bool readmode;
        Image<uint8_t> lock;
    };

    template <template <class ImageType> class AdapterType, class ImageType, typename... Args>
    inline Cache<AdapterType<ImageType>> makecached (const ImageType& parent, Args&&... args) {
      return { { parent, std::forward<Args> (args)... } , true };
    }

    template <template <class ImageType> class AdapterType, class ImageType, typename... Args>
    inline Cache<AdapterType<ImageType>> makecached_add (const ImageType& parent, Args&&... args) {
      return { { parent, std::forward<Args> (args)... } , false };
    }

  }

  namespace DWI
  {
    namespace SVR
    {

      class QSpaceBasis
      {
        MEMALIGN (QSpaceBasis)
        public:
          typedef Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> RowMatrixXf;

          QSpaceBasis (const Eigen::MatrixXf& grad, const int lmax,
                       const vector<Eigen::MatrixXf>& rf, const Eigen::MatrixXf& rigid)
            : lmax (lmax), nv (grad.rows()), ne (rigid.rows() / nv), nc (get_ncoefs(rf)),
              shellbasis (init_shellbasis(grad, rf))
          {
            init_Y(grad, rigid);
          }

          QSpaceBasis (const Eigen::MatrixXf& grad, const int lmax,
                       const vector<Eigen::MatrixXf>& rf)
            : lmax (lmax), nv (grad.rows()), ne (1), nc (get_ncoefs(rf)),
              shellbasis (init_shellbasis(grad, rf))
          {
            RowMatrixXf zero (nv, 6);
            init_Y(grad, zero);
          }

          FORCE_INLINE Eigen::Ref<const Eigen::VectorXf> get_projection (size_t idx) const { return Y.row(idx); }
          FORCE_INLINE Eigen::Ref<const Eigen::VectorXf> get_projection (size_t v, size_t z) const { return Y.row(v*ne + z); }

          FORCE_INLINE size_t get_ncoefs () const { return nc; }
          FORCE_INLINE const RowMatrixXf& getY () const { return Y; }
          FORCE_INLINE const Eigen::MatrixXf& getShellBasis (const int shellidx) const { return shellbasis[shellidx]; }

        private:
          const int lmax;
          const size_t nv, ne, nc;
          const vector<Eigen::MatrixXf> shellbasis;
          RowMatrixXf Y;

          vector<Eigen::MatrixXf> init_shellbasis(const Eigen::MatrixXf& grad, const vector<Eigen::MatrixXf>& rf) const
          {
            Shells shells (grad.template cast<double>());
            vector<Eigen::MatrixXf> basis;

            for (size_t s = 0; s < shells.count(); s++) {
              Eigen::MatrixXf B;
              if (rf.empty()) {
                B.setIdentity(Math::SH::NforL(lmax), Math::SH::NforL(lmax));
              }
              else {
                B.setZero(nc, Math::SH::NforL(lmax));
                size_t j = 0;
                for (auto& r : rf) {
                  for (size_t l = 0; l < r.cols() and 2*l <= lmax; l++)
                    for (size_t i = l*(2*l-1); i < (l+1)*(2*l+1); i++, j++)
                      B(j,i) = r(s,l);
                }
              }
              basis.push_back(B);
            }

            return basis;
          }

          void init_Y(const Eigen::MatrixXf& grad, const Eigen::MatrixXf& motion)
          {
            DEBUG("initialise Y");
            assert (grad.rows() == nv);     // one gradient per volume

            vector<size_t> idx = get_shellidx(grad);
            Y.resize(nv*ne, nc);

            Eigen::Vector3f vec;
            Eigen::Matrix3f rot;
            rot.setIdentity();
            Eigen::VectorXf delta;

            for (size_t v = 0; v < nv; v++) {
              vec = {grad(v, 0), grad(v, 1), grad(v, 2)};
              for (size_t e = 0; e < ne; e++) {
                // rotate vector with motion parameters
                rot = get_rotation(motion.row(v*ne+e));
                // evaluate basis functions
                Math::SH::delta(delta, rot*vec, lmax);
                Y.row(v*ne+e) = shellbasis[idx[v]]*delta;
              }
            }

          }

          inline Eigen::Matrix3f get_rotation(const Eigen::VectorXf& p) const
          {
            Eigen::Matrix3f m = se3exp(p).topLeftCorner<3,3>();
            return m;
          }

          inline size_t get_ncoefs(const vector<Eigen::MatrixXf>& rf) const
          {
            size_t n = 0;
            if (rf.empty()) {
              n = Math::SH::NforL(lmax);
            } else {
              for (auto& r : rf)
                n += Math::SH::NforL(std::min(2*(int(r.cols())-1), lmax));
            }
            return n;
          }

          vector<size_t> get_shellidx(const Eigen::MatrixXf& grad) const
          {
            Shells shells (grad.template cast<double>());
            vector<size_t> idx (shells.volumecount());

            for (size_t s = 0; s < shells.count(); s++) {
              for (auto v : shells[s].get_volumes())
                idx[v] = s;
            }
            return idx;
          }

      };


      template <class ImageType>
      class QSpaceMapping : public Adapter::Base<QSpaceMapping<ImageType>, ImageType>
      {
        MEMALIGN (QSpaceMapping<ImageType>)
        public:
          using base_type = Adapter::Base<QSpaceMapping<ImageType>, ImageType>;
          using value_type = typename ImageType::value_type;
          using vector_type = typename Eigen::Matrix<value_type, Eigen::Dynamic, 1>;

          using base_type::parent;

          QSpaceMapping (const ImageType& parent, const QSpaceBasis& basis)
            : base_type (parent), basis (basis)
          {
            assert (parent.ndim() == 4);
            assert (parent.size(3) == basis.get_ncoefs());
            set_shotidx(0);
          }

          FORCE_INLINE size_t ndim () const { return 3; }

          value_type value () {
            value_type res = 0; size_t i = 0;
            for (auto l = Loop(3) (parent()); l; l++, i++)
              res += qr[i] * parent().value();
            return res;
          }

          void adjoint_add (value_type val) {
            if (val != 0) {
              size_t i = 0;
              for (auto l = Loop(3) (parent()); l; l++, i++)
                parent().value() += qr[i] * val;
            }
          }

          FORCE_INLINE void set_shotidx (size_t idx) {
            qr = basis.get_projection(idx);
          }

        private:
          const QSpaceBasis& basis;
          vector_type qr;
      };


    }
  }

}


#endif
