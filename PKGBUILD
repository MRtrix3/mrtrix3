# Maintainer: J-Donald Tournier <jdtournier@gmail.com>

_realname=mrtrix3
pkgbase=mingw-w64-${_realname}
pkgname="${MINGW_PACKAGE_PREFIX}-${_realname}"
pkgver=0
pkgrel=1
arch=('x86_64')
pkgdesc="Tools for the analysis of diffusion MRI data (mingw-w64)"
depends=("python"
         "${MINGW_PACKAGE_PREFIX}-qt5-svg"
         "${MINGW_PACKAGE_PREFIX}-fftw"
         "${MINGW_PACKAGE_PREFIX}-libtiff"
         "${MINGW_PACKAGE_PREFIX}-zlib")
makedepends=("git"
             "python"
             "pkg-config"
             "${MINGW_PACKAGE_PREFIX}-gcc"
             "${MINGW_PACKAGE_PREFIX}-qt5-svg"
             "${MINGW_PACKAGE_PREFIX}-eigen3"
             "${MINGW_PACKAGE_PREFIX}-fftw"
             "${MINGW_PACKAGE_PREFIX}-libtiff"
             "${MINGW_PACKAGE_PREFIX}-zlib")


license=("MPL")
url="http://www.mrtrix.org/"
source=("${_realname}::git+https://github.com/GIT_USER/${_realname}.git#tag=TAGNAME")
sha512sums=('SKIP')


pkgver() {
  cd "${_realname}"
  git describe --abbrev=8 | tr '-' '_'
}

build() {
  cd "${_realname}"
  ./configure
  ./build
}

package() {

  cd "${_realname}"

  install -d "${pkgdir}${MINGW_PREFIX}/bin/"
  install -D bin/* "${pkgdir}${MINGW_PREFIX}/bin/"

  for fname in $(find lib/ share/ -type f); do
    install -Dm644 "${fname}" "${pkgdir}${MINGW_PREFIX}/${fname}"
  done

  mkdir -p "${pkgdir}${MINGW_PREFIX}/share/licenses/${_realname}-${pkgver}"
  cp LICENCE.txt "${pkgdir}${MINGW_PREFIX}/share/licenses/${_realname}-${pkgver}"
}

