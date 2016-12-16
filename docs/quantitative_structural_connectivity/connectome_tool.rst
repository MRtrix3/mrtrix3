Using the connectome visualisation tool
==========================

The connectome tool bar in *MRtrix3* has been designed from scratch, with
the intention of providing a simple, data-driven mechanism for visually
assessing individual connectomes as well as the results of network-based
group statistics. The interface may therefore vary considerably from
other connectome visualisation packages, and may be intimidating for new
users who simply want to 'see the connectome'. I hope I can convince you
in this tutorial that the design of this tool allows you, the user, to
dictate exactly *how* you want to visualise the connectome, rather than
being forced to conform to a particular prior expectation of how such
things should be visualised.

Initialising the tool
---------------------

My suspicion is that new users will load the tool, and immediately
think: 'Where do I load my connectome?'. Well, let's take a step
backwards. If you were to give the software a connectome matrix, with no
other data, there would be no way to visualise that connectome in the
space of an MR image: the software has no information about the spatial
locations of the nodes upon which that connectome is based. So the first
step is actually to load an image to provide the tool with this
information, using the "Node image" button at the top of the toolbar.
The desired image is the output of the ``labelconvert`` command, as
detailed in the :ref:`stuct_connectome_construction` guide: the
tool uses this image to localise each parcel in 3D space in preparation
for visualisation. Alternatively, you can load the relevant parcellation
image from the command-line when you first run ``mrview``, using the
``-connectome.init`` option.

.. ATTENTION::
If you still do not see anything in the ``mrview`` main window, this is
likely because you have not yet opened a primary image in ``mrview``. This
is currently necessary for ``mrview`` to correctly set up the camera
positioning. The easiest solution is to open your parcellation image not
only to initialise the connectome tool, but also as a standard image in
``mrview``; then simply *hide* the main image using the 'View' menu.

With the basis parcellation image loaded, the tool will display the
location of each node; note however that all of the nodes are exactly
the same colour, and exactly the same size, and there are no connections
shown between them - it's an entirely dis-connected network. This makes
sense - we haven't actually provided the tool with any information
regarding which connections are present and which are absent. We can
also do the opposite: change the "Edge visualisation" - "Visibility"
from 'None' to 'All', and now the software shows every edge in the
connectome non-discriminantly.

Therefore, we need some mechanism of informing the software of which
edges should be drawn, and which should not. Most logically, this could
be achieved by loading a structural connectome, and perhaps applying
some threshold. So now, for the "Edge visualisation" - "Visibility"
option, select "Matrix file", and load your connectome. The software now
uses the data from this external file to threshold which edges are drawn
and which are not, and also allows you to vary that threshold
interactively. (You can also load a connectome matrix from the command
line using the ``-connectome.load`` option.)

The connectome still however has a binary appearance; every edge in the
connectome is either present or absent, and they all have the same size
and the same colour. We know that our connectome contains weights
distributed over a wide scale, and would like to be able to see this as
part of our visualisation; for instance, we may decide that more dense
connections should have a 'hot' colour appearance, whereas less dense
connections should be darker. We can achieve this by changing the "Edge
visualisation" - "Colour" from 'Fixed' to 'Matrix file', and selecting
an appropriate matrix file (perhaps the same file as was used for the
visibility threshold, perhaps not).

For most users, connectome data will be loaded using the
'open' button in the 'connectome matrices' section, or at the command-line
when ``mrview`` is first run using the ``-connectome.load`` option.

Basis of connectome visualisation customisation
-----------------------------------------------

With the above steps completed, you should obtain a fairly rduimentary
visualisation of the connectome you have loaded. The plethora of
buttons and gadgets in the connectome tool user interface is however
a clue regarding the scope of customisation available for precisely
how the connectome data will be displayed.

As an example, consider the 'Edge visualisation - Colour' entry. These
options control how the colour of each individual edge in the connectome
will be determined, based on the data the tool is provided with. Clicking
on the main combo box shows that there are a few options available:

* *Fixed*: Use the same fixed colour to display all visible edges.

* *By direction*: The XYZ spatial offset between the two nodes connected by
an edge is used to derive an RGB colour (much like the default streamlines
colouring).

* *Connectome*: The colour of each edge will depend on the value for that
edge in the connectome you have loaded, based on some form of
value -> colour mapping (a 'colour map').

* *Matrix file*: Operates similarly to the *connectome* option; except that
the value for each edge is drawn from a matrix file that is *not* the
connectome matrix you have loaded (though it must be based on the same
parcellation to have any meaning). So for instance: You could load a
structural connectome file as your connectome matrix and show only those
edges where the connection density is above a certain threshold, but then
set the *colour* of each edge based on a *different matrix file* that
contains functional connectivity values.

If the *Connectome* or *Matrix file* options are used, it is also possible
to alter the colour map used, and modify the values at which the edges will
reach the colours at either extreme of the colour map.

Hopefully, this simple demonstration will be enough to highlight the
design principle of this tool, and therefore the frame of mind necessary
to use it effectively:

**What *data* do I want to determine a specific *visual property* of my
connectome?**

There is tremendous power in separating these two entities. For
instance, consider a use case where I have performed network-based group
statistics, and wish to visualise my result. I may choose to threshold
the connectome edges based on statistical significance, but set the
width of the connections based on the mean connection strength to get an
idea of the density of connections in the detected network, but set the
colour of each edge based on the effect size to see which components of
the network are most affected. I can even automatically hide any nodes
that are not involved in the detected network by selecting "Node
visualisation" - "Visibility" - 'Degree >= 1'.

Importing detailed node information
-----------------------------------

When the parcellation image is first loaded, the software has no
information regarding the designations of the underlying nodes, so it
simply labels them as "Node 1", "Node 2" etc.. To show the anatomical
name of each node in the list, you must load the connectome
lookup table that was used as the target output in the ``labelconvert``
step during [structural connectome construction]. This file provides a
list of node indices and their corresponding names, so is perfect for
subsequent assessment of the resulting connectomes, whether using this
tool or in other contexts (e.g. Matlab). Such a lookup table may also
include a pre-defined colour for each node, which can then be used
during visualisation by selecting "Node Visualisation -> Colour -> LUT".

Advanced visualisation
----------------------

There are a couple of neat tricks that can be used to produce
impressive-looking visualisations, but need some pre-processing or
careful consideration in order to achieve them.

Visualising edges as streamlines / streamtubes
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Rather than drawing a straight line between connected nodes to represent
an edge, it is possible with tractography-based connectome construction
to instead represent each connection based on the structural trajectory
by which those nodes are inter-connected. This can be achieved as
follows:

-  When generating the connectome using :ref:`tck2connectome`, use the
   ``-assignments`` option. This will produce a text file where each
   line contains the indices of the two nodes to which that particular
   streamline was assigned.

-  Use the :ref:`connectome2tck` command to produce a single track file,
   where every streamline represents the mean, or *exemplar*, trajectory
   between two nodes. This is achieved using two command-line options:
   ``-exemplars`` to instruct the command to generate the exemplar
   trajectory for each edge, rather than keeping all streamlines (you
   will need to provide your parcellation image); and ``-files single``
   to instruct the command to place all computed exemplars into a single
   output file.

-  In the ``mrview`` connectome toolbar, select "Edge visualisation" -
   "Geometry" - 'Streamlines / Streamtubes', and select the exemplar
   track file just generated.

Visualising nodes as triangulated meshes
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Although the node parcellations are represented as volumetric
segmentations, and we do not yet have support for importing mesh-based
parcellations, it is still possible to visualise the conectome nodes
using a mesh-based representation. This is done by explicitly converting
the volume of each parcel to a triangulated mesh. The process is as
follows:

-  Compute a triangular mesh for each node, and store the results in a
   single file. The command is called :ref:`label2mesh`. Note that the
   output file *must* be in the ``.obj`` file format: this is the only
   format currently supported that is capable of storing multiple mesh
   objects in a single file.

-  (Optional) Smooth the meshes to make them more aesthetically pleasing
   (the results of the conversion process used in ``label2mesh`` appear
   very 'blocky'). Apply the :ref:`meshfilter` command, using the
   ``smooth`` operator. Again, the output must be in the ``.obj``
   format.

-  In the ``mrview`` connectome toolbar, select "Node visualisation" -
   "Geometry" - 'Mesh', and select the mesh file just generated.

Using node selection to highlight features of interest
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The table in the connectome toolbar that lists the node names and
colours can also be used to select and highlight particular nodes. In
most cases, this will simply be an additional 'toy' for navigating the
data; however it's also possible that this capability will prove to be a
powerful tool for demonstrating network features.

In any connectome visualisation software, when the user selects one or
more particular nodes of interest, some modification must be applied to
the visual features of the nodes in order to 'highlight' the nodes of
interest. In many cases, this may be hard-wired to behave in a
particular way. In the case of ``mrview`` in *MRtrix3*, this highlighting
mechanism is entirely flexible: the user can control the visual modifications
applied to both those network elements selected and those not selected. For
instance, you may choose for nodes to become completely opaque when you
select them, while other un-selected nodes remain transparent; or they
may grow in size with respect to the rest of the connectome; or they may
change in colour to highlight them; or those nodes not selected may
disappear entirely. This flexibility is accessed via the "Selection
visualisation settings" button, which will open a dialog window
providing access to these settings.

As manual selection applies to nodes only, the behaviour for edges is as
follows:

-  When no nodes are selected, all edges are drawn according to their
   standard settings.

-  If a single node is selected, all edges emanating from that node are
   considered to be 'selected', and the relevant visual modifiers will
   be applied.

-  If two or more nodes are selected, only connections exclusively
   connecting between the nodes of interest are considered to be
   'selected'.

Node visualisation using matrices
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

When using external data files to control the visual properties of the
connectome, most commonly *vector files* will be used to determine
visual properties of nodes, and *matrix files* will be used to determine
visual properties of edges. These provide precisely one scalar value per
connectome element, and therefore provide a static visual configuration.

It is however also possible to set any visual property of the connectome
nodes based on a *matrix file*. In this scenario, the values to be drawn
from the matrix - and hence their influence on the relevant visual
property of the nodes - depends on the *current node selection*. That
is: once you select a node of interest, the software extracts the
relevant row from the matrix, and uses only that row to influence the
node visual property to which it has been assigned. In the case where
multiple nodes of interest are selected, an additional drop-down menu is
provided, that allows you to prescribe how those multiple rows of matrix
data are combined in order to produce a single scalar value per node,
which can then be used to influence its relevant visual property.

