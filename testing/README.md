# Testing

Add tests here for MRtrix3 commands to ensure that the output is consistent
with expectation. If you intend to help with development of MRtrix3, it is
recommended you run the enclosed tests regularly to ensure your changes do not
introduce regressions. 

## To run the tests

Simply run the `./run_tests` script from within the MRtrix3 toplevel folder:

        ./run_tests
        
This will fetch the testing data as a submodule from a separate dedicated repo,
build the testing executables, then run the tests. All activities are logged to
the `testing.log` file - take a look in there for details of any failures.

Note that the `./run_tests` script will _not_ build the executables to be tested.
This is to allow developers to build and test individual commands without
needing to rebuild all other commands. If you want to test all commands, make
sure you run `./build` first.


## Adding tests
 
Add a script to the `tests/` folder. Each line of these scripts constitutes a
single test, and will be run as a single unit. Use && and || bash constructs if
needed to create compound commands. Each of these lines should return a zero
exit code if successful. You can test the output of your commands against your
expected output using the `testing_diff_image` command (note other commands are
available to check various types of output). 

Note that this script will be invoked directly in the context set up by the
`run_tests` script, so does not need to be executable, or to set up any
redirection, or to uses a hash-bang line to specify the interpreter.  Just add
commands to be run, and if any of them produce a non-zero exit code, this will
be caught by the `run_tests` script.  All commands will also be logged. 

The testing will consider each line of the test scripts to correspond to an
individual test, and report as such. If any of your tests need multiple
commands, simply list them all on the same line, separated by semicolons or
other constructs such as `&&` or `||`. Look within existing test scripts for
examples.

#### Temporary files 

If your tests need to create temporary files, make sure they are prefixed with
'tmp' and are not placed in subfolders - the run_tests script will make sure
these are deleted prior to running the next set of tests. 

## Adding test data

If needed, you can add test data to the [test_data
repo](https://github.com/MRtrix3/test_data), preferably within its own
subfolder if you don't anticipate these data will be suitable for testing other
commands. Please keep the size of these data small to ensure this repository
doesn't grow too large. In general, you really don't need large images to
verify correct operation.

These data are included as a _submodule_ within the main MRtrix3 repository, as
a means of ensuring that the version of the data used for the tests is recorded
within the main repo, while keeping its history separate, and hence on a
separate repository. This separation helps to keep the size of the main
repository small, and avoid large downloads for users. The downside is that
making changes to the test data is more complicated. For [here a full introduction
into git submodules](https://git-scm.com/book/en/v2/Git-Tools-Submodules).

To add data, you will need to:

- add your data to the `master` branch of the [test_data repo](https://github.com/MRtrix3/test_data);

- update the main MRtrix3 repo to reflect the new version of the data needed
  for the tests.

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
  
3. Directly within the MRtrix3 installation
  
  Adding data can be done by pushing commits on the `master` branch of the
  submodule within the MRtrix3 folder. Note that you will need to have run
  `./run_tests` at least once to initialise the submodule. For this to work, you
  will need to override the default URL for the [test_data
  submodule](https://github.com/MRtrix3/test_data) in your local
  installation, since the default URL is the public HTTPS version, which is
  read-only. This only needs to be done once, using the following instructions: 
  
  ```ShellSession
  $ git config submodule.testing/data.url git@github.com:MRtrix3/test_data.git
  $ rm -rf testing/data .git/modules/testing/
  $ ./run_tests
  ```
  After this, you should be able to add data by committing directly to the
  `master` branch. Note that by default, the submodule will be in detached HEAD
  state, since it is locked to the specific version specified in the main MRtrix3
  folder. Remember to checkout `master` before commtting:
  
  ```ShellSession
  $ cd testing/data
  $ git checkout master
  $ git add your_data.mif
  $ git commit 
  $ git push
  ```
  
#### Updating the main MRtrix3 repo to make use of the latest commits

Once your new data have been committed to the [test_data
repo](https://github.com/MRtrix3/test_data), all you need to do is tell the
main MRtrix3 repository to record the new SHA1 for the latest version of the
test data:
```ShellSession
$ git submodule update --remote
```

At this point, you can commit and push this change, or optionally add your
tests in the same commit:
```ShellSession
$ git add testing/data
$ git add testing/tests/mycommand    # <- optional
$ git commit
$ git push
```
  


