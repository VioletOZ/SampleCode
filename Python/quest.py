from dataclasses import dataclass
from datetime import date
from enum import Enum

from common import GameError, Inventory, Result


class Cycle(Enum):
    ONCE = 0
    DAILY = 1
    WEEKLY = 2


class Action(Enum):
    SUMMON = 1
    SKILL_UP = 2
    EVENT_PLAY = 3


@dataclass(frozen=True)
class Quest:
    quest_id: int
    cycle: Cycle
    action: Action
    goal: int
    reward: dict
    target: int = 0


class QuestService:
    def __init__(self, db, quests):
        self._db = db
        self._quests = {quest.quest_id: quest for quest in quests}

    @staticmethod
    def period(cycle, today: date):
        if cycle is Cycle.DAILY:
            return today.toordinal()
        if cycle is Cycle.WEEKLY:
            return (today.toordinal() - 1) // 7
        return 0

    # 게임 행동 1건을 조건이 맞는 모든 퀘스트에 반영. 기간이 바뀐 퀘스트는 여기서 초기화
    def occur(self, tx, user_id, action, count, today, target=0):
        rows = [(user_id, quest.quest_id, self.period(quest.cycle, today), min(count, quest.goal), quest.goal)
                for quest in self._quests.values()
                if quest.action is action and quest.target in (0, target) and count > 0]
        tx.execute_many('INSERT INTO user_quest VALUES (?1, ?2, ?3, ?4, 0) ON CONFLICT (user_id, quest_id) DO UPDATE SET '
                        'progress = CASE WHEN period = excluded.period THEN MIN(progress + excluded.progress, ?5) ELSE excluded.progress END, '
                        'rewarded = CASE WHEN period = excluded.period THEN rewarded ELSE 0 END, '
                        'period = excluded.period', rows)

    def progress(self, user_id, today):
        with self._db.transaction() as tx:
            rows = {row['quest_id']: row for row in tx.all('SELECT * FROM user_quest WHERE user_id = ?', user_id)}
        result = {}
        for quest_id, quest in self._quests.items():
            row = rows.get(quest_id)
            active = row is not None and row['period'] == self.period(quest.cycle, today)
            result[quest_id] = (row['progress'], bool(row['rewarded'])) if active else (0, False)
        return result

    def claim(self, user_id, quest_id, today):
        quest = self._quests.get(quest_id)
        if quest is None:
            raise GameError(Result.NOT_FOUND)

        with self._db.transaction() as tx:
            period = self.period(quest.cycle, today)
            row = tx.one('SELECT * FROM user_quest WHERE user_id = ? AND quest_id = ? AND period = ?', user_id, quest_id, period)
            if row is None or row['progress'] < quest.goal:
                raise GameError(Result.NOT_COMPLETE)
            if row['rewarded']:
                raise GameError(Result.ALREADY_DONE)
            tx.execute('UPDATE user_quest SET rewarded = 1 WHERE user_id = ? AND quest_id = ?', user_id, quest_id)
            Inventory.add(tx, user_id, quest.reward)
        return quest.reward
