# Maintainer: cobra-r9 <cobra.rev.9@gmail.com>
pkgname=nex
pkgver=0.2.1
pkgrel=1
pkgdesc="An enhanced tiling WM for X11 - a fork of bspwm"
arch=('x86_64')
license=('MIT')
makedepends=('gcc' 'make' 'git')
depends=('libxcb' 'xcb-util' 'xcb-util-wm' 'xorg-xinit' 'sxhkd')
optdepends=('alacritty' 'rofi' 'picom' 'thunar')
source=()
sha256sums=()

build() {
    cd "$startdir"
    make
}

package() {
    cd "$startdir"
    install -Dm755 "build/bin/nex" "$pkgdir/usr/bin/nex"
    install -Dm755 "build/bin/nexl" "$pkgdir/usr/bin/nexl"
    install -Dm644 LICENSE "$pkgdir/usr/share/licenses/$pkgname/LICENSE"
    install -Dm644 readme.md "$pkgdir/usr/share/docs/$pkgname/README.md"
    cp -r docs/* "$pkgdir/usr/share/docs/$pkgname/"
    echo "Run 'make purge' to clean makepkg residues..."
}
