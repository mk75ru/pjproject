#!/usr/bin/env sh
#
source ./.env

export PGPASSWORD="incom"

psql -U postgres -c "DROP DATABASE IF EXISTS autocall;"

psql -U postgres -c "DROP DATABASE IF EXISTS test;"

psql -U postgres -c "DROP DATABASE IF EXISTS test_database;"
