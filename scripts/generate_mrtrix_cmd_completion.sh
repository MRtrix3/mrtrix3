#!/bin/bash
#
# Generates bash completion file for MRtrix commands
# MRtrix needs to be installed prior to running this
#
# Author: Rami Tabbara
#

output_file="mrtrix_command_completion"
mrtrix_cmd_dir=$(dirname $(which mrview))
mrtrix_commands=$(find $mrtrix_cmd_dir -executable -type f -exec basename {} \; | tr "\n" " ")

printf '
# MRtrix command completion
#
# Author: Rami Tabbara
#
# The easiest way to for a user without admin access to use this is to reference this file in .bashrc
# i.e. add the following line in .bashrc
# . /your_mrtrix_scripts_dir_path/mrtrix_command_completion
#

' > "$output_file"

for command in $mrtrix_commands
do	
	# Parse command's full usage page and extract 
	options=`$command __print_full_usage__ | grep -ohE -- "^OPTION[[:space:]][^[:space:]]+" | sed -E "s/^OPTION[[:space:]]//g;" | tr "\n" " "`
	dash_options=$(echo $options | sed -E 's/([^[:space:]]+)/-\1 --\1/g')
	
	current_option=""
	current_num_of_option_args=0
	current_arg=""
    current_arg_optional=-1
    current_arg_allows_multiple=-1
	current_arg_type=""
	current_arg_choices=""	
	
	flush_option_num_args()
	{
		if [[ "$current_option" == "" ]]; then
			printf '\t_%s_max_num_args() { echo %s; }\n' "$command" "$current_num_of_option_args" >> "$output_file"
		else					
			printf '\t_%s_%s_max_num_args() { echo %s; }\n' "$command" "$current_option" "$current_num_of_option_args" >> "$output_file"
		fi
	}

	flush_option_arg()
	{
		if [[ "$current_option" == "" ]]; then		
			printf '\t_%s_arg_%s()' "$command" "$current_num_of_option_args" >> "$output_file"
		else
			printf '\t_%s_%s_arg_%s()' "$command" "$current_option" "$current_num_of_option_args" >> "$output_file"
		fi

		printf ' { echo "%s"; }\n' "$current_arg_choices" >> "$output_file"
	}
	
	flush_option_arg_allows_multiple()
	{
		if [[ "$current_option" == "" ]]; then		
			printf '\t_%s_arg_%s_allows_multiple()' "$command" "$current_num_of_option_args" >> "$output_file"
		else
			printf '\t_%s_%s_arg_%s_allows_multiple()' "$command" "$current_option" "$current_num_of_option_args" >> "$output_file"
		fi
		
		printf ' { echo %s; }\n' "$current_arg_allows_multiple" >> "$output_file"
	}

	parse_option_arg_choices()
	{
		current_arg_choices=""
		choices=($*)	
		case "$current_arg_type" in
		"CHOICE")
					for choice in "${choices[@]}"; do
						# End of choice lists are generally delimited by -1
						# however tckgen choices are delimited by 2 (?)
						if [[ "$choice" == -1 || "$choice" == 2 ]]; then
							break;
						else
							current_arg_choices="$current_arg_choices $choice"
						fi
					done;;
		"IMAGEIN")				
					current_arg_choices='`ls *.{mih,mif,nii,dcm,msf,hdr,mgh} 2> /dev/null | tr "\n" " "`';;
		"FILEIN")
					current_arg_choices='`ls 2> /dev/null | grep -E "*\.[^[:space:]]+"`';;
		
		"FLOAT")	;&	
		"INT")
					# Default value		
					current_arg_choices=${choices[2]};;
		esac
	}

	printf '
_%s()
{ 
'   "$command" >> "$output_file"

	while IFS='' read -r line || [[ -n "$line" ]]; do
		words=($line)
    	case "${words[0]}" in
    	"OPTION") 						
					flush_option_num_args;
					current_arg="";
					current_arg_type="";
					current_option="${words[1]}";
				  	current_num_of_option_args=0;;
    	"ARGUMENT") 
					current_arg="${words[1]}";
					current_arg_optional="${words[2]}"; 
					current_arg_allows_multiple="${words[3]}";
					current_arg_type="${words[4]}";
					parse_option_arg_choices "${words[@]:5}";
					flush_option_arg_allows_multiple;
					flush_option_arg;
					let "current_num_of_option_args+=1";;
		esac	
	done < <(echo "`$command __print_full_usage__`")

	flush_option_num_args;
	
	printf '
	current_option=""
	current_max_args=0
	current_arg_index=0;
	consumed_option_tokens=0;
	current_num_consumed_option_args=0;
	first_option_consumed=0;

	COMPREPLY=();
	cur="${COMP_WORDS[COMP_CWORD]}";

	if [[ "$cur" == -* ]]; then
		COMPREPLY=( $( compgen -W "'%s'" -- "$cur" ) );
	else
		current_option=""
		word=""		

		for ((i=${#COMP_WORDS[@]}-2; i>=0; i--)); do
  			word="${COMP_WORDS[$i]}"
			if [[ "$word" == -* ]]; then
				option=$(echo $word | sed -E "s/-//g");
				arg_index=$(( $COMP_CWORD - $i - 1));
				max_args=$(_%s_${option}_max_num_args);
				
				current_num_consumed_option_args=$((current_num_consumed_option_args > max_args ? max_args : current_num_consumed_option_args))
				consumed_option_tokens=$(($current_consumed_option_tokens + $current_num_consumed_option_args + 1))
				current_num_consumed_option_args=0;
				
				if (( first_option_consumed == 0 )); then
					current_option="$option";
					current_arg_index=$arg_index;
					current_max_args=$max_args;
					first_option_consumed=1;
				fi
				
			else
				current_num_consumed_option_args=$(($current_num_option_args + 1));
			fi
		done

		if [[ "$current_option" != "" ]] && (( $current_arg_index >= 0 && $current_arg_index < $current_max_args )); then			
			COMPREPLY=( $(_%s_${current_option}_arg_${current_arg_index}));	
		else
			current_arg_index=$(($COMP_CWORD - $consumed_option_tokens - 1));		
			current_max_args=$(_%s_max_num_args);

			# Check whether any of our arguments accepts multiple inputs
			# If so, adjust the argument index to be the min. of these
			for ((i=${current_max_args} - 1; i>=0; i--)); do
				if (( $(_%s_arg_${i}_allows_multiple) == 1 )); then
					current_arg_index=$((current_arg_index > i ? i : current_arg_index))
				fi
			done

			if (( $current_arg_index >= 0 && $current_arg_index < $current_max_args )); then
				COMPREPLY=( $(_%s_arg_${current_arg_index}));
			fi
		fi
		
		COMPREPLY=( $( for i in ${COMPREPLY[@]} ; do echo $i ; done | grep "^${cur}" ) )
	fi
}
complete -F _%s %s \n' "$dash_options" "$command" "$command" "$command" "$command" "$command" "$command" "$command" >> "$output_file"
done

