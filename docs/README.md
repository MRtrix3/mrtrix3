## MRtrix3 documentation generation

The [online documentation for *MRtrix3*](http://mrtrix.readthedocs.org/) is generated based on the contents of this `docs/` directory. It is additionally possible to generate a version of this documentation locally, which could theoretically then be produced in a different format (e.g. PDF) or with a different visual style. This document outlines the steps necessary to locally reproduce the documentation as it appears online.

1. If there have been local modifications made to *MRtrix3* executables (whether compiled C++ binaries or Python scripts / algorithm files), then the contents of specifically the `docs/reference/` directory may not match the contents of the commands' help pages / available config file options / environment variables as modified through those changes. In order to ensure that the documentation pages are concordant with the local code changes:
   (from the *MRtrix3* root directory):
   ```
   $ ./build
   $ ./docs/generate_user_docs.sh
   ```

2. Install requisite packages via your distribution's package manager (exact package names may vary between different platforms) (may require `sudo`):

   -  `python-pip`

   -  `python-sphinx`
      (may not be installable from package manager on all platforms)

3. Install requisite Python packages using `pip` (may require `sudo`):

   -  `pip install recommonmark sphinx sphinx-notfound-page sphinx_rtd_theme typing`
      (should not need to include `sphinx` in this list if it was installed via the distribution package manager in step 2)

4. Compile the documentation:

   ```
   $ python -m sphinx -n -N -w sphinx.log docs/ compiled_docs/
   ```
