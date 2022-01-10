ubuntuPD="root"
iReaderPD="123456@iReader"

mysql -u root -p << EOF
CREATE USER 'myReader'@'localhost' IDENTIFIED  BY '123456@iReader';
create database myDataBase charset utf8;
GRANT ALL on myDataBase.* to 'myReader'@'localhost';
exit
EOF


