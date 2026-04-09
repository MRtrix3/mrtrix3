Performing post hoc testing following statistical inference
-----------------------------------------------------------

From version ``3.1.0``, additional functionality is provided for performing *post hoc* analysis in a statistically rigorous manner.
This is where the process of statistical inference and interpretation is explicitly split into two stages:

1.  An omnibus F-test is first evaluated,
    with the null hypothesis being accepted or rejected in each element based on a pre-specified *p*-value threshold.

2.  To discover which constituent parts of the omnibus F-test contributed to rejection of the null hypothesis,
    those constituent parts are tested individually,
    in a way that remains faithful to the false positive guarantees of the original test.

This process is demonstrated here using an example.
There are three groups, referred to as *A*, *B* and *C*.
One wishes to test whether there is any difference between any of these groups;
this is equivalent to rejecting the null hypothesis that all three groups are equivalent.
However upon observation of a statistically significant difference,
one is also interested to know which group difference(s) it was that led to that result,
both in terms of the groups involved and the direction of the effect.

A possible design matrix for the experimental model
(assuming there are no other experimental factors of interest)
is as follows:

+------+---------+---------+---------+
|      | Group A | Group B | Group C |
+======+=========+=========+=========+
| A    |    1    |    0    |    0    |
+------+---------+---------+---------+
| A_2  |    1    |    0    |    0    |
+------+---------+---------+---------+
| ...  |   ...   |   ...   |   ...   |
+------+---------+---------+---------+
| A_NA |    1    |    0    |    0    |
+------+---------+---------+---------+
| B_1  |    0    |    1    |    0    |
+------+---------+---------+---------+
| B_2  |    0    |    1    |    0    |
+------+---------+---------+---------+
| ...  |   ...   |   ...   |   ...   |
+------+---------+---------+---------+
| B_NB |    0    |    1    |    0    |
+------+---------+---------+---------+
| C_1  |    0    |    0    |    1    |
+------+---------+---------+---------+
| C_2  |    0    |    0    |    1    |
+------+---------+---------+---------+
| ...  |   ...   |   ...   |   ...   |
+------+---------+---------+---------+
| C_NC |    0    |    0    |    1    |
+------+---------+---------+---------+


1.  Omnibus test

    To perform the omnibus test,
    *no* t-tests are performed using the :code:`-ttests` option;
    instead, a single F-test is specified via the :code:`-ftest` option,
    providing a file that contains the following::

        1 -1  0
        1  0 -1
        0  1 -1

    Further, we assume that some form of analysis mask has been calculated
    (whether a template voxel mask, a template fixel mask, or other),
    and that this is provided to the statistical inference command via the :code:`-mask` option.

2.  Calculation of *post hoc* calculation mask

    One of the results derived step 1 is a file that encodes,
    for each element within the analysis mask,
    the family-wise error (FWE) corrected *p*-value.
    Typically, during manual interrogation of such results,
    one would manipulate the visualisation such that only elements with *p* \< 0.05 are shown.
    For this process, one must instead explicitly compute a binary mask that consists of those elements.
    This can be achieved using ``mrthreshold`` with the ``-abs 0.95 -comparison gt`` options
    (since statistical inference commands actually export data corresponding to the *complement* of the FWE *p*-value,
    or ``1.0 - p``;
    further, it is required to omit elements for which ``p = 0.05``,
    and therefore only select elements for which the computed value is greater than *but not equal to* 0.95).

3.  Perform *post hoc* analysis

    This step consists of multiple components:

    1.  Utilisation of t-tests of interest

        Since the purpose of the *post hoc* analysis is to discover which group differences contributed to the statistically significant omnibus test,
        one must provide those tests explicitly.
        In the example used here,
        those correspond to signed differences between pairs of groups.
        The following contents would therefore be provided as a file as input to the ``-ttests`` option::

             1 -1  0
            -1  1  0
             1  0 -1
            -1  0  1
             0  1 -1
             0 -1  1

        This will perform six one-tailed *t*-tests.

    2.  Utilisation of post hoc inference mask

        The statistical inference command is run a second time,
        omitting the omnibus F-test that was specified in the first instance,
        and instead providing the set of *t*-tests as above.
        In addition to this,
        *both* the :code:`-mask` and :code:`-posthoc` command-line options are used:
        the input to the :code:`-mask` option is the same analysis mask as that used in the omnibus test,
        whereas the input to the :code:`-posthoc` option is the binary mask of statistically significant elements derived in step 2.
        The subtleties of the difference between these two options is described in greater detail in :ref:`difference_between_mask_and_posthoc` below.

    3.  Use of strong familywise error control

        Using the :code:`-strong` command-line option to enfoce *strong* FWE control across the set of *post hoc* tests
        (six in this example)
        results in a single non-parametric null distribution being computed across the complete set of *post hoc* tests being performed.
        This is required to ensure that the false positive control imposed upon the results of the *post hoc* tests is not weaker
        (and therefore more permissive)
        than that of the original omnibus test.

The sum total effect of this set of components is that the set of elements deemed statistically significant across the set of *post hoc* tests
(using the same *p*-value threshold as was pre-determined for the omnibis test)
can be interpreted under the same false positive control guarantees as that of the original omnibus F-test;
being, in this instance, the null hypothesis that all groups are equivalent at $\alpha$ < 0.05.

.. _difference_between_mask_and_posthoc:

Difference between :code:`-mask` and :code:`-posthoc`
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

While on initial confontation it may seem counter-intuitive to be providing two different binary masks to a statistical inference command,
there is a difference in the way the inputs to these two command-line options are utilised,
and this is important to the validity of the *post hoc* testing process.

If a particular element (eg. fixel, voxel) is *present* in the data specified via the :code:`-mask` option,
but is *absent* in the data specified via the :code:`-posthoc` option,
then that element:

-   *Will* have a test statistic computed per hypothesis;
-   *Will* contribute to statistical enhancement;
-   Will *not* be eligible to contribute to the non-parametric null distribution,
    even if its enhanced test statistic is the maximum of all elements within the mask;
-   Will *not* have a *p*-value computed per hypothesis.

By constructing this discrepancy between *statistical enhancement* and *statistical inference*,
the manifested empirical behaviour of whatever statistical enhancement algorithm is in use
(eg. Connectivity-based Fixel Enhancement (CFE), Threshold-Free Cluster Enhancement (TFCE))
is entirely consistent between the initial omnibus F-test and the subsequent post hoc tests.
If, hypothetically, one were to perform *post hoc* analysis following an omnibus test by provding the set of *p* \< 0.05 elements via the :code:`-mask` option,
then the behaviour of statistical enhancement would be quite different between the omnibus and *post hoc* tests,
violating the validity of the latter.

Note that it is not strictly guaranteed that all elements that reached statistical significance in an omnibus F-test will exhibit statistical significance in at least one *post hoc* test.
The purpose of this process is rather to ensure that any elements that do reach statistical significance in a *post hoc* test can be scientifically reported with the same false positive guarantees as the original omnibus test.
