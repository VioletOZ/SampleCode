#include "stdafx.h"
#include "Data/UserTable.h"
#include <utility>
#include <mysqlx/xdevapi.h>

std::optional<UserRow> UserTable::FindByUuid(const std::string& uuid)
{
    auto found = _session.sql("SELECT id, nickname FROM `user` WHERE uuid = ?").bind(uuid).execute();
    mysqlx::Row row = found.fetchOne();
    if (!row) return std::nullopt;
    return UserRow{ row[0].get<int64_t>(), row[1].get<std::string>() };   // 위 SELECT 컬럼 순서와 같게 유지(X DevAPI는 이름으로 못 꺼냄)
}

bool UserTable::NicknameExists(const std::string& nickname)
{
    auto taken = _session.sql("SELECT 1 FROM `user` WHERE nickname = ?").bind(nickname).execute();
    return static_cast<bool>(taken.fetchOne());
}

UserRow UserTable::Insert(const std::string& uuid, std::string nickname)
{
    auto created = _session.sql("INSERT INTO `user` (uuid, nickname, login_date) VALUES (?, ?, NOW())")
                       .bind(uuid, nickname).execute();
    return UserRow{ static_cast<int64_t>(created.getAutoIncrementValue()), std::move(nickname) };
}

// UPDATE ... WHERE id IN (?, ?, ...) 1건으로 묶음 전체 갱신
void UserTable::UpdateLoginDates(const DbWriteEntries& entries)
{
    std::string sql = "UPDATE `user` SET login_date = NOW() WHERE id IN (";
    sql.reserve(sql.size() + entries.size() * 2 + 1);
    for (size_t i = 0; i < entries.size(); ++i) sql += (i == 0) ? "?" : ",?";
    sql += ')';

    mysqlx::SqlStatement statement = _session.sql(sql);
    for (const auto& entry : entries) statement.bind(entry.first);
    statement.execute();
}
