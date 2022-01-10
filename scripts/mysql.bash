<<<<<<< HEAD
ubuntuPD="root"
iReaderPD="123456@iReader"

mysql -u root -p << EOF
CREATE USER 'myReader'@'localhost' IDENTIFIED  BY '123456@iReader';
create database myDataBase charset utf8;
GRANT ALL on myDataBase.* to 'myReader'@'localhost';
exit
EOF


=======
ubuntuPD="root"
iReaderPD="123456@iReader"

mysql -u root -p << EOF
CREATE USER 'iReader'@'localhost' IDENTIFIED  BY '123456@iReader';
create database iReaderDataBase charset utf8;
GRANT ALL on iReaderDataBase.* to 'iReader'@'localhost';
exit
EOF


>>>>>>> 6e3698d0b0c8dda16fd56ca04329a5f75e4f0595
