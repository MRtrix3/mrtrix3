#!/bin/bash
#
# Generates bash completion file for MRtrix commands
# MRtrix needs to be installed prior to running this
#
# Author: Rami Tabbara
#

output_file="mrtrix_command_completion"
mrtrix_cmd_dir=$(dirname $(which mrview))
mrtrix_commands=$(find /home/rami/mrtrix3/bin/ -executable -type f -exec basename {} \; | tr "\n" " ")

printf '
# MRtrix command completion
#
# The easiest way to for a user without admin access to use this is to reference this file in .bashrc
# i.e. add the following line in .bashrc
# . /your_mrtrix_scripts_dir_path/mrtrix_command_completion
#
' > "$output_file"

for command in $mrtrix_commands
do	
	# Parse command's help page and extract options
	# In particular, options are listed at the start of a newline and are of the form -foo
	options=`$command --help | col -b | grep -ohE -- "^[[:space:]]+-[^[:space:]]+" | tr -s "^[:space:]+" " " | tr "\n" " "`
	printf '
_%s()
{
	local cur

	COMPREPLY=()
	cur=${COMP_WORDS[COMP_CWORD]}

	if [[ "$cur" == -* ]]; then
		COMPREPLY=( $( compgen -W "'%s'" -- "$cur" ) )
	fi
}
complete -F _%s %s
' "$command" "$options" "$command" "$command" >> "$output_file"
done

