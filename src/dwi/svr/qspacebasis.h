/* Copyright (c) 2017-2019 Daan Christiaens
 *
 * MRtrix and this add-on module are distributed in the hope
 * that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 */

#ifndef __dwi_svr_qspacebasis_h__
#define __dwi_svr_qspacebasis_h__


#include <Eigen/Dense>

#include "types.h"
#include "math/SH.h"
#include "dwi/shells.h"
#include "adapter/base.h"

#include "dwi/svr/param.h"


namespace MR
{
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
          {  }

          FORCE_INLINE size_t ndim () const { return 3; }

          FORCE_INLINE value_type value () const {
            vector_type c = parent().row(3);
            value_type s = basis.get_projection(v_, z_).adjoint() * c;
            return s;
          }

          FORCE_INLINE void adjoint_add (value_type val) {
            if (val != 0)
              parent().row(3) += val * basis.get_projection(v_, z_);
          }

          FORCE_INLINE void set_encoding_index (size_t v, size_t z) { v_ = v; z_ = z; }

        private:
          QSpaceBasis basis;
          size_t v_, z_;
      };


    }
  }

}


#endif
