#!/usr/bin/env sh

source ./.env

export PGPASSWORD="incom"
FORCE=$1

createdb() {
    psql -U postgres -c "DROP DATABASE IF EXISTS ${DB_NAME};"
    psql -U postgres -c "DROP ROLE IF EXISTS ${DB_USER};"
    psql -U postgres -c "CREATE DATABASE ${DB_NAME};"
    psql -U postgres -c "CREATE ROLE ${DB_USER} WITH LOGIN PASSWORD '${DB_PASSWORD}';"
    psql -U postgres -c "GRANT ALL PRIVILEGES ON DATABASE ${DB_NAME} TO ${DB_USER};"
    psql -U postgres --dbname=${DB_NAME} -c "GRANT ALL PRIVILEGES ON SCHEMA public TO ${DB_USER};"
    psql -U postgres -c "DROP EXTENSION IF EXISTS timescaledb;"
    psql -U postgres --username=${DB_USER} --dbname=${DB_NAME} --host=localhost \
        -c "CREATE EXTENSION IF NOT EXISTS timescaledb;"
    psql -U postgres --username=${DB_USER} --dbname=${DB_NAME} --host=localhost \
        -c "CREATE SCHEMA ${DB_SCHEMA};"
    psql -U postgres --username=${DB_USER} --dbname=${DB_NAME} --host=localhost \
        -c "CREATE TABLE ${DB_SCHEMA}.${DB_TABLE}(${DB_TABLE_COLUMNS});"
#    psql -U postgres --username=${DB_USER} --dbname=${DB_NAME} --host=localhost \
#        -c "CREATE INDEX ${DB_INDEX_NAME} ON ${DB_SCHEMA}.${DB_TABLE} ($DB_INDEX_FIELD);"
    psql -U postgres --username=${DB_USER} --dbname=${DB_NAME} --host=localhost \
        -c "SELECT create_hypertable('${DB_SCHEMA}.${DB_TABLE}','${DB_TIMESTAMP_COLUMN}', \
        chunk_time_interval => INTERVAL '${DB_INTERVAL}');"
    psql -U postgres --username=${DB_USER} --dbname=${DB_NAME} --host=localhost \
        -c "SELECT add_retention_policy('${DB_SCHEMA}.${DB_TABLE}', drop_after => INTERVAL '${DB_DROP_INTERVAL}');"

}

if psql -U postgres  -lqt | cut -d \| -f 1 | grep -qw ${DB_NAME}; then
    echo "база ${DB_NAME} уже существует"
    if [[ ${FORCE} == "-f" ]]
    then
        echo "принудительно пересоздаем базу ${DB_NAME}"
        createdb
    fi
else
    echo "база ${DB_NAME} не существует, создаем"
    createdb
fi
