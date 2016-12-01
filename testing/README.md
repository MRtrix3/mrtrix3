# Testing

Add tests here for MRtrix3 commands to ensure that the output is consistent
with expectation. If you intend to help with development of MRtrix3, it is
recommended you run the enclosed tests regularly to ensure your changes do not
introduce regressions. 

Recommended command to fully test everything is working:
```ShellSession
./configure && ./build && ./run_tests 
```
Look in `testing.log` for details of any failures encountered. 

## To run the tests

Simply run the `./run_tests` script from within the MRtrix3 toplevel folder:
```ShellSession
./run_tests
```
        
This will fetch the testing data as a submodule from a separate dedicated repo,
build the testing executables, then run the tests. All activities are logged to
the `testing.log` file - take a look in there for details of any failures.

Note that the `./run_tests` script will _not_ build the executables to be tested.
This is to allow developers to build and test individual commands without
needing to rebuild all other commands. If you want to test all commands, make
sure you run `./build` first.

### Running tests for a single command

You can run a test for a single command simply by adding the name of the
command as the argument:

```ShellSession
./run_tests mrconvert

```

This means you don't need to build everything to test one particular command.
For example, this is a construct I commonly use:
```ShellSession
./build release/bin/mrconvert && ./run_tests mrconvert
```

## Adding tests
 
Add a script to the `tests/` folder. Each line of these scripts constitutes a
single test, and will be run as a single unit. Use `&&` and `||` bash
constructs if needed to create compound commands. Each of these lines should
return a zero exit code if successful. You can test the output of your commands
against your expected output using the `testing_diff_data` command (note other
commands are available to check various types of output - look in `testing/cmd`
for the full list). 

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

- add your data to the [test_data repo](https://github.com/MRtrix3/test_data);

- update the main MRtrix3 repo to reflect the new version of the data needed
  for the tests.

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
  folder. Remember to checkout `master` before committing:
  
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
test data.

The first thing to do is ensure your main MRtrix3 repo has already initialised
the `testing/data` submodule. The simplest way to do this is to just run
`./run_tests` (you can abort as soon as the 'fetching test data... OK' line has
completed). Alternatively, you can run the relevant command directly yourself:
```ShellSession
$ git submodule update --init
```

At this point, you can update the submodule with the latest version on the [test_data
repo](https://github.com/MRtrix3/test_data):
```ShellSession
$ git submodule update --remote
```
**Note:** this will record the SHA1 of the current `master` branch on the
[test_data repo](https://github.com/MRtrix3/test_data). See the next section
if you need to record any other branch or SHA1.

At this point, it's good practice to check your git status, and make sure
everything makes sense. For example, assuming I'd just added some test data,
and modified the tests for `mrconvert`:
```ShellSession
$ git status
On branch master
Your branch is up-to-date with 'github/master'.
Changes not staged for commit:
  (use "git add <file>..." to update what will be committed)
  (use "git checkout -- <file>..." to discard changes in working directory)

        modified:   testing/data (new commits)
        modified:   testing/tests/mrconvert

no changes added to commit (use "git add" and/or "git commit -a")
```

If everything looks in order, you can add these to the staging area:
```ShellSession
$ git add testing/data 
$ git add testing/tests/mrconvert
```
then commit and push:
```ShellSession
$ git commit
$ git push
```
At this point, you can check whether things are working correctly by running
the `./run_tests` scripts, and checking that the expected checksum shows up
afterwards when running:
```ShellSession
$ git submodule
 b6d0e018d0bad96edec423b81ef1b006a94932fb testing/data (b6d0e01)
```
It should show the expected version, _without_ a leading `+` sign.


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
the corresponding branch on the main MRtrix3 repo has also been merged. This
allows other developers to keep pushing additional data to `master` without
anything breaking.

> Technically, there is no reason to merge the branch on the [test_data
> repo](https://github.com/MRtrix3/test_data) to `master` since `run_tests`
> will pull in the relevant commit via its SHA1 checksum. However, merging to
> `master` ensures that the branch can be deleted without risk of losing the
> relevant commit.

To do this, developers need to push their changes to a different branch than
`master` on the [test_data repo](https://github.com/MRtrix3/test_data). This
can be done using for example:
```ShellSession
$ git clone git@github.com:MRtrix3/test_data.git
$ cd test_data
$ git checkout -b dwi2fod_bug_fix
# make changes to test data...
$ git commit -a
$ git push origin dwi2fod_bug_fix
```
At that point, the modifications should be on the `dwi2fod_bug_fix` branch
only. So far so good.

The next difficulty is updating the main MRtrix3 repo with that particular
branch. As mentioned above, `git submodule update --remote` will pull in the
current `master` branch, so we need to do something else. The solution is to
manually checkout the relevant branch. First, make sure the submodule has been
initialised (either run `./run_tests` or `git submodule update --init` as per
instructions above). Then:
```ShellSession
$ cd testing/data
$ git fetch
$ git checkout dwi2fod_bug_fix
```
At which point you can go back to the toplevel folder of the main repo, and
commit these changes as before:
```ShellSession
$ cd ../..
$ git add testing/data
$ git commit 
$ git push
```






