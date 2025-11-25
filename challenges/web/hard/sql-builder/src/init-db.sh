#!/bin/bash
set -e

until mariadb -u root -e "SELECT 1;" &> /dev/null; do
  echo "Waiting for MariaDB to be ready..."
  sleep 1
done

mariadb -u root <<-EOSQL
    ALTER USER 'root'@'localhost' IDENTIFIED BY 'example';
    CREATE DATABASE IF NOT EXISTS example;
    FLUSH PRIVILEGES;

    USE example;
    CREATE TABLE users (name VARCHAR(100) PRIMARY KEY, email VARCHAR(100));
    INSERT INTO users (name, email) VALUES ('John Doe', 'john@example.com');

EOSQL
