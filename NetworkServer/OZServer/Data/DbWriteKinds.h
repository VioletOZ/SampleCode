#pragma once
#include "Db/DbWriteBuffer.h"

// 실제 지연 쓰기 종류 표(주기, 개수, 묶음 쓰기 함수). SQL은 각 테이블의 DAO(Data/*Table.cpp)에 둔다
const DbWriteKindSpecs& GetDbWriteKindSpecs();
