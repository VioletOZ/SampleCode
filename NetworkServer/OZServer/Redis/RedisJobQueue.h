#pragma once
#include "Common/WorkQueue.h"
#include "Redis/RedisJob.h"

// GameLoop 스레드 -> Redis 워커 스레드. 결과는 DbResultQueue(GameLoop 콜백 큐)로 돌아온다
using RedisJobQueue = WorkQueue<RedisJob>;
