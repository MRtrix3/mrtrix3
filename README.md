# Installing MRtrix3 on macOS

MRtrix3 can be installed easily on macOS by copy-pasting the following command in a Terminal prompt:

```
bash -c "$(curl -fsSL https://raw.githubusercontent.com/MRtrix3/macos-installer/master/install)"
```

The ``install``-script will download the binaries for the latest MRtrix3 release, unpack them to ~/Applications/mrtrix3, and modify the $PATH eviornment variable to make the MRtrix commands available on the Terminal.

If a prior installation of MRtrix3 exists in the same location, it will warn the user and ask for permission to replace the existing installation.

# For developers/power users
``./build`` will download and build MRtrix3 and all its dependencies, create Application bundles for MRView and SHview (including icon sets), and package everything up in a tarball.

``./install`` will download the latest tarball that was generated using ``build``, unpack it to ~/Applications/mrtrix3, and modify the $PATH eviornment variable to make the MRtrix commands available on the Terminal. Note that it will request permission to replace an existing MRtrix3 installation.

``./install -f`` will will replace an existing installation without further user interaction. 
