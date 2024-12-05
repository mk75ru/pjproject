#!/usr/bin/env sh

source ./.env

export PGPASSWORD="incom"

if [[ $1 == "" ]]
then
    psql -U postgres --username=${DB_USER} --dbname=${DB_NAME} --host=localhost \
        -c  "SELECT show_chunks('${DB_SCHEMA}.${DB_TABLE}');"
    echo "Укажите имя удаляемого чанка в качесте аргумента скрипта"
    exit 1
fi

psql -U postgres --username=${DB_USER} --dbname=${DB_NAME} --host=localhost \
 -c "SELECT _timescaledb_functions.drop_chunk('$1');"
