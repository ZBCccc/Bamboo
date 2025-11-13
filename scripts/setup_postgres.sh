#!/usr/bin/env bash
set -euo pipefail

# PURPOSE:
#   Install and start PostgreSQL on Ubuntu, create role `sse` (password `123456`)
#   and database `bamboo`. Safe to run multiple times.

if ! command -v sudo >/dev/null 2>&1; then
  echo "sudo is required" >&2
  exit 1
fi

echo "[1/4] Install PostgreSQL packages"
sudo apt-get update -y
sudo apt-get install -y postgresql postgresql-contrib

echo "[2/4] Enable and start PostgreSQL service"
sudo systemctl enable --now postgresql

echo "[3/4] Create role 'sse' and database 'bamboo' if not exists"
# Create role via DO block (allowed inside a function)
sudo -u postgres psql -v ON_ERROR_STOP=1 <<'SQL'
DO $$
BEGIN
  IF NOT EXISTS (SELECT FROM pg_roles WHERE rolname = 'sse') THEN
    CREATE ROLE sse LOGIN PASSWORD '123456';
  END IF;
END$$;
SQL

# Create database OUTSIDE a DO block (CREATE DATABASE not allowed inside DO)
DB_EXISTS=$(sudo -u postgres psql -Atqc "SELECT 1 FROM pg_database WHERE datname='bamboo'")
if [ "$DB_EXISTS" != "1" ]; then
  sudo -u postgres createdb -O sse bamboo
fi

# Ensure privileges (harmless if already granted)
sudo -u postgres psql -v ON_ERROR_STOP=1 -c "GRANT ALL PRIVILEGES ON DATABASE bamboo TO sse;"

echo "[4/4] Verify local connection via psql (using socket)"
sudo -u postgres psql -Atqc "SELECT 'ok'" >/dev/null && echo "PostgreSQL ready"

echo "Done. Connection string in code: postgresql://sse:123456@127.0.0.1/bamboo"

