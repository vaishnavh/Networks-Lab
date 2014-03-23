#!/bin/bash
ws=$1
p=$2
while [ "$(echo $p '<=' 1 | bc -l)" -eq 1 ]
do
	q=0
	if [ "$(echo $p '==' $2 | bc -l)" -eq 1 ]
	then q=$3
	fi	
	while [ "$(echo $q '<=' 1 | bc -l)" -eq 1 ]
	do
		echo "$p $q"
		./script.sh $ws $p $q >> readings2.txt 
		./script.sh $ws $p $q >> readings2.txt
		q=`echo $q + 0.1 | bc `
	done
	p=`echo $p + 0.1 | bc`
done
