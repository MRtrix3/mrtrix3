Command-line parsing      {#command_line_parsing}
====================

Overview      {#command_line_overview}
========

Command-line parsing in MRtrix is based on a set of fairly generic conventions
to maximise consistency for end-users, and convenience for developers. A
command is expected to accept a certain number of arguments, and a certain
number of options. These are specified in the code using the MR::App::ARGUMENTS and
MR::App::OPTIONS vectors. They are also used to generate the help page for the
application, and it is therefore sensible to provide as much information in the
description fields as neccessary for end-users to understand how to use the
command.

Arguments and options are specified within the usage() function of each command
(see @ref command_howto for details).  Arguments are supplied as a vector of
MR::App::Argument objects, and by default each MR::App::Argument is expected to
have its value supplied on the command-line (although one argument can be
made optional, or allowed to be supplied multiple times).

Options are supplied as a vector of MR::App::Option objects, and by default are
optional (although they can be specified as 'required'). By default, only one
instance of each option is allowed, but this can also be changed.  Options may
also accept additional arguments, which should be supplied immediately after
the option itself.

Parsing of the command-line is done by first identifying any options supplied
and inserting them into the option list, along with their corresponding
arguments (if any). All remaining tokens are taken to be arguments, and
inserted into the MR::App::argument list. Checks are performed at this stage
to ensure the number of arguments and options supplied is consistent with that
specified in the usage() function.

The values of these arguments and options can be retrieved within the
application using the MR::App::argument list and the MR::App::option list.
Note that in practice, the MR::App::get_options() method is a much more
convenient way of querying the command-line options.


An overview of the basic contents of an application is given in @ref
command_howto. The description and syntax of the command should be fully
specified in the usage() function, and the various arguments and options can
then be retrieved and used in the application. Each of these aspects is
described below. 




Specifying the description and syntax     {#command_line_usage}
=====================================

The role of the usage() function is to populate the MR::App::DESCRIPTION,
MR::App::ARGUMENTS, and MR::App::OPTIONS vectors, and optionally the
MR::App::VERSION, MR::App::AUTHOR, and MR::App::COPYRIGHT entries if default
values are not suitable.  The example below illustrates a basic usage()
function: 

~~~{.cpp}
#include "app.h"

using namespace MR;
using namespace App;

void usage ()
{
  VERSION[0] = 1; // these are optional,
  VERSION[1] = 2; // and will default to the 
  VERSION[2] = 3; // MRtrix core version if ommitted

  // optional, defaults to J-Donald Tournier
  AUTHOR = "Joe Bloggs (j.bloggs@bogus.org)";

  // optional, defaults to the MRtrix GPL disclaimer
  COPYRIGHT = "Copyright 1967 The Institute of Bogus Science"; 

  DESCRIPTION 
  + "A brief description of the command";

  ARGUMENTS
  + Argument ("input", "the input image").type_image_in();

  OPTIONS
  + Option ("myopt",
      "my option to this command. Takes one additional float argument.")
  +   Argument ("value");
}
~~~


The DESCRIPTION     {#command_line_description}
===============

As shown in the example, description entries are added to the
MR::App::DESCRIPTION vector using the `+` operator, and consist of `const
char*` strings. Each separate string will be formatted into a distinct
paragraph on the help page.


The ARGUMENTS       {#command_line_arguments}
=============

As shown in the example, individual command-line arguments are added to the
MR::App::ARGUMENTS vector as MR::App::Argument objects using the `+` operator.
As a minimum, each Argument is constructed with its short-hand name (used in
the syntax line), and its description, which will appear in a separate listing
after the syntax line.

By default, an Argument is typed as a string. There are a number of other types
that can be used, and each type will perform its own checks on the values
provided by the user. For example:

~~~{.cpp}
ARGUMENTS 
+ Argument ("input", 
    "a description of the input argument")
    .type_integer (0, 8, 16);
~~~

The type specifiers that can be used are:

- [type_text()](@ref MR::App::Argument::type_text()): 
  the default.


- [type_image_in()](@ref MR::App::Argument::type_image_in()): 
  the argument corresponds to an already-existing image that
  will be used as an input. The argument value will be retrieved as a string
  corresponding to the image specifier.

- [type_image_out()](@ref MR::App::Argument::type_image_out()):
  argument corresponds to a non-existing image that will be produced by the
  program. The argument value will be retrieved as a string corresponding to
  the image specifier.

- [type_integer()](@ref MR::App::Argument::type_integer()):
  the argument corresponds to an integer value in the range [minimum, maximum],
  with default value as specified. The argument value will be retrieved as an
  int. If (minimum, default, maximum) are ommitted, default values will be
  used.

- [type_float(minimum, default, maximum)](@ref MR::App::Argument::type_float()):
  the argument corresponds to a floating-point value in the range [minimum,
  maximum], with default value as specified. The argument value will be retrieved
  as a float or double. If (minimum, default, maximum) are ommitted, default
  values will be used.

- [type_choice()](@ref MR::App::Argument::type_choice()) (list):
  the argument corresponds to one of a set number of available choices,
  supplied as a NULL-terminated array of strings. The argument value can be
  retrieved as a string or as an int, corresponding to the appropriate index into
  the list of choices.

- [type_sequence_int()](@ref MR::App::Argument::type_sequence_int()):
  the argument corresponds to a sequence of integers. The argument value will
  be retrieved as a `std::vector<int>`.

- [type_sequence_float()](@ref MR::App::Argument::type_sequence_float()):
  the argument corresponds to a sequence of floating-point values. The argument
  value will be retrieved as a `std::vector<float>`.

In certain cases, it is possible that none of these types are suitable. In
these cases, the default type ([type_text()](@ref
MR::App::Argument::type_text())) should be used, and any custom casting and
checking will need to be performed explicitly by the programmer.

It is possible to specify at most one argument as being optional, by adding
the [optional()](@ref MR::App::Argument::optional()) flag:

~~~{.cpp}
ARGUMENTS 
+  Argument ("input", 
    "a description of the input argument")
    .optional();
~~~

Finally, at most one argument can be specified as repeatable (in other words, a
number of these arguments can be supplied one after the other).  This is useful
for example if the command can operate on multiple data sets sequentially:

~~~{.cpp}
ARGUMENTS
+ Argument ("input", 
    "a description of the input argument")
    .allow_multiple();
~~~


The OPTIONS      {#command_line_options}
===========

Individual command-line options are added to the OPTIONS vector as MR::App::Option
objects using the `+` operator.  As a minimum, each Option is constructed with
its short-hand name (used on the command-line), and its description:

~~~{.cpp}
OPTIONS
+  Option ("option", "a description of the option");
~~~

Similarly to arguments, options can be specified as [repeatable](@ref MR::App::Option::allow_multiple()):

~~~{.cpp}
OPTIONS
+ Option ("option", "a description of the option").allow_multiple();
~~~

Options can also be specified as [required](@ref MR::App::Option::required()) (by default, options are optional):

~~~{.cpp}
OPTIONS
+ Option ("option", "a description of the option").required();
~~~

To handle additional arguments to an MR::App::Option, arguments can be added to the
option as MR::App::Argument objects, again using the `+` operator. Note that in
this case the description field of the argument is ignored, and so does not
need to be specified: These Arguments can be typed in the same way as regular
MR::App::Arguments. For example:

~~~{.cpp}
OPTIONS 
+ Option ("option", "a description of the option")
    + Argument ("value").type_integer ()
    + Argument ("list").type_sequence_int ();
~~~


Defining option groups     {#command_line_option_group}
----------------------

Option objects are actually inserted into MR::App::OptionGroup objects; in the
example above, new Options were added to the default MR::App::OptionGroup. It
is possible to create new option groups that will appear under their own
heading in the help page.  

~~~{.cpp}
OPTIONS
+   Option ("normal", "a 'standard' option")

+ OptionGroup ("My options")

+   Option ("check", 
    "this option will now appear in the 'My options' section);
~~~

This makes it possible to define OptionGroup objects for commonly-used
functions elsewhere in the code, and simply add them into the application when
required. For example: 

~~~{.cpp}
// funny_processing.h:

extern const OptionGroup funny_options;
~~~

~~~{.cpp}
// funny_processing.cpp:

const OptionGroup funny_options ("Funny options")
+ Option ("joke", "a option that is now part of the funny options)
+   Argument ("name")

+ Option ("prank", "another option");
~~~


~~~{.cpp}
// my_application.cpp:

void usage () 
{ 
  ...

  OPTIONS
  + Option ("normal", "a 'standard' option")

  + funny_options; 
}
~~~

This is particularly useful since these options can then be handled by the
relevant functions or classes, without the developer of the application
needing to worry about them in any way. Following on from the example above,
the 'funny_processing' code might declare a function 'do_funny_processing()',
which could then retrieve any relevant parameters supplied by the user on the
command-line, with no interaction with the body of the application using that
function (other than adding the relevant OptionGroup to the OPTIONS list): 

~~~{.cpp}
// funny_processing.cpp:

void do_funny_processing () 
{
  App::Options opt = App::get_options ("joke");
  if (opt.size()) {
    // do something else...
  }

  ...
}
~~~

Breaking up an option group into multiple lists      {#command_line_option_group_broken_up}
===============================================

In some cases, it might be useful to break up an MR::App::OptionGroup into
several distinct sub-groups, even though they would all conceptually belong to
the same section. For instance, some options might only be required when
processing input data, others when producing output data. Some applications
might only need to process data without producing any, others might only
produce data, and others might need to do both. To support this, developers can
provide several OptionGroups with identical sections names; these will then be
displayed together in the help page if used together in the corresponding
application. 

For example:

~~~{.cpp}
// funny_processing.h:

extern const OptionGroup funny_options_in;
extern const OptionGroup funny_options_out;
~~~

~~~{.cpp}
// funny_processing.cpp:

const OptionGroup funny_options_in ("Funny options")
+ Option ("funny_in", "specify funny input data")
+   Argument ("name");

// Note that the group heading is the same in both cases:
const OptionGroup funny_options_out ("Funny options")
+ Option ("funny_out", "specify funny output data")
+   Argument ("name");
~~~

~~~{.cpp}
// my_application.cpp:
   
void usage () 
{ 
  ...

  OPTIONS
  + Option ("normal", "a 'standard' option")

  // these will both now appear under the same heading "Funny options" on the
  // help page, with the options themselves appearing in the order listed here:
  + funny_options_in 
  + funny_options_out;
}
~~~



Retrieving command-line argument and option values    {#command_line_retrieve}
==================================================

Argument and option values can be retrieved at any point within the
application.  Arguments are provided via the MR::App::argument vector, which is
a `std::vector<App::ParsedArgument>`. Their values can be retrieved using the
subscript operator, indexed by their position on the command-line.  For
convenience, values can be obtained directly by implicit type-casting (i.e.
simply by using the argument in a context that requires the relevant type -
e.g. assignment).  The following examples should clarify how this is done:

~~~{.cpp}
void run () {
  size_t numarg = argument.size(); // the number of arguments

  // retrieving values as a string:
  auto header = Header::open (argument[0]);

  // retrieving value by implicit type-casting:
  // Note that this argument is expected to have been specified using type_integer().
  // Bound checks are performed at this stage.
  int value = argument[1];

  // Another example of retrieval via implicit type-casting:
  // Note that in this case, the argument is expected to have been specified using type_sequence_int()
  std::vector<int> list = argument[2];

  ...
}
~~~

Option values are most easily retrieved using the MR::App::get_options()
convenience function. This returns a MR::App::Options object that is to all
intents and purposes identical in use to a `std::vector<std::vector<App::ParsedArgument>>`: 
one vector of values for each matching option specified on the command-line (an
option may be specified multiple times if it has been specified using
allow_multiple()).  The values are stored in the same way as the parsed
arguments, and their values can therefore also be retrieved using implicit
type-casting (see above). The example below should clarify this:

~~~{.cpp}
void run () {
  Options opt = get_options ("option");

  if (opt.size()) { // check whether any such options have been supplied
    // retrieve first argument of first option as an int:
    int value = opt[0][0];

    // retrieve second argument of first option as a sequence of ints:
    std::vector<int> list = opt[0][1];
  }

  // in cases where the option has been specified using allow_multiple(),
  // opt.size() will return the number of such options.
  // In this example, the first argument of each additional option is added
  // to the list of values by implicit type-casting to a float:
  opt = get_options ("multiple_option");
  std::vector<float> values;
  for (size_t n = 0; n < opt.size(); ++n)
    values.push_back (opt[n][0]);

  ...
}
~~~

A common use-case is to set a variable from the command-line, defaulting
to a set value otherwise. This can be done using the
MR::App::get_option_value() convenience call. For example:

~~~{.cpp}
void run() {
  // param will be set to 0.0 unless the otherwise stated via the 'parameter' option:
  const float param = get_option_value ("parameter", 0.0);

  ...
}
~~~

If for any reason the get_options() convenience function is not suitable,
options can be accessed directly using the MR::App::option variable, which is
a `std::vector` of MR::App::ParsedOption objects, one per option in the same
order as on the command-line. Note that in this case, option values will only
be available as `const char*` string arrays, and no bounds checking will be
performed. For further details, refer to the MR::App::ParsedOption description
page.






