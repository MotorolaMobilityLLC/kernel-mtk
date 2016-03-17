#!/bin/bash

cat $1 | grep "\*TOI\*" | cut -b 22- | sed "s/ /,/g" | sed "s/\.//" | sort -n > $1.tmp
COLUMNS=$(cat $1.tmp | awk -F ',' ' { print $2 } ' | sort | uniq)
echo -n "pfn," > $1.tmp2
for NAME in $COLUMNS; do
    echo -n "$NAME," >> $1.tmp2
done
echo >> $1.tmp2
FIRST=1
declare -A data
while IFS=, read -r pfn column value; do
    if [ $FIRST -eq 1 ]; then
        FIRST=0
        LAST_PFN=$pfn
    fi
    if [ $pfn -ne $LAST_PFN ]; then
        echo -n "$LAST_PFN," >> $1.tmp2;
        for NAME in $COLUMNS; do
            echo -n "${data[$NAME]}," >> $1.tmp2
        done
        data=( )
        echo >> $1.tmp2
        LAST_PFN=$pfn
    fi
    if [ -z "$value" ]; then
        data[$column]=X
    else
        data[$column]=$value
    fi
done < $1.tmp
mv $1.tmp2 $1.csv
rm $1.tmp
LIBREOFFICE=$(which libreoffice)
[ -n "$LIBREOFFICE" ] && libreoffice $1.csv &
