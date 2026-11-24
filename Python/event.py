from dataclasses import dataclass, field
from datetime import date
from typing import Protocol

from common import GameError, Inventory, Result
from quest import Action


class Ranking(Protocol):
    def set_score(self, key: str, user_id: int, score: int) -> None: ...
    def top(self, key: str, count: int) -> list: ...
    def rank(self, key: str, user_id: int) -> int: ...
    def size(self, key: str) -> int: ...


class RedisRanking:
    def __init__(self, redis):
        self._redis = redis

    def set_score(self, key, user_id, score):
        self._redis.zadd(key, {user_id: score}, gt=True)

    def top(self, key, count):
        return [(int(user_id), int(score)) for user_id, score in self._redis.zrevrange(key, 0, count - 1, withscores=True)]

    def rank(self, key, user_id):
        rank = self._redis.zrevrank(key, user_id)
        return 0 if rank is None else rank + 1

    def size(self, key):
        return self._redis.zcard(key)


@dataclass(frozen=True)
class RankReward:
    reward: dict
    max_rank: int = 0
    max_ratio: float = 0.0

    def matches(self, rank, total):
        return rank <= self.max_rank if self.max_rank else rank / total <= self.max_ratio


@dataclass(frozen=True)
class PassStep:
    score: int
    free: dict
    premium: dict


@dataclass(frozen=True)
class Goods:
    price: int
    limit: int
    reward: dict


# 이벤트마다 다른 값만 정의하고 기능은 EventService가 공유한다
@dataclass(frozen=True)
class Event:
    event_id: int
    start: date
    end: date
    claim_end: date
    rank_rewards: tuple = ()
    pass_steps: tuple = ()
    goods: dict = field(default_factory=dict)

    @property
    def ranking_key(self):
        return f'event:ranking:{self.event_id}'


class EventService:
    def __init__(self, db, events, ranking: Ranking, quests):
        self._db = db
        self._events = {event.event_id: event for event in events}
        self._ranking = ranking
        self._quests = quests

    def finish_play(self, user_id, event_id, score, point, today):
        event = self._get(event_id)
        if not event.start <= today <= event.end:
            raise GameError(Result.NOT_IN_PERIOD)
        if score < 0 or point < 0:
            raise GameError(Result.INVALID_REQUEST)

        with self._db.transaction() as tx:
            tx.execute('INSERT INTO user_event VALUES (?, ?, ?, ?, 0, 0, 0, 0) ON CONFLICT (user_id, event_id) DO UPDATE SET '
                       'score = MAX(score, excluded.score), point = point + excluded.point', user_id, event_id, score, point)
            self._quests.occur(tx, user_id, Action.EVENT_PLAY, 1, today, event_id)
        self._ranking.set_score(event.ranking_key, user_id, score)

    def ranking(self, user_id, event_id, count=50):
        event = self._get(event_id)
        return self._ranking.top(event.ranking_key, count), self._ranking.rank(event.ranking_key, user_id)

    # 지급까지 끝나야 수령 처리된다
    def claim_ranking(self, user_id, event_id, today):
        event = self._get(event_id)
        if not event.end < today <= event.claim_end:
            raise GameError(Result.NOT_IN_PERIOD)

        rank, total = self._ranking.rank(event.ranking_key, user_id), self._ranking.size(event.ranking_key)
        reward = next((r.reward for r in event.rank_rewards if rank and r.matches(rank, total)), None)
        if reward is None:
            raise GameError(Result.NOT_FOUND)

        with self._db.transaction() as tx:
            if not tx.execute('UPDATE user_event SET ranking_rewarded = 1 WHERE user_id = ? AND event_id = ? AND ranking_rewarded = 0',
                              user_id, event_id):
                raise GameError(Result.ALREADY_DONE)
            Inventory.add(tx, user_id, reward)
        return rank, reward

    # 결제 검증이 끝난 뒤 호출
    def unlock_pass(self, user_id, event_id):
        with self._db.transaction() as tx:
            if not tx.execute('UPDATE user_event SET premium = 1 WHERE user_id = ? AND event_id = ? AND premium = 0', user_id, event_id):
                raise GameError(Result.ALREADY_DONE)

    def claim_pass(self, user_id, event_id, today):
        event = self._get(event_id)
        if not event.start <= today <= event.claim_end:
            raise GameError(Result.NOT_IN_PERIOD)

        with self._db.transaction() as tx:
            row = self._row(tx, user_id, event_id)
            reached = sum(row['score'] >= step.score for step in event.pass_steps)
            premium = reached if row['premium'] else row['premium_step']
            rewards = [step.free for step in event.pass_steps[row['free_step']:reached]]
            rewards += [step.premium for step in event.pass_steps[row['premium_step']:premium]]
            if not rewards:
                raise GameError(Result.NOT_COMPLETE)

            tx.execute('UPDATE user_event SET free_step = ?, premium_step = ? WHERE user_id = ? AND event_id = ?',
                       reached, premium, user_id, event_id)
            for reward in rewards:
                Inventory.add(tx, user_id, reward)
        return rewards

    def exchange(self, user_id, event_id, goods_id, count, today):
        event = self._get(event_id)
        goods = event.goods.get(goods_id)
        if goods is None:
            raise GameError(Result.NOT_FOUND)
        if not event.start <= today <= event.claim_end:
            raise GameError(Result.NOT_IN_PERIOD)
        if count < 1:
            raise GameError(Result.INVALID_REQUEST)

        with self._db.transaction() as tx:
            bought = tx.one('SELECT count FROM user_exchange WHERE user_id = ? AND event_id = ? AND goods_id = ?', user_id, event_id, goods_id)
            if (bought['count'] if bought else 0) + count > goods.limit:
                raise GameError(Result.LIMIT_EXCEEDED)
            price = goods.price * count
            if not tx.execute('UPDATE user_event SET point = point - ? WHERE user_id = ? AND event_id = ? AND point >= ?',
                              price, user_id, event_id, price):
                raise GameError(Result.NOT_ENOUGH)
            tx.execute('INSERT INTO user_exchange VALUES (?, ?, ?, ?) ON CONFLICT DO UPDATE SET count = count + excluded.count',
                       user_id, event_id, goods_id, count)
            Inventory.add(tx, user_id, {item_id: amount * count for item_id, amount in goods.reward.items()})

    def _get(self, event_id):
        if event_id not in self._events:
            raise GameError(Result.NOT_FOUND)
        return self._events[event_id]

    @staticmethod
    def _row(tx, user_id, event_id):
        row = tx.one('SELECT * FROM user_event WHERE user_id = ? AND event_id = ?', user_id, event_id)
        if row is None:
            raise GameError(Result.NOT_FOUND)
        return row
