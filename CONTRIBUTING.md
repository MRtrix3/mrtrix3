# Contributing to *MRtrix3*

There are many ways in which to contribute to the ongoing improvement of *MRtrix3*:

## Bug reporting

1. Please search on both the [*MRtrix3* community forum](http://community.mrtrix.org/search)
   and the [GitHub issue list](https://github.com/MRtrix3/mrtrix3/issues)
   to see if anybody else has lodged a similar observation.

1. How confident are you that the behaviour you have observed is in fact a
   genuine bug, and not a misunderstanding?

   -  *Confident*: Please [open a new GitHub issue](https://github.com/MRtrix3/mrtrix3/issues/new);
      select the "bug report" issue template to get started.

   -  *Not so confident*: That's fine! Consider instead creating a new topic
      on the [*MRtrix3* community forum](http://community.mrtrix.org/);
      others can then comment on your observation and determine the
      appropriate level of escalation.

## Requesting a new feature

Please search the [GitHub issue list](https://github.com/MRtrix3/mrtrix3/issues)
to see if anybody else has made a comparable request:

   -  If a corresponding issue already exists, please add a comment to that
      issue to escalate the request. Additionally, describe any
      aspect of that feature not yet described in the existing issue.

   -  If no such listing exists, then you are welcome to create a [new
      issue](https://github.com/MRtrix3/mrtrix3/issues/new) outlining the
      request. Be sure to select the "feature request" option to get started
      with writing the Issue.
      
## Asking questions

General questions regarding *MRtrix3* installation, usage, or any other
aspect that is not specific to the *MRtrix3* *code*, should be directed to
the [community forum](http://community.mrtrix.org/). Also consider perusing
the [online documentation](https://mrtrix.readthedocs.io/en/latest/) for
the software, in case your issue has already been described there.

## Making direct contributions

Thanks for your interest in making direct contributions to *MRtrix3*!
We are excited to expand the breadth of researchers involved in improving
and expanding this software, and to ensure that all who make such
contributions receive appropriate acknowledgement through Git.

The instructions below give an overview of how to go about generating a
proposed change to *MRtrix3*. The *MRtrix3* developers are however more
than willing to be engaged in the process for those without experience
in Git or GitHub.

1. You will need to create a *fork* of the [*MRtrix3* repository](https://github.com/MRtrix3/mrtrix3)
   into your GitHub account, where unlike the main *MRtrix3* repository,
   you will have full write access to make the requisite changes.

1. Create a Git branch that is named appropriately according to the
   modifications that are being made. The existing code branch on which
   this new derived branch should be based depends on the nature of the
   proposed change (described later below).

1. Generate one or more Git commits that apply your proposed changes to
   the repository:

   -  Individual commits should ideally have a clear singular purpose,
      and not incorporate multiple unrelated changes. If your proposed
      changes involve multiple disparate components, consider breaking
      those changes up into individual commits.

      Conversely, if multiple code changes are logically grouped with /
      linked to one another, these should ideally be integrated into a
      single commit.

   -  Commits should contain an appropriate message that adequately
      describes the change encapsulated within.

      If the change demands a longer description, then the commit message
      should be broken into a synopsis (less than 80 characters) and message
      body, separated by two newline characters (as this enables GitHub to
      parse them appropriately).

      This can be achieved at the command-line as follows:

      `$ git commit -m $'Commit synopsis\n\nHere is a much longer and wordier description of my proposed changes that doesn\'t fit into 80 characters.\nI can even spread the message body across multiple lines.'`

      (Note also the escape character "`\`" necessary for including an
      apostrophe in the message text)

   -  Where relevant, commit messages can also contain references to
      GitHub issues or pull requests (type the "`#`" character followed
      by the issue / PR number), and/or other individual commits (copy
      and paste the first 8-10 characters of the commit hash).

   -  If multiple persons have contributed to the proposed changes, it is
      possible to modify individual Git commits to have [multiple
      authors](https://help.github.com/en/articles/creating-a-commit-with-multiple-authors),
      to ensure that all contributors receive appropriate acknowledgement.

   As a general rule: Git commits and commit messages should be constructed
   in such a way that, at some time in the future, when one is navigating
   through the contribution history, the evolution of the code is as clear
   as possible.

1. Identify the appropriate classification of the change that you propose
   to make, and read the relevant instructions there:

   -  "[**Fixing a bug**](#fixing-a-bug)": If the current code behaviour is
      *egregiously incorrect*.

   -  "[**Adding or altering features**](#adding-or-altering-features)": If:

      -  The current code behaviour is considered incorrect or non-ideal,
         but is nevertheless *functional*;

      -  The proposed change would *alter* the output data generated by
         a command;

      -  The proposed change improves the *performance* of a particular
         command or process, but does not change its output.

   -  "[**Documentation**](#documentation)": If you wish to make changes to
      the [*MRtrix3 documentation*](https://mrtrix.readthedocs.io/en/latest/).

1. Check that your modified code does not prevent *MRtrix3* from
   passing existing tests (all files are in the *MRtrix3* root directory):

   1.  If adding or modifying C++ code, make sure that script "`check_syntax`"
       executes successfully.

   1.  If adding or modifying Python code, make sure that script
       "`run_pylint`" executes successfully.

   1.  If there is a chance of your modifications altering the observable
       behaviour of one or more existing commands, make sure that script
       "`run_tests`" executes successfully.

1. For code contributions, if possible, a unit test or reproducibility
   test should be added. This not only demonstrates the behaviour of the
   proposed code, but will also preclude future regression of the behaviour
   of that code. For tests that require the addition of raw data to the
   [`test_data` repository](https://github.com/MRtrix3/test_data), please
   coordinate with one of the *MRtrix3* developers.

1. Once completed, a Pull Request should be generated that merges the
   corresponding branch in your forked version of the *MRtrix3* repository
   into the appropriate target branch of the main *MRtrix3* repository
   itself:

   -  If your intended changes are complete, and you consider them ready
      to be reviewed by an *MRtrix3* team member and merged imminently,
      then create a standard Pull Request.

   -  If your changes are ongoing, and you are seeking feedback from the
      *MRtrix3* developers before completing them, then create a
      [draft Pull Request](https://github.blog/2019-02-14-introducing-draft-pull-requests/).

#### Fixing a bug

1. If there does not already exist a [GitHub Issue](https://github.com/MRtrix3/mrtrix3/issues)
   describing the bug, consider reporting the bug as a standalone Issue
   prior to progressing further; that way developers can confirm the issue,
   and possibly provide guidance if you intend to resolve the issue yourself.
   Later, the Pull Request incorporating the necessary changes should then
   reference the listed Issue (simply add somewhere in the description
   section the "`#`" character followed by the issue number).

1. Bug fixes are merged directly to `master`; as such, modifications to the
   code should be made in a branch that is derived from `master`, and the
   corresponding Pull Request should select `master` as the target branch
   for code merging.

1. A unit test or reproducibility test should ideally be added: such a
   test should fail when executed using the current `master` code, but pass
   when executed with the proposed changes.

#### Adding or altering features

1. New features, as well as any code changes that alter the output data
   produced by a command, are merged to the `dev` branch, which contains
   all resolved changes since the most recent tag update. As such, any
   proposed changes that fall under this classification should be made
   in a branch that is based off of the `dev` branch, and the corresponding
   Pull Request should select `dev` as the target branch for code merging.

#### Coding conventions

While we do not have strict enforced coding conventions in *MRtrix3*, the
accepted conventions should be self-evident from the code itself.

A few explicit notes on such:

1. For both C++ and Python, indentation is achieved using two space
   characters.

1. Newline characters are Unix-style ("`LF`" / '`\n`'); any changes that
   introduce Windows-style newline characters ("`CR LF`" / "`\r\n`")
   will need to be edited accordingly.

1. In Python, variable / class / module names are enforced through
   `pylint`. Script "`run_pylint`" in the *MRtrix3* root directory
   will test any code modifications against these expectations.

1. If the operation of your code is not trivially self-apparent,
   please endeavour to comment appropriately.

1. Do not leave excess newline characters at the end of a file.

#### Documentation

For proposed documentation changes, please communicate with the *MRtrix3*
developers by creating a [new GitHub issue](https://github.com/MRtrix3/mrtrix3/issues/new).
The appropriate course of action will depend on the component of the
documentation intended to be changed as well as the nature of the change.
In addition, the location of much of the *MRtrix3* documentation will be
changing imminently (Pull Request #1622), and so any proposed documentation
content changes would need to be coordinated with such.
