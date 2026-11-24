<?php
declare(strict_types=1);

namespace GameSample;

require_once __DIR__ . '/GameException.php';

final class CardInfo
{
    public function __construct(
        public readonly int $id,
        public readonly int $maxEnchant,
        public readonly int $maxExp,
        public readonly int $duplicateExp,
        public readonly int $duplicateGold,
        public readonly ?int $promotesTo = null,
    ) {}
}

final class Card
{
    public function __construct(
        public readonly int $id,
        public readonly int $enchant = 0,
        public readonly int $exp = 0,
    ) {}
}

final class CardResult
{
    /** @param int[] $removed */
    public function __construct(
        public readonly ?Card $card,
        public readonly int $gold,
        public readonly array $removed,
    ) {}
}

interface CardRepository
{
    public function find(int $userId, int $cardId): ?Card;

    public function equippedCardId(int $userId): ?int;

    /** 제거, 저장, 장착, 골드를 한 트랜잭션으로 처리 */
    public function apply(int $userId, CardResult $result, ?int $equipCardId): void;
}

final class CardService
{
    /** @param array<int, CardInfo> $cards */
    public function __construct(private readonly CardRepository $repository, private readonly array $cards) {}

    public function grant(int $userId, int $cardId): CardResult
    {
        $result = $this->resolve($userId, $cardId);
        $equipped = $this->repository->equippedCardId($userId);
        $equipCardId = in_array($equipped, $result->removed, true) ? $result->card?->id : null;

        $this->repository->apply($userId, $result, $equipCardId);
        return $result;
    }

    // 중복 카드: 강화 -> 한도면 상위 카드로 승급 -> 최상위면 경험치 -> 최대면 골드
    private function resolve(int $userId, int $cardId): CardResult
    {
        $removed = [];
        $exp = 0;
        while (true) {
            if (in_array($cardId, $removed, true)) {
                throw new GameException("Promotion loop: {$cardId}");
            }
            $info = $this->cards[$cardId] ?? throw new GameException("Unknown card: {$cardId}");
            $owned = $this->repository->find($userId, $cardId);

            if ($owned === null) {
                return new CardResult(new Card($cardId, 0, min($exp, $info->maxExp)), 0, $removed);
            }
            if ($owned->enchant < $info->maxEnchant) {
                return new CardResult(new Card($cardId, $owned->enchant + 1, min($owned->exp + $exp, $info->maxExp)), 0, $removed);
            }
            if ($info->promotesTo === null) {
                return $this->convert($owned, $info, $exp, $removed);
            }
            $removed[] = $cardId;
            $exp += $owned->exp;
            $cardId = $info->promotesTo;
        }
    }

    /** @param int[] $removed */
    private function convert(Card $owned, CardInfo $info, int $exp, array $removed): CardResult
    {
        if ($owned->exp >= $info->maxExp) {
            return new CardResult($owned, $info->duplicateGold, $removed);
        }
        $total = min($owned->exp + $exp + $info->duplicateExp, $info->maxExp);
        return new CardResult(new Card($owned->id, $owned->enchant, $total), 0, $removed);
    }
}
