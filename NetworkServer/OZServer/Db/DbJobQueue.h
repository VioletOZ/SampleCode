#pragma once
#include "Common/WorkQueue.h"
#include "Db/DbJob.h"

// GameLoop 스레드 -> DB 워커 스레드(풀)로 작업을 넘기는 큐(구현은 공용 WorkQueue — Redis 워커와 같은 코드)
using DbJobQueue = WorkQueue<DbJob>;
