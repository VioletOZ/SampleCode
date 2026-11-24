#include "stdafx.h"
#include "Data/DbWriteKinds.h"
#include "Data/UserTable.h"

namespace {

const DbWriteKindSpecs SPECS = { {
    // 종류 순서 = WriteKind 순서
    { "login_date", std::chrono::seconds(1), 1000,
      [](mysqlx::Session& session, const DbWriteEntries& entries) { UserTable(session).UpdateLoginDates(entries); } },
} };

}

const DbWriteKindSpecs& GetDbWriteKindSpecs()
{
    return SPECS;
}
