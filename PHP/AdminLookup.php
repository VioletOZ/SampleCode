<?php
declare(strict_types=1);

namespace GameSample;

require_once __DIR__ . '/GameException.php';

enum Format
{
    case Text;
    case Number;
    case Boolean;
    case Time;

    public function render(mixed $value): string
    {
        if ($value === null) {
            return '-';
        }
        return match ($this) {
            self::Text => (string) $value,
            self::Number => number_format((float) $value),
            self::Boolean => $value ? 'O' : 'X',
            self::Time => gmdate('Y-m-d H:i:s', (int) $value),
        };
    }
}

final class Column
{
    public function __construct(
        public readonly string $title,
        public readonly Format $format = Format::Text,
        public readonly bool $sortable = false,
    ) {}
}

final class LookupPage
{
    /** @param array<string, Column> $columns */
    public function __construct(
        public readonly string $table,
        public readonly array $columns,
        public readonly string $requiredRole = 'viewer',
    ) {}
}

// 조회 화면은 설정만 추가하면 만들어진다
final class AdminLookup
{
    private const ROLES = ['viewer' => 1, 'operator' => 2, 'admin' => 3];
    private const MAX_ROWS = 100;

    /** @param array<string, LookupPage> $pages */
    public function __construct(private readonly \PDO $db, private readonly array $pages) {}

    /** @return array<string, LookupPage> */
    public function pagesFor(string $role): array
    {
        return array_filter($this->pages, fn(LookupPage $page) => $this->allowed($page, $role));
    }

    /**
     * @param array<string, scalar> $filters
     * @return array{titles: string[], rows: string[][]}
     */
    public function search(string $name, string $role, array $filters = [], ?string $sort = null, bool $descending = false, int $page = 1): array
    {
        $config = $this->pages[$name] ?? throw new GameException('Unknown page');
        if (!$this->allowed($config, $role)) {
            throw new GameException('Permission denied');
        }

        $statement = $this->db->prepare($this->buildQuery($config, array_keys($filters), $sort, $descending, $page));
        $statement->execute(array_values($filters));

        $rows = [];
        while ($row = $statement->fetch(\PDO::FETCH_ASSOC)) {
            $cells = [];
            foreach ($config->columns as $field => $column) {
                $cells[] = $column->format->render($row[$field]);
            }
            $rows[] = $cells;
        }
        $titles = array_map(fn(Column $column) => $column->title, array_values($config->columns));
        return ['titles' => $titles, 'rows' => $rows];
    }

    // 컬럼 이름은 설정에 있는 것만 쓰고 값은 바인딩한다
    /** @param string[] $filters */
    private function buildQuery(LookupPage $config, array $filters, ?string $sort, bool $descending, int $page): string
    {
        foreach ($filters as $field) {
            if (!isset($config->columns[$field])) {
                throw new GameException('Invalid filter');
            }
        }
        if ($sort !== null && !(isset($config->columns[$sort]) && $config->columns[$sort]->sortable)) {
            throw new GameException('Invalid sort');
        }

        $query = 'SELECT ' . implode(', ', array_keys($config->columns)) . " FROM {$config->table}";
        if ($filters !== []) {
            $query .= ' WHERE ' . implode(' AND ', array_map(fn(string $field) => "{$field} = ?", $filters));
        }
        if ($sort !== null) {
            $query .= " ORDER BY {$sort} " . ($descending ? 'DESC' : 'ASC');
        }
        return $query . ' LIMIT ' . self::MAX_ROWS . ' OFFSET ' . ((max($page, 1) - 1) * self::MAX_ROWS);
    }

    private function allowed(LookupPage $page, string $role): bool
    {
        return (self::ROLES[$role] ?? 0) >= self::ROLES[$page->requiredRole];
    }
}
