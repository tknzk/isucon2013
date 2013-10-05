ALTER TABLE `isucon`.`memos` MODIFY COLUMN is_private TINYINT(1);
ALTER TABLE `isucon`.`memos` ADD INDEX user_is_private_idx (`user`, `is_private`);
ALTER TABLE `isucon`.`memos` ADD INDEX is_private_idx (`is_private`);
