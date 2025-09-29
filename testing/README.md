# Testing

Add tests here for MRtrix3 commands to ensure that the output is consistent with expectation.
If you perform any modifications to MRtrix3,
it will be necessary to ensure that existing tests still pass before any changes can be merged;
further, if adding new commands or functionalities,
it is highly recommended that new tests be added to evaluate such.

## Running tests

### Build configuration

In order for it to be possible to execute tests,
`cmake` must be instructed at configure time to build the requisite tools
using the `cmake` variable "`MRTRIX_BUILD_TESTS`":
```ShellSession
cmake -B build -DMRTRIX_BUILD_TESTS=ON ...
cmake --build build
```

Subsequent instructions here are written
assuming that the `cmake` build directory is named "`build`".

### Execution of all tests

To run tests,
use command `ctest` (an executable bundled with CMake),
executing from inside the `cmake` build directroy:
```ShellSession
cd build
ctest
```

Note that some tests involve the invocation of commands
that are not a part of the *MRtrix3* software.
As such, these tests may fail on some system configuration,
but this is not necessarily indicative of an issue with *MRtrix3* source code.

### Test execution via labels

The *MRtrix3* tests are assigned multiple labels that allow one to nominate
an appropriate subset of tests to be executed by `ctest`.

The following test labels are ascribed to tests as appropriate
to facilitate appropriate batch testing:

```ShellSession
cd build
ctest -L binary # Runs all C++ binary command tests
ctest -L script # Runs all Python command tests
ctest -L unittest # Runs all unit tests
ctest -L pythonci # Runs a small subset of Python command tests suitable for Continuous Integration (CI) testing
```

For each command in *MRtrix3*,
a corresponding test label is constructed using the name of that command,
which contains all tests for that command:

```ShellSession
cd build
ctest -L mrconvert # Runs all tests of the C++ command mrconvert
ctest -L dwi2response # Runs all tests of the Python command dwi2response
```

### Advanced usage

-   You can choose to rerun only those tests that previously failed
    by specifying the `--rerun-failed` option.

-   You don't need to build every command to test one particular command.
    For example, you can do this:
    ```ShellSession
    cmake --build build --target mrconvert testing_tools
    cd build
    ctest -L bin_mrconvert
    ```

    Note that, in addition to the command you want to test,
    you also need to build target "`testing_tools`"
    (an umbrella target that builds the tools needed to evaluate the outputs generated
    by commands when conducting tests).

-   Regular expressions can be used to select a subset of tests:
    ```ShellScript
    cd build
    ctest -R 5ttgen_hsvs # Run only the tests of specifically the "HSVS" algorithm of the 5ttgen command
    ```

-   Environment variables "`MRTRIX_BINARIES_DATA_DIR`" and "`MRTRIX_SCRIPTS_DATA_DIR`"
    can be used to specify alternative locations to be used by `cmake`
    to clone the appropriate versions of those data within the build directory.
    By setting these to contain local filesystem locations,
    internet traffic will be reduced if deleting build directories or using multiple build configurations;
    it additionally enables the execution of `ctest` utilising updated test data
    prior to pushing the updated test data to GitHub
    (see "Adding test data" step 7 below).

## Adding tests

A new text file should be added to one of the following locations,
depending on the nature of the test to be added:

```
testing/binaries/tests/cmdname/
testing/scripts/tests/cmdname/
testing/unit_tests/
```

"`cmdname`" in these examples should be replaced with the name of the *MRtrix3* command
for which a new test is being added.
The name of the file should communicate the nature of the test being performed.
The test should be constructed in such a way
that a non-zero return code is yielded if an error is encountered
or the data produced does not conform to expectation.
Directory `testing/tools/` provides a set of commands
that can be useful for detecting divergence of the output produced by a command
from that produced by an earlier version of the software.

Note that this script will be invoked directly parsed by the build system.
It therefore does not need to be executable,
no redirection setup is necessary, 
and no hash-bang line is required to specify the interpreter.
Just add commands to be run, and if any of them produce a non-zero exit code,
this will be reported when running `ctest`.
All command outputs will also be logged. 

## Temporary files 

If your tests need to create temporary files,
make sure that they are prefixed with "`tmp`" and are not placed in subfolders;
any such content will be deleted prior to running the next set of tests. 

## Adding test data

If necessary, new data can be added to the testing suite,
in the form of exemplar input data
and/or data produced by the command and manually verified to be correct
in order to detect any future regression of such.

As with the tests themselves,
the appropriate location for such data depends on the nature of the test:

-   C++ binary commands: [`test_data` repo](https://github.com/MRtrix3/test_data).

-   Python commands: [`script_test_data` repo](https://github.com/MRtrix3/script_test_data).

-   Unit tests: Some limited data exist in `testing/data/`;
    consult with the package maintainers if you believe this may be suitable for your use case.

Test data should be kept as small as possible whilst fulfilling the test requirements,
to minimise both the size of the repositories and the computational expense of the tests.
For C++ binaries, images of only a small zoomed FoV are frequently used;
for Python commands that necessitate whole-brain data,
images of reduced spatial resolution are commonly used.

Data used for testing will be downloaded at build time.
Each test when executed has its working directory set
to the root path of the corresponding data within the build directory;
in this way, the contents of each test script can reference these data
using paths relative to that root.

Proper integration of changes to the software and corresponding changes to test data
can be tricky where the changes occur across different repositories.
The following instructions are recommendations from @Lestropie
following years of experience with this mechanism.
In the following instructions,
"source code repository" is in reference to [`github.com/MRtrix3/mrtrix3`](https://github.com/MRtrix3/mrtrix3),
and "data repository" is in reference to either [`github.com/MRtrix3/test_data`](https://github.com/MRtrix3/test_data)
or [`github.com/MRtrix3/script_test_data`](https://github.com/MRtrix3/script_test_data)
depending on the nature of the proposed modification.

1.  Determine the target branch for the source code repository for your modifications.
    As per the [contributing guidelines](https://github.com/MRtrix3/mrtrix3/CONTRIBUTING.md),
    the suitable target branch for a Pull Request depends on the nature
    of the change being proposed.

2.  Determine the data repository for which test data will need to be modified.
    This will depend on the language in which the command under modification / being added is written.

3.  Independently obtain the following two commit hash identifiers:

    1.  Within file `testing/CMakeLists.txt` in the source code repository,
        the version of the test data repository that is currently downloaded
        whenever executing tests for the target source branch.

    2.  Within the data repository,
        the branch of the same name as the target branch in the source code repository.

    If these identifiers do *not* match,
    then it is highly recommended that this be rectified before proceeding.
    It is best to attempt to trace the source of the discrepancy
    rather than simply moving a branch from one commit to another,
    as divergences are harder to detect and resolve here
    then when dealing with only a single `git` repository.

4.  Clone the relevant data repository
    (if you do not yet have such a clone).

5.  Within your clone of the relevant data repository,
    create a new branch based off of the branch identified in step 3.2.
    It is highly recommended to give this branch in the data repository
    the same name as the relevant branch in the source code repository.

6.  Create a commit adding or modifying test data.

7.  Push the updated branch to GitHub.
    Note that if `cmake` is configured to use local clones
    of the test data repositories rather than GitHub
    (as explained in "Running tests" -> "Advanced usage" above),
    then this step will not be necessary for verifying test suite success
    on your local machine;
    test data changes will however nevertheless eventually
    need to be pushed to GitHub
    in order for other developers,
    or indeed Continuous Integration (CI) checks,
    to be able to acquire the updated data.

8.  In your clone of the source code repository:

    1.  In file `testing/CMakeLists.txt`,
        find the invocation of the ExternalProject_Add() function
        corresponding to the relevant data repository,
        and change the content of argument "`GIT_TAG`"
        to reflect the commit created by step 6.

    2.  Make any other additions or modifications to tests
        that do not involve modification of test *data*.

    3.  Create a commit that includes changes from both steps 8.1 and 8.2,
        and push the updated source code repository branch to GitHub.

9.  Ensure that within the Pull Request,
    no merge conflict occurs at the location modified by step 8.1.
    A merge conflict here indicates that on the target source branch,
    there has been evolution of the test data accessed by that branch
    in parallel to the test data changes made in step 6.
    Resolving this necessitates explicit merging of changes
    between the corresponding branches in the test data repository,
    followed by repeating step 8.1 and 8.3 to access the result of this merge.

10. After the relevant Pull Request on the source code repository is merged:

    1.  For the data repository branch that has the same name
        as the target branch of the Pull Request,
        do a *force push* that results in that branch
        pointing to the same version of the test data
        that is now accessed by the updated source code.

    2.  Delete from GitHub the branch on the data repository
        that had the same name as the feature branch merged by the Pull Request.

By following this sequence,
the branches contained within each test data repository should be limited to:

-   A `master` branch,
    corresponding to the very same commit that is accessed
    by the `master` branch of the source code repository;

-   A `dev` branch,
    corresponding to the very same commit that is accessed
    by the `dev` branch of the source code repository;

-   Additional branches corresponding only to those active branches
    where corresponding test data changes are necessary,
    ideally with the same branch name as that of the source code branch,
    and with the tip of that branch kept up to date with the commit hash
    that is downloaded when building that source code branch.
