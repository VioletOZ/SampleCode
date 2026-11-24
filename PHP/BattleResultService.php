<?php
declare(strict_types=1);

namespace GameSample;

require_once __DIR__ . '/GameException.php';

enum Outcome
{
    case Win;
    case Lose;
}

final class BattleReport
{
    public function __construct(
        public readonly string $battleId,
        public readonly Outcome $outcome,
        public readonly int $score,
        public readonly int $playSeconds,
    ) {}
}

final class Player
{
    public function __construct(
        public readonly int $id,
        public int $level = 1,
        public int $exp = 0,
        public int $gold = 0,
        public int $wins = 0,
        public int $losses = 0,
        public int $streak = 0,
        public int $bestScore = 0,
    ) {}
}

final class BattleRule
{
    /** @param array<int, int> $requiredExp 레벨 => 다음 레벨까지 필요한 경험치 */
    public function __construct(
        public readonly array $requiredExp,
        public readonly int $winExp,
        public readonly int $loseExp,
        public readonly int $scorePerGold,
        public readonly int $levelUpGold,
        public readonly int $maxScorePerSecond,
        public readonly int $minPlaySeconds,
    ) {}

    public function maxLevel(): int
    {
        return count($this->requiredExp) + 1;
    }
}

interface BattleRepository
{
    /** 시작할 때 저장한 전투를 한 번만 꺼낸다. 이미 정산했거나 없으면 null */
    public function takeStartedAt(int $userId, string $battleId): ?int;

    public function save(Player $player): void;

    public function updateRanking(int $userId, int $score): void;
}

final class BattleResultService
{
    public function __construct(private readonly BattleRepository $repository, private readonly BattleRule $rule) {}

    /** @return array{exp: int, gold: int, levelUps: int} */
    public function finish(Player $player, BattleReport $report, int $now): array
    {
        $startedAt = $this->repository->takeStartedAt($player->id, $report->battleId)
            ?? throw new GameException('Battle not found or already finished');
        $this->validate($report, $now - $startedAt);

        $win = $report->outcome === Outcome::Win;
        $exp = $win ? $this->rule->winExp : $this->rule->loseExp;
        $levelUps = $this->addExp($player, $exp);
        $gold = intdiv($report->score, $this->rule->scorePerGold) + $levelUps * $this->rule->levelUpGold;

        $isBest = $report->score > $player->bestScore;
        $player->gold += $gold;
        $player->wins += $win ? 1 : 0;
        $player->losses += $win ? 0 : 1;
        $player->streak = $win ? $player->streak + 1 : 0;
        $player->bestScore = max($player->bestScore, $report->score);

        $this->repository->save($player);
        if ($isBest) {
            $this->repository->updateRanking($player->id, $report->score);
        }
        return ['exp' => $exp, 'gold' => $gold, 'levelUps' => $levelUps];
    }

    // 클라이언트가 보낸 시간은 서버가 잰 시간을 넘을 수 없다
    private function validate(BattleReport $report, int $elapsed): void
    {
        if ($report->playSeconds < $this->rule->minPlaySeconds || $report->playSeconds > $elapsed) {
            throw new GameException('Invalid play time');
        }
        if ($report->score < 0 || $report->score > $report->playSeconds * $this->rule->maxScorePerSecond) {
            throw new GameException('Invalid score');
        }
    }

    private function addExp(Player $player, int $exp): int
    {
        $before = $player->level;
        $maxLevel = $this->rule->maxLevel();
        $player->exp += $exp;
        while ($player->level < $maxLevel && $player->exp >= $this->rule->requiredExp[$player->level]) {
            $player->exp -= $this->rule->requiredExp[$player->level];
            ++$player->level;
        }
        if ($player->level >= $maxLevel) {
            $player->exp = 0;
        }
        return $player->level - $before;
    }
}
