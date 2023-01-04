/* Copyright (c) 2008-2023 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#ifndef __file_pyconfig_h__
#define __file_pyconfig_h__

/* Config file options listed here so that they can be scraped by
 * generate_user_docs.sh and added to the list of config file options in
 * the documentation without modifying that script to read from the scripts
 * directory.
 */

//CONF option: Dwi2maskAlgorithm
//CONF default: legacy
//CONF The dwi2mask algorithm to utilise whenever dwi2mask must be invoked
//CONF within a Python script, and the user is not provided with the
//CONF opportunity to select the algorithm at the command-line.

//CONF option: Dwi2maskTemplateSoftware
//CONF default: fsl
//CONF The software to be used for registration and transformation
//CONF by default within the "dwi2mask template" algorithm.

//CONF option: Dwi2maskTemplateImage
//CONF default: (none)
//CONF The template image to utilise by default whenever the "dwi2mask ants"
//CONF or "dwi2mask template" algorithms are invoked but no template image
//CONF / mask pair are explicitly nominated at the command-line.

//CONF option: Dwi2maskTemplateMask
//CONF default: (none)
//CONF The template brain mask to utilise by default whenever the "dwi2mask
//CONF ants" or "dwi2mask template" algorithms are invoked but no template
//CONF image / mask pair are explicitly nominated at the command-line.

//CONF option: Dwi2maskTemplateANTsFullOptions
//CONF default: (none)
//CONF When dwi2mask template is used with -software antsfull (or
//CONF Dwi2maskTemplate is set to "antsfull"), specify the command-line
//CONF options with which command "antsRegistration" will be provided.

//CONF option: Dwi2maskTemplateANTsQuickOptions
//CONF default: (none)
//CONF When dwi2mask template is used with -software antsquick (or
//CONF Dwi2maskTemplate is set to "antsquick"), specify the command-line
//CONF options with which command "antsRegistrationSynQuick.sh" will be
//CONF provided.

//CONF option: Dwi2maskTemplateFSLFlirtOptions
//CONF default: (none)
//CONF When dwi2mask template is used with -software fsl (or
//CONF Dwi2maskTemplate is set to "fsl"), specify the command-line
//CONF options with which FSL command flirt will be provided.

//CONF option: Dwi2maskTemplateFSLFnirtConfig
//CONF default: (none)
//CONF When dwi2mask template is used with -software fsl (or
//CONF Dwi2maskTemplate is set to "fsl"), specify the configuration
//CONF file to be provided to the FSL command fnirt.

//CONF option: ScriptScratchDir
//CONF default: `.`
//CONF The location in which to generate the scratch directories to be
//CONF used by MRtrix Python scripts. By default they will be generated
//CONF in the working directory.
//CONF Note that this setting does not influence the location in which
//CONF piped images and other temporary files are created by MRtrix3;
//CONF that is determined based on config file option :option:`TmpFileDir`.

//CONF option: ScriptScratchPrefix
//CONF default: `<script>-tmp-`
//CONF The prefix to use when generating a unique name for a Python
//CONF script scratch directory. By default the name of the invoked
//CONF script itself will be used, followed by `-tmp-` (six random
//CONF characters are then appended to produce a unique name in cases
//CONF where a script may be run multiple times in parallel).

#endif
