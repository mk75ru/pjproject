#!/usr/bin/env sh

source ./.env

export PGPASSWORD="incom"

psql -U postgres --username=${DB_USER} --dbname=${DB_NAME} --host=localhost \
-c "SELECT * FROM ${DB_SCHEMA}.${DB_TABLE} ORDER BY id DESC, human_date_ev DESC;"
