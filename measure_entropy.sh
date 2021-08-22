#!/bin/bash

tmp_file=$(mktemp)
ecalc="./ecalc"

for i in $(find $1)
do
    entropy=$($ecalc -e -f $i)
    file_type=$(file $i | awk '{print $2}')

    i=$(printf $i | rev | cut -d'/' -f 1 | rev)

    printf "%s %s %s\n" $i $file_type $entropy >> $tmp_file
done

sort -t' ' -k 3 -n $tmp_file | column -t -s' ' | less
rm $tmp_file

exit 0
