import random
from collections import Counter
from dataclasses import dataclass
from enum import Enum

from common import GameError, Inventory, Result
from quest import Action


class Cost(Enum):
    CURRENCY = 1
    TICKET = 2
    FREE = 3


@dataclass(frozen=True)
class SummonLevel:
    next_exp: int
    weights: dict


@dataclass(frozen=True)
class Banner:
    banner_id: int
    levels: tuple
    currency_id: int
    price: int
    ticket_id: int
    max_count: int
    free_count: int
    free_limit: int


class SummonService:
    def __init__(self, db, banners, quests, rng=None):
        self._db = db
        self._banners = {banner.banner_id: banner for banner in banners}
        self._quests = quests
        self._rng = rng or random.SystemRandom()

    # 비용 차감, 뽑기, 지급, 퀘스트를 한 트랜잭션으로 처리
    def summon(self, user_id, banner_id, cost, count, today):
        banner = self._banners.get(banner_id)
        if banner is None:
            raise GameError(Result.NOT_FOUND)
        if cost is not Cost.FREE and not 1 <= count <= banner.max_count:
            raise GameError(Result.INVALID_REQUEST)

        with self._db.transaction() as tx:
            row = tx.one('SELECT * FROM user_summon WHERE user_id = ? AND banner_id = ?', user_id, banner_id)
            level, exp, free_day, free_used = (row['level'], row['exp'], row['free_day'], row['free_count']) if row else (1, 0, 0, 0)

            if cost is Cost.CURRENCY:
                Inventory.consume(tx, user_id, banner.currency_id, banner.price * count)
            elif cost is Cost.TICKET:
                Inventory.consume(tx, user_id, banner.ticket_id, count)
            else:
                free_used = free_used if free_day == today.toordinal() else 0
                if free_used >= banner.free_limit:
                    raise GameError(Result.LIMIT_EXCEEDED)
                count, free_day, free_used = banner.free_count, today.toordinal(), free_used + 1

            results, level, exp = self._draw(banner, level, exp, count)
            Inventory.add(tx, user_id, Counter(results))
            tx.execute('INSERT OR REPLACE INTO user_summon VALUES (?, ?, ?, ?, ?, ?)', user_id, banner_id, level, exp, free_day, free_used)
            self._quests.occur(tx, user_id, Action.SUMMON, count, today, banner_id)
        return results, level, exp

    # 뽑는 도중 소환 레벨이 오르면 다음 뽑기부터 새 확률 적용
    def _draw(self, banner, level, exp, count):
        results = []
        for _ in range(count):
            table = banner.levels[level - 1]
            results.append(self._rng.choices(list(table.weights), list(table.weights.values()))[0])
            exp += 1
            if level < len(banner.levels) and exp >= table.next_exp:
                exp -= table.next_exp
                level += 1
        return results, level, exp
