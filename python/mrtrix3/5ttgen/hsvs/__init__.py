# Copyright (c) 2008-2023 the MRtrix3 contributors.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# Covered Software is provided under this License on an "as is"
# basis, without warranty of any kind, either expressed, implied, or
# statutory, including, without limitation, warranties that the
# Covered Software is free of defects, merchantable, fit for a
# particular purpose or non-infringing.
# See the Mozilla Public License v. 2.0 for more details.
#
# For more details, see http://www.mrtrix.org/.


HIPPOCAMPI_CHOICES = [ 'subfields', 'first', 'aseg' ]
THALAMI_CHOICES = [ 'nuclei', 'first', 'aseg' ]

# Have not had success segmenting the posterior commissure
# FAST just doesn't run well there, at least with a severely reduced window of data
# Though it may actually get envelloped within the brain stem, in which case it's not as much of a concern
ATTEMPT_PC = False



ASEG_STRUCTURES = [ ( 5,  4, 'Left-Inf-Lat-Vent'),
                    (14,  4, '3rd-Ventricle'),
                    (15,  4, '4th-Ventricle'),
                    (24,  4, 'CSF'),
                    (25,  5, 'Left-Lesion'),
                    (30,  4, 'Left-vessel'),
                    (44,  4, 'Right-Inf-Lat-Vent'),
                    (57,  5, 'Right-Lesion'),
                    (62,  4, 'Right-vessel'),
                    (72,  4, '5th-Ventricle'),
                    (250, 3, 'Fornix') ]


HIPP_ASEG = [ (17,  2, 'Left-Hippocampus'),
              (53,  2, 'Right-Hippocampus') ]

AMYG_ASEG = [ (18,  2, 'Left-Amygdala'),
              (54,  2, 'Right-Amygdala') ]

THAL_ASEG = [ (10,  2, 'Left-Thalamus-Proper'),
              (49,  2, 'Right-Thalamus-Proper') ]

OTHER_SGM_ASEG = [ (11,  2, 'Left-Caudate'),
                   (12,  2, 'Left-Putamen'),
                   (13,  2, 'Left-Pallidum'),
                   (26,  2, 'Left-Accumbens-area'),
                   (50,  2, 'Right-Caudate'),
                   (51,  2, 'Right-Putamen'),
                   (52,  2, 'Right-Pallidum'),
                   (58,  2, 'Right-Accumbens-area') ]


VENTRICLE_CP_ASEG = [ [ ( 4,  4, 'Left-Lateral-Ventricle'),
                        (31,  4, 'Left-choroid-plexus')     ],
                      [ (43,  4, 'Right-Lateral-Ventricle'),
                        (63,  4, 'Right-choroid-plexus')    ] ]


CEREBELLUM_ASEG = [ ( 6,  4, 'Left-Cerebellum-Exterior'),
                    ( 7,  3, 'Left-Cerebellum-White-Matter'),
                    ( 8,  2, 'Left-Cerebellum-Cortex'),
                    (45,  4, 'Right-Cerebellum-Exterior'),
                    (46,  3, 'Right-Cerebellum-White-Matter'),
                    (47,  2, 'Right-Cerebellum-Cortex') ]


CORPUS_CALLOSUM_ASEG = [ (192, 'Corpus_Callosum'),
                         (251, 'CC_Posterior'),
                         (252, 'CC_Mid_Posterior'),
                         (253, 'CC_Central'),
                         (254, 'CC_Mid_Anterior'),
                         (255, 'CC_Anterior') ]


BRAIN_STEM_ASEG = [ (16, 'Brain-Stem'),
                    (27, 'Left-Substancia-Nigra'),
                    (28, 'Left-VentralDC'),
                    (59, 'Right-Substancia-Nigra'),
                    (60, 'Right-VentralDC') ]


SGM_FIRST_MAP = { 'L_Accu':'Left-Accumbens-area',  'R_Accu':'Right-Accumbens-area',
                  'L_Amyg':'Left-Amygdala',        'R_Amyg':'Right-Amygdala',
                  'L_Caud':'Left-Caudate',         'R_Caud':'Right-Caudate',
                  'L_Hipp':'Left-Hippocampus',     'R_Hipp':'Right-Hippocampus',
                  'L_Pall':'Left-Pallidum',        'R_Pall':'Right-Pallidum',
                  'L_Puta':'Left-Putamen',         'R_Puta':'Right-Putamen',
                  'L_Thal':'Left-Thalamus-Proper', 'R_Thal':'Right-Thalamus-Proper' }


