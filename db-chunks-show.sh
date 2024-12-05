#!/usr/bin/env sh

source ./.env

export PGPASSWORD="incom"

psql -U postgres --username=${DB_USER} --dbname=${DB_NAME} --host=localhost \
 -c  "SELECT show_chunks('${DB_SCHEMA}.${DB_TABLE}');"
