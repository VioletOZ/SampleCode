# SampleCode

Session::~Session()
{
	if (GetLastQueuedWorld() && !GameAccountID().IsNull())
	{
		auto gameAccountID = GameAccountID();
		auto queuedWrold = GetLastQueuedWorld();

		if (m_IsEnterWorld && !g_LobbySrvMgr->IsClose())
		{
			// 서버 오픈 후 바로 밀고 들어올 경우 과도하게 접속이 가능해짐 (월드 동접 수 업데이트 시간이 걸림 )
			// 월드 진입일 경우 조금 늦게 대기열 삭제
			auto pTask = std::make_shared<os::DelayedTask>([gameAccountID, queuedWrold]()
			{
				g_Connection.PlayerWaitDel(gameAccountID, queuedWrold, false);
			});
			pTask->AddTimer(5 * 1000);
		}
		else
		{
			g_Connection.PlayerWaitDel(gameAccountID, queuedWrold);
		}
	}
}


// 대기열 관리
	auto pWorld = g_Worlds->Find(request->world_number());
	if (!pWorld)
	{
		request->set_result(WORLD_SERVER_NOTCONNECT);
		SendPacket(pSocket, WorldCharacterList, *request);
		return;
	}

Session::Ptr pSession = Session::Mgr()->Find(sessionID);

auto pList = g_Waiting->MutableList(request->world_number());
		pos = pList->FindPos(pSession->GameAccountID());

		if ((pSession->GetLastQueuedWorld() == request->world_number())
			&& (pSession->GetLastQueuedPos() != std::numeric_limits<size_t>::max())
			&& (pos == std::numeric_limits<size_t>::max()))
		{
			// 대기열 추가는 되었는데 서버에서 갱신이 안되었을 경우 추가된 값 사용
			pos = pSession->GetLastQueuedPos();
		}

// 대기열 무조건 추가
		if (pos == std::numeric_limits<size_t>::max())
		{
			// 다른 월드 대기열이 있으면 해제
			if (pSession->GetLastQueuedWorld())
			{
				g_Connection.PlayerWaitDel(pSession->GameAccountID(), pSession->GetLastQueuedWorld());
			}

			g_Connection.PlayerWaitAdd(pSession->GameAccountID(), request->world_number())
				.then([sessionID, request](auto future) mutable
			{
				Session::Ptr pSession = Session::Mgr()->Find(sessionID);
				if (!pSession || pSession->IsExpired())
				{
					return;
				}

				try
				{
					auto msg = future.get();
					if (msg.header().status_code() == OK)
					{
						if (msg.pos() != std::numeric_limits<int64>::max())
						{
							pSession->SetLastQueuedPos(msg.pos());
							pSession->SetLastQueuedWorld(request->world_number());

							// 다음 스텝
							_onWorldCharacterGetWaitList(sessionID, msg.pos(), request);
							return;
						}
					}
				}
				catch (const std::exception&)
				{
				}

				request->set_queue_number(std::numeric_limits<int>::max());
				request->set_result(SUCCESS);
				pSession->SendPacket(WorldCharacterList, *request);
			});

			return;
		}


// 대기열이 정상적으로 추가 안되었으면 문제있는거임
		if (pos == std::numeric_limits<size_t>::max())
		{
			request->set_queue_number(std::numeric_limits<int>::max());
			request->set_result(SUCCESS);
			pSession->SendPacket(WorldCharacterList, *request);
			return;
		}

		// 대기열 계산
		auto maxCount = pWorld->MaxConcurrentPlayers();
		auto curCount = pWorld->ConcurrentPlayers();
		auto diff = maxCount - curCount;

		auto lobby_login_count = g_ConfigServer.Config().GetMutableInt(L"LobbyLoginCount", 100);
		auto login_count = (maxCount > lobby_login_count) ? lobby_login_count : maxCount;

		if ((pos <= diff) && (diff > 0) && (pos <= login_count))
		{
			// 진입 허가
		}
		else
		{
			//진입 실패하면 대기..
			request->set_queue_number(static_cast<int>(pos));
			request->set_result(SUCCESS);
			pSession->SendPacket(WorldCharacterList, *request);
			return;
		}

	// 진입허가가 되면 계정정보 얻어오면서 로그인로직탐
