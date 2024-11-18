#!/usr/bin/env sh

source ./.env

export PGPASSWORD="incom"

createdb() {
psql -U postgres -c "CREATE DATABASE test_database;"
psql -U postgres -c "CREATE USER test_user WITH password 'qwerty';"
psql -U postgres -c "GRANT ALL ON DATABASE test_database TO test_user;"
psql -U postgres --host=localhost --dbname=test_database --username=test_user \
    -c "CREATE EXTENSION IF NOT EXISTS timescaledb;"
psql -U postgres --host=localhost --dbname=test_database --username=test_user \
    -c "CREATE TABLE conditions(tstamp timestamptz NOT NULL,device VARCHAR(32) NOT NULL,temperature FLOAT NOT NULL);"
psql -U postgres --host=localhost --dbname=test_database --username=test_user \
     -c "SELECT create_hypertable('conditions', 'tstamp',chunk_time_interval => INTERVAL '1 day');"
}

createdb
