# Maintainer: J-Donald Tournier <jdtournier@gmail.com>

_realname=mrtrix3
pkgbase=mingw-w64-${_realname}
pkgname="${MINGW_PACKAGE_PREFIX}-${_realname}"
pkgver=3.0_RC1
pkgrel=1
arch=('i686' 'x86_64')
pkgdesc="Tools for the analysis of diffusion MRI data (mingw-w64)"
depends=("python"
         "${MINGW_PACKAGE_PREFIX}-qt5"
         "${MINGW_PACKAGE_PREFIX}-zlib")
makedepends=("python"
             "pkg-config"
             "${MINGW_PACKAGE_PREFIX}-gcc"
             "${MINGW_PACKAGE_PREFIX}-qt5"
             "${MINGW_PACKAGE_PREFIX}-eigen3")

#options=('!strip' 'debug')
license=("MPL")
url="http://www.mrtrix.org/"
#install=${_realname}-${CARCH}.install
source=(https://github.com/MRtrix3/${_realname}/archive/$pkgver.tar.gz)
sha256sums=('27bb2ff726dbac0dc3470c2aebd7449c5c6fe4521f74ba3594b02b6e4b717e07')

prepare() {
  echo ${srcdir}
  echo ${_realname}
  cd "${srcdir}/${_realname}-${pkgver}"
  patch -p1 -i "../../0001-fix-legacy-fixel-code.patch"
}

build() {
  cd "${_realname}-${pkgver}"
  ./configure 
  ./build
}

package() {

  cd "${_realname}-${pkgver}"

  install -d "${pkgdir}${MINGW_PREFIX}/bin/"
  install -D bin/* "${pkgdir}${MINGW_PREFIX}/bin/"

  for fname in $(find lib/ share/ -type f); do
    install -Dm644 "${fname}" "${pkgdir}${MINGW_PREFIX}/${fname}"
  done

  mkdir -p "${pkgdir}${MINGW_PREFIX}/share/licenses/${_realname}-${pkgver}"
  cp LICENCE.txt "${pkgdir}${MINGW_PREFIX}/share/licenses/${_realname}-${pkgver}"
}
# dummy.
