#!/bin/bash


tables_migrate_directory=$1
file_supportTable=$2
file_include=$3

for file in $tables_migrate_directory/*;
do
    
    table=$(cat $file | grep tableStruct | cut -d ':' -f2 | tr -d ',' | tr -d '"' | tr -d ' ')
    supportTable=$(cat $file | grep supportTableStruct | cut -d ':' -f2 | tr -d ',' | tr -d '"' | tr -d ' ')
    tableName=$(cat $file | grep tableName | cut -d ':' -f2 | tr -d ',' | tr -d '"' | tr -d ' ')
    supportTableName=$(cat $file | grep supportTableName | cut -d ':' -f2 | tr -d ',' | tr -d '"' | tr -d ' ')
    tableType=$(cat $file | grep tableType | cut -d ':' -f2 | tr -d ',' | tr -d '"' | tr -d ' ')
    supportTableType=$(cat $file | grep supportTableType | cut -d ':' -f2 | tr -d ',' | tr -d '"' | tr -d ' ')
    
    echo ">>> Copying $table table to $supportTable table"
    sed -i -E '$a\ \n' $file_supportTable  
    sed -E "/(TABLE|struct \[\[eosio\:\:table\(\"$tableName\"\)\]\]) $table/,/$tableType\s*\;/!d" $file_include >> $file_supportTable
    sed -i "s/TABLE $table/TABLE $supportTable /g" $file_supportTable
    sed -i "s/struct \[\[eosio\:\:table(\"$tableName\")\]\] $table/struct \[\[eosio\:\:table(\"$supportTableName\")\]\] $supportTable /g" $file_supportTable
    sed -i "s/$table\s*,/$supportTable,/g" $file_supportTable
    sed -i "s/$table\s*:/$supportTable:/g" $file_supportTable
    sed -i "s/$table\s*>/$supportTable>/g" $file_supportTable
    sed -i "s/EOSLIB_SERIALIZE($table/EOSLIB_SERIALIZE($supportTable/g" $file_supportTable
    sed -i "s/name(\"$tableName\")\s*,/name(\"$supportTableName\"),/g" $file_supportTable
    sed -i "s/$tableType/$supportTableType/g" $file_supportTable
done


