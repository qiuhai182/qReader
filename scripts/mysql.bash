#!/bin/bash

mysql -u root -p << EOF
CREATE USER 'iReader'@'localhost' IDENTIFIED  BY '123456@iReader';
create database iReaderDataBase charset utf8;
GRANT ALL on iReaderDataBase.* to 'iReader'@'localhost';
exit
EOF
echo "create user:'iReader' with database:'iReaderDataBase' succeed."

