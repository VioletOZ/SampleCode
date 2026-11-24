from dataclasses import dataclass

from common import GameError, Inventory, Result
from quest import Action

PRESET_SLOTS = 6


@dataclass(frozen=True)
class Skill:
    skill_id: int
    book_id: int
    equippable: bool
    level_costs: tuple


class SkillService:
    def __init__(self, db, skills, gold_id, quests):
        self._db = db
        self._skills = {skill.skill_id: skill for skill in skills}
        self._gold_id = gold_id
        self._quests = quests

    def open(self, user_id, skill_id):
        skill = self._get(skill_id)
        with self._db.transaction() as tx:
            if tx.one('SELECT 1 FROM user_skill WHERE user_id = ? AND skill_id = ?', user_id, skill_id):
                raise GameError(Result.ALREADY_DONE)
            Inventory.consume(tx, user_id, skill.book_id, 1)
            tx.execute('INSERT INTO user_skill VALUES (?, ?, 1)', user_id, skill_id)

    def level_up(self, user_id, skill_id, today):
        skill = self._get(skill_id)
        with self._db.transaction() as tx:
            row = tx.one('SELECT level FROM user_skill WHERE user_id = ? AND skill_id = ?', user_id, skill_id)
            if row is None:
                raise GameError(Result.NOT_OWNED)
            level = row['level']
            if level > len(skill.level_costs):
                raise GameError(Result.MAX_LEVEL)
            Inventory.consume(tx, user_id, self._gold_id, skill.level_costs[level - 1])
            tx.execute('UPDATE user_skill SET level = ? WHERE user_id = ? AND skill_id = ?', level + 1, user_id, skill_id)
            self._quests.occur(tx, user_id, Action.SKILL_UP, 1, today, skill_id)
        return level + 1

    # 0은 빈 슬롯
    def equip(self, user_id, slots):
        equipped = [skill_id for skill_id in slots if skill_id]
        if len(slots) != PRESET_SLOTS or len(equipped) != len(set(equipped)):
            raise GameError(Result.INVALID_REQUEST)
        if not all(self._get(skill_id).equippable for skill_id in equipped):
            raise GameError(Result.INVALID_REQUEST)

        with self._db.transaction() as tx:
            owned = {row['skill_id'] for row in tx.all('SELECT skill_id FROM user_skill WHERE user_id = ?', user_id)}
            if not owned.issuperset(equipped):
                raise GameError(Result.NOT_OWNED)
            tx.execute_many('INSERT OR REPLACE INTO user_preset VALUES (?, ?, ?)',
                            [(user_id, slot, skill_id) for slot, skill_id in enumerate(slots, 1)])

    def _get(self, skill_id):
        if skill_id not in self._skills:
            raise GameError(Result.NOT_FOUND)
        return self._skills[skill_id]
