#!/bin/sh
#mysql -uroot -proot isucon < /home/isucon/mysql.alter.sql
cat /home/isucon/script/mysql.alter.sql | mysql -uroot -proot isucon
