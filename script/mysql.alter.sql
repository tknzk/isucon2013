ALTER TABLE `isucon`.`memos` MODIFY COLUMN is_private TINYINT(1);
ALTER TABLE `isucon`.`memos` ADD INDEX user_is_private_idx (`user`, `is_private`);
ALTER TABLE `isucon`.`memos` ADD INDEX is_private_idx (`is_private`);
ALTER TABLE `isucon`.`users` ADD INDEX users_username_12_idx (`username`(12));
ALTER TABLE `isucon`.`memos` ADD COLUMN `username` varchar(255);
UPDATE memos , users set memos.username = users.username where memos.user = users.id;
