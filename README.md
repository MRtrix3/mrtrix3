# Installing MRtrix3 on macOS

MRtrix3 can be installed easily on macOS by copy-pasting the following command in a Terminal prompt:

```
sudo bash -c "$(curl -fsSL https://raw.githubusercontent.com/MRtrix3/macos-installer/master/install)"
```

The ``install``-script will download the binaries for the latest MRtrix3 release, unpack them to /usr/local/mrtrix3. In addition it will create the appropriate symlinks in /usr/local/bin and /Applications.

Uninstallation is as simple as:
```
sudo bash -c "$(curl -fsSL https://raw.githubusercontent.com/MRtrix3/macos-installer/master/uninstall)"
```

# For developers/power users
``./build`` will download and build MRtrix3 and all its dependencies, create Application bundles for MRView and SHview (including icon sets), and package everything up in a tarball.

``./install`` will download the latest tarball that was generated using ``build`` and unpack it to /usr/local/mrtrix3. In addition, it will create the appropriate symlinks in /usr/local/bin and /Applications.

``./uninstall`` will remove the installation at /usr/local/mrtrix3. In addition, it will remove corresponding symlinks in /usr/local/bin and /Applications.

Adding `` -f`` to ``./(un)install`` will skip user dialogs.
