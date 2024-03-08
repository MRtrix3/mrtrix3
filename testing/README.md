# Testing

Add tests here for MRtrix3 commands to ensure that the output is consistent
with expectation. If you intend to help with development of MRtrix3, it is
recommended you run the enclosed tests regularly to ensure your changes do not
introduce regressions. 

Recommended workflow to fully test everything is working:
```ShellSession
cmake -B build -DMRTRIX_BUILD_TESTS=ON
cmake --build build
cd build && ctest
```

## To run the tests
To run the tests, you will need to use `ctest` (an executable bundled with CMake)
as follows:
```ShellSession
cd build_directory
ctest
```

Note that this requires you to build the project enabling the `MRTRIX_BUILD_TESTS`
option when configuring CMake. Only once you have sucessfully built the project, you'll be 
able to run the tests.

### Running tests for specific commands

In order to run a specific set of tests, `ctest` allows you to make use of regex expressions, for example:

```ShellSession
ctest -R unit # Runs all unit tests
ctest -R bin # Runs all binary tests
ctest -R bin_5tt2gmwmi # Runs the binary test for the 5tt2gmwmi command
ctest -E unit # Runs all tests, except unit tests
```

You can also choose to rerun tests have failed by specifying the `--rerun-failed` option.

You don't need to build every command to test one particular command.
For example, you can do this:
```ShellSession
cmake --build build --target mrconvert 
cd build
ctest -R bin_mrconvert
```

## Adding tests
 
Add a script to the `tests/` folder. Each line of these scripts constitutes a
single test, and will be run as a single unit. Use `&&` and `||` bash
constructs if needed to create compound commands. Each of these lines should
return a zero exit code if successful. You can test the output of your commands
against your expected output using the `testing_diff_image` command (note other
commands are available to check various types of output - look in `testing/tools`
for the full list). 


Note that this script will be invoked directly parsed by the build system, 
so does not need to be executable, or to set up any
redirection, or to uses a hash-bang line to specify the interpreter.  Just add
commands to be run, and if any of them produce a non-zero exit code, this will
be reported when running `ctest`.  All commands will also be logged. 

The testing will consider each line of the test scripts to correspond to an
individual test, and report as such. If any of your tests need multiple
commands, simply list them all on the same line, separated by semicolons or
other constructs such as `&&` or `||`. Look within existing test scripts for
examples.

#### Temporary files 

If your tests need to create temporary files, make sure they are prefixed with
'tmp' and are not placed in subfolders - these will be deleted prior to running the
next set of tests. 

## Adding test data

If needed, you can add test data to the [test_data
repo](https://github.com/MRtrix3/test_data), preferably within its own
subfolder if you don't anticipate these data will be suitable for testing other
commands. Please keep the size of these data small to ensure this repository
doesn't grow too large. In general, you really don't need large images to
verify correct operation.

Data used for testing will be downloaded at build time, ensuring that each test
has its working directory set to the root path of data in the build directory.

To add data, you will need to:

- add your data to the [test_data repo](https://github.com/MRtrix3/test_data);

- update the main MRtrix3 repo to reflect the new version of the data needed
  for the tests. To do this you will need to update the commit hash in `testing/CMakeLists.txt`
  to match the latest commit that corresponds to the change in the testing repo.

**Note:** if you need to _modify_ exising data in a way that would break
existing tests, you will need to be very careful with how you do this - see the
relevant section below for details.

#### Adding the data to the [test_data repo](https://github.com/MRtrix3/test_data) repository

This can be done using a number of approaches:

1. Using the GitHub website

  Probably the simplest approach is simply to upload your new data via the [GitHub
  interface](https://github.com/MRtrix3/test_data/upload/master). This will
  commit directly to the `master` branch of the  [test_data
  repo](https://github.com/MRtrix3/test_data). 

2. From a separate standalone repo:

  Simply clone the [test_data repo](https://github.com/MRtrix3/test_data), add
  your data, commit, and push:

  ```ShellSession
  $ git clone git@github.com:MRtrix3/test_data.git
  $ cd test_data
  $ git add your_data.mif
  $ git commit 
  $ git push
  ```
  
  You can delete the standalone repo once you have pushed your changes. 
  
#### Updating the main MRtrix3 repo to make use of the latest commits

Once your new data have been committed to the [test_data
repo](https://github.com/MRtrix3/test_data), all you need to do is tell the
main MRtrix3 repository to record the new SHA1 for the latest version of the
test data.

To do this, navigate to `testing/CMakeLists.txt` and update the commit hash in the 
`ExternalProject_Add` function for the testing repo.

## What to do when modifying existing test data  

Generally, _adding_ data to the [test_data
repo](https://github.com/MRtrix3/test_data) is straightforward, and has no
consequences for existing tests. However, there may be cases where changes in
the code modify the output of some commands, and the test data therefore
needs to be updated to match. If these modifications are pushed to the `master`
branch of the [test_data repo](https://github.com/MRtrix3/test_data),
everything will be fine initially, since the `master` branch will still pull in
the specific SHA1 commit that was recorded at the time.  However, problems will
occur if other developers also push data to the [test_data
repo](https://github.com/MRtrix3/test_data) _after_ that point,
since these changes will most likely be pushed to its `master` branch, on top
of the earlier modifications. 

For example, imagine the following situation:

- Jack fixes a critical bug in `dwi2fod` that leads to big change in the
  expected output, and commits these changes to branch `dwi2fod_bug_fix`.

- Jack then modifies the data used in the `dwi2fod` tests in the [test_data
  repo](https://github.com/MRtrix3/test_data), pushes these to its `master`
  branch, and records the new updated SHA1 into the `dwi2fod_bug_fix` branch.

- later on, Gill adds a new command to her own branch `featureX`

- Gill adds tests for hew new command, along with new test data. Following the
  instructions, she pushes this additional data onto the `master` branch of the
[test_data repo](https://github.com/MRtrix3/test_data).

- Gill's tests now fail, because her version of the test data now also
  includes Jack's modifications, but her `featureX` branch does not include
Jack's corresponding modifications to the code in `dwi2fod_bug_fix`. 

So this is clearly not the best outcome...

#### Solution: push modifications to separate branch

To avoid these issues, the best thing to do is to push any changes that
_modify_ existing data to a separate branch on the [test_data
repo](https://github.com/MRtrix3/test_data), and merge these to `master` when
the corresponding commit hash on the main MRtrix3 has correctly been updated. 
This allows other developers to keep pushing additional data to `master` without
anything breaking.

> Technically, there is no reason to merge the branch on the [test_data
> repo](https://github.com/MRtrix3/test_data) to `master` since CMake
> will pull in the relevant commit via its SHA1 checksum. However, merging to
> `master` ensures that the branch can be deleted without risk of losing the
> relevant commit.

To do this, developers need to push their changes to a different branch than
`master` on the [test_data repo](https://github.com/MRtrix3/test_data). 
For example, this procedure can be followed:
```ShellSession
$ git clone git@github.com:MRtrix3/test_data.git
$ cd test_data
$ git checkout -b dwi2fod_bug_fix
# make changes to test data...
$ git commit -a
$ git push origin dwi2fod_bug_fix
```
which should push the modifications on the `dwi2fod_bug_fix` branch only. 
Then you can update the commit hash in `testing/CMakeLists.txt` to be the latest
commit hash in `dwi2fod_bug_fix` branch.
