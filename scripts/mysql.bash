ubuntuPD="root"
iReaderPD="123456@iReader"

mysql -u root -p << EOF
CREATE USER 'iReader'@'localhost' IDENTIFIED  BY '123456@iReader';
create database iReaderDataBase charset utf8;
GRANT ALL on iReaderDataBase.* to 'iReader'@'localhost';
exit
EOF


