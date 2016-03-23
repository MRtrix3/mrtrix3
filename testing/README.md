# Testing

Add tests here for MRtrix3 commands to ensure that the output is consistent
with expectation. If you intend to help with development of MRtrix3, it is
recommended you run the enclosed tests regularly to ensure your changes do not
introduce regressions. 

## To run the tests

Simply run the `./run_tests` script:

        ./run_tests
        
This will fetch the testing data from a separate dedicated repo, build the
testing executables, then run the tests. All activities are logged to the
`testing.log` file - take a look in there for details of any failures.


## Adding tests
 
Add a script to the `tests/` folder. Each line of these scripts constitutes a
single test, and will be run as a single unit. Use && and || bash constructs if
needed to create compound commands. Each of these lines should return a zero
exit code if successful.

You can add test data to the [test_data
repo](https://github.com/MRtrix3/test_data), preferably within its own
subfolder if you don't anticipate these data will be suitable for testing other
commands. You can test the output of your commands against your expected output
using the `testing_diff_data` command (note other commands are available to
check various types of output). Please keep the size of these data small to
ensure this repository doesn't grow too large. In general, you really don't
need large images to verify correct operation...

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

## Temporary files 

If your tests need to create temporary files, make sure they are prefixed with
'tmp' and are not placed in subfolders - the run_tests script will make sure
these are deleted prior to running the next set of tests. 
