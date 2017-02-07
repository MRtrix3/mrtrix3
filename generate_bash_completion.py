#!/usr/bin/env python
#
# Generates bash completion file for MRtrix commands
# MRtrix needs to be built prior to running this
#
# Author: Rami Tabbara
#


from __future__ import print_function
import getopt, sys, os, re, subprocess, locale

def usage():
  print ('''
Usage:	%s -m [dir_path] -c [file_path]\n
	-m, --mrtrix_commands_dir [dir path]: The directory where the mrtrix executable commands reside.
	-c, --completion_path [file path]: The generated bash completion file path.
''' % (sys.argv[0]))


def main(argv):
	try:
		opts, args = getopt.getopt(argv, "m:c:", ["mrtrix_commands_dir=", "completion_path="])
		if not opts or len(opts) != 2:
			raise getopt.GetoptError('Incorrect number of arguments') 
	except getopt.GetoptError:
		usage()
		sys.exit(2)
	
	commands_dir = ""
	completion_path = ""
	commands = []

	for opt, arg in opts:
		if opt in ("-m", "--mrtrix_commands_dir"):
			commands_dir = arg
		elif opt in ("-c", "--completion_path"):
			completion_path = arg
	
	if not commands_dir or not os.path.isdir (commands_dir) or not completion_path:
		usage()
		sys.exit (2)
	
	# Filter out any non executable files or debug commands
	commands = [comm for comm in os.listdir (commands_dir) if os.access ( os.path.join( commands_dir, comm ), os.X_OK) and not re.match (r'^\w+__debug', comm)]

	if not commands:
		sys.exit (0)
	
	parse_commands (commands_dir, completion_path, commands)




###########################################################################
#                         PARSE FUNCTION
###########################################################################


def parse_commands (commands_dir, completion_path, commands):
		
	completion_file = open (completion_path, 'w+')
		
###########################################################################
#                         INNER PARSE FUNCTIONS
###########################################################################

	def flush_option_num_args ():
		if not current_option:
			print ('\t_%s_max_num_args() { echo %s; }\n' % (command, current_num_of_option_args), file = completion_file)
		else:					
			print ('\t_%s_%s_max_num_args() { echo %s; }\n' % (command, current_option, current_num_of_option_args), file = completion_file)
	
	def flush_option_arg ():
		if not current_option:		
			print ('\t_%s_arg_%s()' % (command, current_num_of_option_args), file = completion_file)
		else:
			print ('\t_%s_%s_arg_%s()' % (command, current_option, current_num_of_option_args), file = completion_file)

		print ('\t{ echo "%s"; }\n' % (current_arg_choices) , file = completion_file)

	def	flush_option_arg_allows_multiple ():
		if not current_option:	
			print ('\t_%s_arg_%s_allows_multiple()' %  (command, current_num_of_option_args), file = completion_file)
		else:
			print ('\t_%s_%s_arg_%s_allows_multiple()' % (command, current_option, current_num_of_option_args), file = completion_file)
		
		print ('\t{ echo %s; }\n' % (current_arg_allows_multiple), file = completion_file)
	
	def parse_option_arg_choices (arg_type, choices = []):
		arg_choices = ""

		if arg_type == 'CHOICE':
			for choice in choices:
				choice = choice.rstrip ()
				# End of choice lists are generally delimited by -1
				# however tckgen choices are delimited by 2 (?)
				if choice in ['-1', '2']:
					break;
				else:
					arg_choices +=  " " + choice
		elif arg_type == 'IMAGEIN':
			arg_choices ='`eval ls $1*.{mih,mif,nii,dcm,msf,hdr,mgh,nii.gz,mif.gz} 2> /dev/null | tr "\n" " "``eval ls -d $1*/ 2> /dev/null | tr "\n" " "`'
		elif arg_type == 'FILEIN':
			arg_choices ='`eval ls "$1*" 2> /dev/null | grep -E "$1*\.[^[:space:]]+"``eval ls -d $1*/ 2> /dev/null | tr "\n" " "`'
		elif arg_type == 'TRACKSIN':
			arg_choices ='`eval ls $1*.tck 2> /dev/null | tr "\n" " "``eval ls -d $1*/ 2> /dev/null | tr "\n" " "`'
		#elif arg_type in ['FLOAT', 'INT']:
		else:
			arg_choices = "__empty__"
		

		return arg_choices


###########################################################################
#                          IS_SCRIPT FUNCTION
###########################################################################
	def is_script (command):
		textchars = bytearray({7,8,9,10,12,13,27} | set(range(0x20, 0x100)) - {0x7f})
		with open(os.path.join(commands_dir, command), "rb") as f:
			bytes = f.read(1024)
		return not bytes.translate(None, textchars)

	
###########################################################################
#                         END INTERNAL FUNCTIONS
###########################################################################

	encoding = locale.getdefaultlocale()[1]

	for command in sorted(commands):

		if is_script(command):
			continue

		single_dash_options = ""
		double_dash_options = ""
		current_option = ""
		current_num_of_option_args = 0
		current_arg = ""
		current_arg_optional = -1
		current_arg_allows_multiple = -1
		current_arg_type = ""
		current_arg_choices = ""			

		try:
			process = subprocess.Popen ([ os.path.join( commands_dir, command ), '__print_full_usage__'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)

			print ('''
_%s()
{ 
''' % (command), file = completion_file)

			for line in process.stdout:
				words = line.decode(encoding).split (' ')
				
				if not words:
					continue				
				
				if words[0] == 'OPTION':
					flush_option_num_args ()
					current_arg = ""
					current_arg_type = ""
					current_option = words[1]
					single_dash_options += ' -' + current_option
					double_dash_options += ' --' + current_option
					current_num_of_option_args = 0
				elif words[0] == 'ARGUMENT':
					current_arg = words[1]
					current_arg_optional = words[2] 
					current_arg_allows_multiple = words[3]
					current_arg_type = words[4].rstrip ()
					current_arg_choices = parse_option_arg_choices (current_arg_type, words[5:])
					flush_option_arg_allows_multiple ()
					flush_option_arg ()
					current_num_of_option_args += 1

			flush_option_num_args ()

			print ('''
	current_option=""
	current_max_args=0
	current_arg_index=0;
	consumed_option_tokens=0;
	current_num_consumed_option_args=0;
	first_option_consumed=0;

	COMPREPLY=();
	cur="${COMP_WORDS[COMP_CWORD]}";
	cur_filtered=`echo "${COMP_WORDS[COMP_CWORD]}" | sed 's/\..*$//g'`;

	if [[ "$cur" == --* ]]; then
		COMPREPLY=( $( compgen -W "%s" -- "$cur" ) );
	elif [[ "$cur" == -* ]]; then
		COMPREPLY=( $( compgen -W "%s" -- "$cur" ) );
	else
		current_option=""
		word=""	
		array=""	

		for ((i=${#COMP_WORDS[@]}-2; i>=0; i--)); do
  			word="${COMP_WORDS[$i]}"
			if [[ "$word" == -* ]]; then
				option=$(echo $word | sed -E "s/-//g");
				if [[ ${#option} == 0 ]]; then
					continue; 
				fi
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
			array=`_%s_${current_option}_arg_${current_arg_index} $cur_filtered`
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
				array=`_%s_arg_${current_arg_index} $cur_filtered`;
			fi
		fi

		if [[ "$array" == "__empty__" ]]; then
			COMPREPLY=()
			compopt +o default %s;
		else
			mapfile -t COMPREPLY < <( compgen -W "$array" -- "$cur" )
			compopt -o default %s;
		fi
	fi
}
complete -F _%s %s \n''' % (double_dash_options, single_dash_options, command, command, command, command, command, command, command, command, command), file = completion_file)

		except:
			pass

	completion_file.close ()


if __name__ == "__main__":
	main(sys.argv[1:])
