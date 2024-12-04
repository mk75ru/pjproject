#!/usr/bin/env sh

source ./.env

export PGPASSWORD="incom"

../createdb.sh -f

#psql -U postgres --username=${DB_USER} --dbname=${DB_NAME} --host=localhost \
#-c  "SELECT remove_retention_policy('${DB_SCHEMA}.${DB_TABLE}',if_exists => BOOLEAN 'true');"

#psql -U postgres --username=${DB_USER} --dbname=${DB_NAME} --host=localhost \
#-c "SELECT add_retention_policy('${DB_SCHEMA}.${DB_TABLE}', drop_after => INTERVAL '10 seconds');"


node test-timescaledb.js


#psql -U postgres --username=${DB_USER} --dbname=${DB_NAME} --host=localhost \
#-c "SELECT * FROM ${DB_SCHEMA}.${DB_TABLE} ORDER BY id DESC, human_date_ev DESC;"
