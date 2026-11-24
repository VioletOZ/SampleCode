-- OZServer DB 스키마 (MySQL 8.0, X Plugin 33060으로 접속)
-- 서버는 시작 시 테이블을 만들지 않는다. 처음 한 번 이 파일을 실행한다.
--   mysql -u root -p < db/schema.sql
-- docker-compose.yml을 쓰면 MySQL 컨테이너가 처음 시작될 때 자동으로 실행된다.

CREATE DATABASE IF NOT EXISTS `account` DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci;
USE `account`;

-- 유저 계정. 로그인 ID(uuid)로 조회하고, 없으면 자동 생성(닉네임은 서버가 15자리 숫자로 생성)
--   SELECT id, nickname FROM user WHERE uuid = ?                     (로그인, Redis 캐시 미적중 시)
--   INSERT INTO user (uuid, nickname, login_date) VALUES (?, ?, NOW()) (신규)
--   UPDATE user SET login_date = NOW() WHERE id IN (...)               (지연 쓰기, 1초 또는 1000개 묶음)
CREATE TABLE IF NOT EXISTS `user` (
  `id`           int          NOT NULL AUTO_INCREMENT,
  `uuid`         varchar(20)  NOT NULL,                           -- 로그인 ID (UserManager::MAX_UUID_LENGTH)
  `nickname`     varchar(20)  NOT NULL,                           -- 중복 불가, 충돌 시 서버가 재생성
  `created_date` datetime     NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `login_date`   datetime     DEFAULT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uuid_UNIQUE` (`uuid`),
  UNIQUE KEY `nickname_UNIQUE` (`nickname`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
