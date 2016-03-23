# Testing

Use this repository to run tests on MRtrix3 commands and ensure that the output
is consistent with expectation. If you intend to help with development of
MRtrix3, you will need to set this up to ensure your changes do not introduce
regressions. 

## Setup

- clone this repo:
    
          git clone https://github.com/MRtrix3/testing.git

    or if you have write access:

          git clone git@github.com:MRtrix3/testing.git

- create a symbolic link to the build script in your main MRtrix3 git clone.
  For example, if you cloned the testing repo within your main MRtrix3 folder:
    
          ln -s ../build

- run the `./run_tests` script:

        ./run_tests
        
    This will build the testing executables, then run the tests. All activities
    are logged to the `testing.log` file - take a look in there for details of any
    failures.


## Adding tests
 
Add a script to the `tests/` folder. Each line of these scripts constitutes a
single test, and will be run as a single unit. Use && and || bash constructs if
needed to create compound commands. Each of these lines should return a zero
exit code if successful.

You can add test data to the `data/` folder, preferably within its own
subfolder if you don't anticipate these data will be suitable for testing other
commands. You can test the output of your commands against your expected output
using the `testing_diff_data` data command. Please keep the size of these data
small to ensure this repository doesn't grow too large. In general, you really
don't need large images to verify correct operation...

Note that this script will be invoked directly in the context set up by the
`run_tests` script, so does not need to be executable, or to set up any
redirection, or to uses a hash-bang line to specify the interpreter.  Just add
commands to be run, and if any of them produce a non-zero exit code, this will
be caught by the `run_tests` script.  All commands will also be logged.

## Temporary files 

If your tests need to create temporary files, make sure they are prefixed with
'tmp' and are not placed in subfolders - the run_tests script will make sure
these are deleted prior to running the next set of tests. 
