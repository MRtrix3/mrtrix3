Advanced debugging
===================

On rare occasions, a user may encounter a critical error (e.g.
"Segmentation fault") within an MRtrix command that does not give
sufficient information to identify the cause of the problem, and that
the developers are unable to reproduce. In these cases, we will often
ask to be provided with example data that can consistently reproduce the
problem in order to localise the issue. An alternative is for the user
to perform an initial debugging experiment, and provide us with the
resulting information. The instructions for doing so are below.

1. If required, install ``gdb``; the GNU Debugging Tool (specific
   instructions for this installation will depend on the operating
   system)

2. *Make sure* you are using the most up-to-date MRtrix code!
   (``git pull``)

3. Configure and compile MRtrix in debug mode:
   ``./configure -debug debug; ./build debug`` Note that this
   compilation will reside *alongside* your existing MRtrix
   installation, but will not interfere with it in any way. Commands
   that are compiled in debug mode will have the ``__debug`` suffix
   added.

4. Execute the problematic command within ``gdb``:
   ``gdb --args command__debug (arguments) (-options)`` From the command
   that is causing a problem, add ``gdb --args`` to the beginning of the
   line, and add ``__debug`` to the name of the MRtrix command. Include
   all of the file paths, options etc. that you used previously when the
   problem occurred.

5. Type ``r`` and hit ENTER to run the command.

6. If an error is encountered, gdb will print an error, and then provide
   a terminal with ``(gdb)`` shown on the left hand side. Type ``bt``
   and hit ENTER: This stands for 'backtrace', and will print details on
   the internal code that was running when the problem occurred.

7. Copy all of the raw text, from the command you ran in instruction 3
   all the way down to the bottom of the backtrace details, and send it
   to us. The best place for these kind of reports is to make a new
   issue in the `Issues <https://github.com/MRtrix3/mrtrix3/issues>`__
   tracker for the GitHub repository.

We greatly appreciate any contribution that the community can make
toward making MRtrix3 as robust as possible, so please don't hesitate to
report any issues you encounter.
