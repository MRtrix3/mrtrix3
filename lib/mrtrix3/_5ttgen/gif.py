def initialise(base_parser, subparsers):
	import argparse
	from mrtrix3 import app
	parser = subparsers.add_parser('gif', author='Matteo Mancini (m.mancini@ucl.ac.uk)',
	synopsis='Generate the 5TT image based on a GIF segmentation image', parents=[base_parser])
	parser.add_argument('input',  help='The input GIF segmentation image')
	parser.add_argument('output', help='The output 5TT image')


def checkOutputPaths():
	pass


def getInputs():
	import os, shutil
	from mrtrix3 import app, path, run
	run.command('mrconvert ' + path.fromUser(app.args.input, True) + ' ' + path.toTemp('input.mif', True))


def execute():
	import os, sys
	from mrtrix3 import app, path, run

	# Generate the images related to each tissue
	run.command('mrconvert input.mif -coord 3 1 CSF.mif')
	run.command('mrconvert input.mif -coord 3 2 cGM.mif')
	run.command('mrconvert input.mif -coord 3 3 cWM.mif')
	run.command('mrconvert input.mif -coord 3 4 sGM.mif')

	# Combine WM and subcortical WM into a unique WM image
	run.command('mrconvert input.mif -coord 3 5 sWM.mif')
	run.command('mrcalc cWM.mif sWM.mif -add WM.mif')

	# Create an empty lesion image
	run.command('mrcalc WM.mif 0 -mul lsn.mif')

	# Convert into the 5tt format
	run.command('mrcat cGM.mif sGM.mif WM.mif CSF.mif lsn.mif 5tt.mif -axis 3')

	if app.args.nocrop:
		run.command('mrconvert 5tt.mif result.mif')
	else:
		run.command('mrmath 5tt.mif sum - -axis 3 | mrthreshold - - -abs 0.5 | mrcrop 5tt.mif result.mif -mask -')


