#!/bin/bash
for pattern in ./slink/scripts/simple/*.txt ./slink/scripts/intermediate/*.txt; do
	for file in $pattern; do
		echo "################################################################################"
		echo "$file Running for solution -----------------------------------------------------"
	        ./stsh-driver -t $file -s ./slink/stsh_soln -a "--suppress-prompt --no-history"
#	        str1=$(./stsh-driver -t $file -s ./slink/stsh_soln -a "--suppress-prompt --no-history")
	        echo "$file Running for my solution --------------------------------------------------"
	        ./stsh-driver -t $file -s ./stsh -a "--suppress-prompt --no-history"
		sleep 5
#	        str2=$(./stsh-driver -t $file -s ./stsh -a "--suppress-prompt --no-history")
#	        if ["$str1" == "$str2" ]; then
#	                echo "$file passed"
#	        else
#	                echo "$file failed"
#	                exit 1
#	        fi
	done;
done;
# pipe, redirection not working with driver...why?
for pattern in ./slink/scripts/advanced/*.txt; do
	for file in $pattern; do
		echo "################################################################################"
		echo "$file Running for solution -----------------------------------------------------"
#	        ./stsh-driver -t $file -s ./slink/stsh_soln -a "--suppress-prompt --no-history"
	        str1=$(./stsh-driver -t $file -s ./slink/stsh_soln -a "--suppress-prompt --no-history")
	        echo "$file Running for my solution --------------------------------------------------"
#	        ./stsh-driver -t $file -s ./stsh -a "--suppress-prompt --no-history"
#		sleep 5
	        str2=$(./stsh-driver -t $file -s ./stsh -a "--suppress-prompt --no-history")
	        if ["$str1" == "$str2" ]; then
	                echo "$file passed"
	        else
	                echo "$file failed"
	                sleep 5
	        fi
	done;
done;
