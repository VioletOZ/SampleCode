import sqlite3
from contextlib import contextmanager
from enum import Enum


class Result(Enum):
    OK = 0
    INVALID_REQUEST = 1
    NOT_FOUND = 2
    NOT_ENOUGH = 3
    NOT_OWNED = 4
    ALREADY_DONE = 5
    NOT_COMPLETE = 6
    MAX_LEVEL = 7
    LIMIT_EXCEEDED = 8
    NOT_IN_PERIOD = 9


class GameError(Exception):
    def __init__(self, code: Result):
        super().__init__(code.name)
        self.code = code


SCHEMA = '''
CREATE TABLE user_item    (user_id INT, item_id INT, count INT NOT NULL, PRIMARY KEY (user_id, item_id));
CREATE TABLE user_quest   (user_id INT, quest_id INT, period INT, progress INT, rewarded INT, PRIMARY KEY (user_id, quest_id));
CREATE TABLE user_summon  (user_id INT, banner_id INT, level INT, exp INT, free_day INT, free_count INT, PRIMARY KEY (user_id, banner_id));
CREATE TABLE user_skill   (user_id INT, skill_id INT, level INT, PRIMARY KEY (user_id, skill_id));
CREATE TABLE user_preset  (user_id INT, slot INT, skill_id INT, PRIMARY KEY (user_id, slot));
CREATE TABLE user_event   (user_id INT, event_id INT, score INT, point INT, premium INT, free_step INT, premium_step INT, ranking_rewarded INT,
                           PRIMARY KEY (user_id, event_id));
CREATE TABLE user_exchange(user_id INT, event_id INT, goods_id INT, count INT, PRIMARY KEY (user_id, event_id, goods_id));
'''


class Database:
    def __init__(self, path=':memory:'):
        self._conn = sqlite3.connect(path, isolation_level=None)
        self._conn.row_factory = sqlite3.Row
        self._conn.executescript(SCHEMA)

    # 예외가 나면 전부 되돌린다
    @contextmanager
    def transaction(self):
        self._conn.execute('BEGIN IMMEDIATE')
        try:
            yield Transaction(self._conn)
        except BaseException:
            self._conn.execute('ROLLBACK')
            raise
        self._conn.execute('COMMIT')


class Transaction:
    def __init__(self, conn):
        self._conn = conn

    def one(self, sql, *params):
        return self._conn.execute(sql, params).fetchone()

    def all(self, sql, *params):
        return self._conn.execute(sql, params).fetchall()

    def execute(self, sql, *params):
        return self._conn.execute(sql, params).rowcount

    def execute_many(self, sql, rows):
        self._conn.executemany(sql, rows)


class Inventory:
    @staticmethod
    def count(tx, user_id, item_id):
        row = tx.one('SELECT count FROM user_item WHERE user_id = ? AND item_id = ?', user_id, item_id)
        return row['count'] if row else 0

    @staticmethod
    def add(tx, user_id, items):
        tx.execute_many('INSERT INTO user_item VALUES (?, ?, ?) '
                        'ON CONFLICT (user_id, item_id) DO UPDATE SET count = count + excluded.count',
                        [(user_id, item_id, count) for item_id, count in items.items()])

    # 잔액 확인과 차감을 UPDATE 하나로 처리해 동시 요청에도 음수가 되지 않는다
    @staticmethod
    def consume(tx, user_id, item_id, count):
        if count < 0:
            raise GameError(Result.INVALID_REQUEST)
        if count and not tx.execute('UPDATE user_item SET count = count - ? WHERE user_id = ? AND item_id = ? AND count >= ?',
                                    count, user_id, item_id, count):
            raise GameError(Result.NOT_ENOUGH)
