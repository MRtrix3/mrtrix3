# For users:

Regular users are probably more interested in installing the precompiled packages. Please see [the relevant page on the main MRtrix3 wiki for instructions](https://github.com/MRtrix3/mrtrix3/wiki/Precompiled-MSYS2-packages).

# For package maintainers:

## PKGBUILD file for MSYS2

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

### Dependencies

The `makepkg` command will check for the required dependencies, and report those
missing. Use `pacman -S` to install them, and try again.


## Building the Qt5 package

From an up to date MSYS2 installation, clone MRtrix3/MinGW repo, `cd` into its `qt5` folder, then issue the following command:
```
MINGW_INSTALLS=mingw64 makepkg-mingw -sCLf
```
One thing to watch out for is that due to Windows filesystem limitations (260 character maximum for the full path), the build may fail if you start it from a folder whose full path is already quite long... You may need to move this close to the filesystem root and avoid long folder names.

## Uploading the package(s) to the repo

1. clone the `MRtrix3.github.io` repo from the `MRtrix3` GitHub organisation, and `cd` into it. If you've already cloned the repo, make sure it's up to date with a `git pull`.

2. copy the package file(s) into the `msys2/` folder, and add them to the git staging area with a `git add msys2/<package_file>`

3. `cd msys2/` into that folder, and add the file(s) to the repo database with this command:
   ```
   repo-add mrtrix3.db.tar.gz <package_file>
   ```

4. `git add mrtrix3.db.tar.gz mrtrix3.files.tar.gz` to stage the files generated in the previous step.

5. Go back to the main folder with `cd ..`, then `git commit` and `git push` to upload to the repo.

5. Wait a few minutes for GitHub's servers to update the website (using Jekyll). 

**NOTE:** Do *not* use `git commit -a`, as this will probably cause the required symbolic links to end up in a mess (due to limitations on MSYS2). To avoid this, make sure you _don't_ add `mrtrix3.db` and `mrtrix3.files` to your commit, which will leave the required symbolic links unchanged  on the parent repo. This shouldn't happen if you follow the instructions above exactly. 

If you do end up committing the links by mistake, you may need to clone the repo onto a Unix system that supports symbolic links, and fix up the links. It should look like this:
```
mingw-w64-x86_64-mrtrix3-3.0_RC3_latest-1-x86_64.pkg.tar.xz
mingw-w64-x86_64-qt5-mrtrix-5.13.2-1-any.pkg.tar.xz
mrtrix3.db -> mrtrix3.db.tar.gz
mrtrix3.db.tar.gz
mrtrix3.files -> mrtrix3.files.tar.gz
mrtrix3.files.tar.gz
```
