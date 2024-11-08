#!/usr/bin/env sh


source ./.env


export PGPASSWORD="incom"

psql -U postgres -c "DROP DATABASE IF EXISTS ${DB_NAME};"
psql -U postgres -c "DROP ROLE IF EXISTS ${DB_USER};"
psql -U postgres -c "CREATE DATABASE ${DB_NAME};"
psql -U postgres -c "CREATE ROLE ${DB_USER} WITH LOGIN PASSWORD '${DB_PASSWORD}';"
psql -U postgres -c "GRANT ALL PRIVILEGES ON DATABASE ${DB_NAME} TO ${DB_USER};"
psql -U postgres --dbname=${DB_NAME} -c "GRANT ALL PRIVILEGES ON SCHEMA public TO ${DB_USER};"
psql -U postgres --username=${DB_USER} --dbname=${DB_NAME} --host=localhost -c "CREATE SCHEMA ${DB_SCHEMA};"
psql -U postgres --username=${DB_USER} --dbname=${DB_NAME} --host=localhost -c "CREATE TABLE ${DB_SCHEMA}.${DB_TABLE}(${DB_TABLE_COLUMNS});"
