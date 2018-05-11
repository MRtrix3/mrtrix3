# PKGBUILD file for MSYS2

This is used to create the installable MRtrix3 package on MSYS2. This will only
create the x86-64 version of the package. To use it, clone this repo and invoke
the `makepkg` command within the toplevel folder:


```
git clone https://github.com/MRtrix3/MinGW.git
cd MinGW/
makepkg
```

This will generate the package file, which can be installed using this command
(edit as appropriate):

```
pacman -U mingw-w64-x86_64-mrtrix3-3.0_RC3-1-x86_64.pkg.tar.xz
```

#### Dependencies

The `makepkg` command will check for the required dependencies, and report those
missing. Use `pacman -S` to install them, and try again.


