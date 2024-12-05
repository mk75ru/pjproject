#!/usr/bin/env sh

source ./.env

export PGPASSWORD="incom"

psql -U postgres --username=${DB_USER} --dbname=${DB_NAME} --host=localhost \
 -c  "SELECT * FROM timescaledb_information.chunks WHERE hypertable_name = '${DB_SCHEMA}.${DB_TABLE}';"
