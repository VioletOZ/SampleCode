//====================================================================================================================================
	// Step 3-3 : 파일럿(Pilot)에 대한 업그레이드 가능 여부 확인 및 업그레이드
	//====================================================================================================================================
	
	else if( $UpgradeItemType == "Pilot" )		// 파일럿 업그레이드다.
	{
		$CurrentPilotageLevel	= $PlayerObject->GetPlayerData()->_Inven['PilotageLevel_'.$UpgradeItemId];
		$CurrentEvadeLevel		= $PlayerObject->GetPlayerData()->_Inven['EvadeLevel_'.$UpgradeItemId];
		$CurItem				= $ItemManager->GetItem($UpgradeItemId);
		$MaxUpgradeLevel		= CONFIG_GAME::MAX_PILOTEVADE_LEVEL;// 현재 최대 레벨은 21까지다. //(int)$PlayerObject->GetLevel();	
	
/*		if( $UpgradeItemStats == "PT" && (int)$CurrentPilotageLevel == $MaxUpgradeLevel )
		{
			// 이미 최고 최대 스탯을 찍었다.
			RaiseError('ERR_UPGRADE_ALREADY_MAX');
			return false;
		}
*/
		if( $UpgradeItemStats == "EV" && (int)$CurrentEvadeLevel == $MaxUpgradeLevel )	
		{
			// 이미 최고 최대 스탯을 찍었다.
			RaiseError('ERR_UPGRADE_ALREADY_MAX');
			return false;
		}

		if( $UpgradeItemStats == "EV" )	// 회피 기동 업그레이드면
		{
			$NextLevel	= $CurrentEvadeLevel + 1;
			$EvadeInfoIndex = 9000300 + $NextLevel;
			
			$EvadeInfo	= $ItemManager->GetItem($EvadeInfoIndex);
			$RealPrice	= $EvadeInfo['DgPctUpGold'];			// 진짜 소모되는 Gold량

			if( $NextLevel != $UpgradeItemStatsNextLevel )
			{
				RaiseError('ERR_UPGRADE_MISMATCH_STATS');
				return false;
			}

			/*
			if( (int)$RealPrice != (int)$UpgradeItemPrice )
			{
				RaiseError('ERR_UPGRADE_MISMATCH_PRICE');
				return false;
			}
			*/
			if( $PlayerObject->GetGold() < (int)$RealPrice )
			{
				RaiseError('ERR_NOTENOUGH_GOLD');
				return false;
			}

			if( $PlayerObject->UpgradePilotEvadeLevel($UpgradeItemId,$NextLevel, $RealPrice) == false )
			{
				return false;
			}
		}
		else
		{
			RaiseError('ERR_NOTSUPPORT_FUNCTION');
			return false;
		}
	}
	else
	{
		RaiseError('ERR_NOTSUPPORT_FUNCTION');
		return false;
	}
	

	//====================================================================================================================================
	// Step 4 : 업그레이드 결과와 플레이어 인벤 목록을 전송
	//====================================================================================================================================
	AddResult('Type', $UpgradeItemType );
	AddResult('RemainGold', $PlayerObject->GetGold() );
	AddResult('ItemIdx', $UpgradeItemId );
	AddResult('Inven', ConvertPlayerInvenInfo($PlayerObject) );
	AddResult('Equip', ConvertPlayerEquipInfo($PlayerObject) );
	//====================================================================================================================================
	// 성공 : 여기까지 왔으면 성공 한 것이다.
	//====================================================================================================================================
	RaiseError('SUCCESS');	// 에러가 없다. 잘 수행되었다. SUCCESS를 세팅하면 된다.
	return true;
}




function OPERATE_TUTORIALCHECK()
{
	TRACE('====================================Start Tutorial Fighter, Pilot Check======================================');

	if( CONFIG_DEBUG::ENABLE_VALIDATION_CHECK == 1 )
	{
		//====================================================================================================================================
		// Step 1 : 인자의 유효성 체크
		//====================================================================================================================================
		
		if( IsValidRequestArguments() == false )
		{
			// TODO ERROR LOG : Arguments 가 !! 틀렸다. 
			// TODO ADD_DB_LOG
			RaiseError('ERR_VARIABLE');
			return false;
		}
	}	
	
	//====================================================================================================================================
	// Step 2 : 플레이어 객체 얻어 인증해보기
	//====================================================================================================================================
	$PlayerObject = GetPlayerObject();
	$nRet = GetAuthModule()->CheckAuth( GetUserId(), GetClientAccessToken(), GetClientProtocol() );
	if ( $nRet != GetErrorCode('SUCCESS') )
	{
		// 뭔가 오류가 발생했다. 
		// 오류의 종류는 GetAuthModule()->CheckAuth 에서 RaiseError 하고 있으니 별도의 에러 코드는 뱉어내지 않는다.
		return false;
	}

	TRACE("User ID : " .GetUserID());

	//====================================================================================================================================
	// Step 3 : 튜토리얼 기체, 파일럿 체크
	//====================================================================================================================================
	$FighterCheck = GetTutorialFighterCheck();
	$PilotCheck = GetTutorialPilotCheck();
	$IntroCheck = GetTutorialIntroCheck();
	

	TRACE("====== 기체체크 : $FighterCheck ==== 파일럿 체크 : $PilotCheck === 인트로 체크 : $IntroCheck ====");
	
	if($PlayerObject->TutorialCheck($FighterCheck, $PilotCheck, $IntroCheck) == false)
	{
		TRACE("FighterCheck : $FighterCheck === PilotCheck : $PilotCheck === IntroCheck : $IntroCheck");
		return false;
	}
	
	//====================================================================================================================================
	// 성공 : 여기까지 왔으면 성공 한 것이다.
	//====================================================================================================================================
	RaiseError('SUCCESS');	// 에러가 없다. 잘 수행되었다. SUCCESS를 세팅하면 된다.
	return true;
}

function OPERATE_TUTORIALPROGRESS()
{
	TRACE("================ Tutorial Progress ===================");
	if( CONFIG_DEBUG::ENABLE_VALIDATION_CHECK == 1 )
	{
		//====================================================================================================================================
		// Step 1 : 인자의 유효성 체크
		//====================================================================================================================================
		
		if( IsValidRequestArguments() == false )
		{
			// TODO ERROR LOG : Arguments 가 !! 틀렸다. 
			// TODO ADD_DB_LOG
			RaiseError('ERR_VARIABLE');
			return false;
		}
	}	
	
	//====================================================================================================================================
	// Step 2 : 플레이어 객체 얻어 인증해보기
	//====================================================================================================================================
	$PlayerObject = GetPlayerObject();
	$nRet = GetAuthModule()->CheckAuth( GetUserId(), GetClientAccessToken(), GetClientProtocol() );
	if ( $nRet != GetErrorCode('SUCCESS') )
	{
		// 뭔가 오류가 발생했다. 
		// 오류의 종류는 GetAuthModule()->CheckAuth 에서 RaiseError 하고 있으니 별도의 에러 코드는 뱉어내지 않는다.
		TRACE("=============== Tutorial Player Auth Error =================");
		return false;
	}

	//TRACE("RegDate : $PlayerData['RegDateTime'];
	//====================================================================================================================================
	// Step 3 : 튜토리얼 넘버링 및 
	//====================================================================================================================================
	$Category = (int) GetTutorialStepCategory();
	$Index = (int) GetTutorialStepIndex();
	$TutorialStepTime = GetNowTime();

	TRACE("====================Category : $Category - Index : $Index===================");
	if(((($Category > CHAPTER1_BEGIN) && ($Category <= CHAPTER1_END)) || (($Category >= CHAPTER2_BEGIN) && ($Category <= CHAPTER2_END))) == false)
	{
		TRACE('Tutorial Category Error ' . $Category);
		return false;
	}	
	
	if($PlayerObject->UpdateTutorialStep($Category, $Index, $TutorialStepTime) == false)
	{
		TRACE('Tutorial Update Fail' . $Category . ':' . $Index);
		return false;
	}

	$TutorialFinishType = GetTutorialFinishType();	
	$FinishType = GetFinishType();
		
	TRACE("===================튜토리얼 종료타입 : $TutorialFinishType ====================");


	if((SC($TutorialFinishType, 'NR') == true && $Index == 1 ) && 
		($Category == CHAPTER1_CON || $Category == CHAPTER2_END))		//보상이 없음 스테이지 진행도만 증가
	{
		$StageIdx			= 100001;
		
		TRACE("====================== 스테이지 : $StageIdx ======================");
		
		if($PlayerObject->TutorialStage($StageIdx) == false)
		{
			TRACE("Tutorial Stage Fail... FinishType : $TutorialFinishType");
			return false;
		}			
	}
	else if(SC($TutorialFinishType, 'LUP') == true)	// 강제 레벨업
	{
		if($PlayerObject->TutorialForceLv($TutorialStepTime) == false)
		{
			TRACE("Tutorial Force LevelUp FAIL.. FinishType : $TutorialFinishType");
			return false;
		}
	}
		
	
	//====================================================================================================================================
	// 성공 : 여기까지 왔으면 성공 한 것이다.
	//====================================================================================================================================
	RaiseError('SUCCESS');	// 에러가 없다. 잘 수행되었다. SUCCESS를 세팅하면 된다.
	return true;
}

public function GivePilotCard(&$PlayerObject, $PCardIndex)
	{
		$TargetCardIndex = $PCardIndex;

		$res_unset = array();
		$res_set = array();

		while( true )
		{
			$IndexName = 'PCard_'.$TargetCardIndex;
			$ExpIndexName = 'PCardExp_'.$TargetCardIndex;

			// 처음 획득 인가?
			if( isset($PlayerObject->_PlayerData->_Inven[$IndexName]) == false )
			{
				$res_set['card'] = array('idx' => $TargetCardIndex, 'enchant' => 0, 'exp' => 0, 'lupxpb' => 0, 'lupxpe' => 0, 'luplvb' => 1, 'luplve' => 1, 'enclvb' => 0, 'enclve' => 0);
				break;
			}

			// 카드 정보 얻기
			$BeforeUpgradeNum = (int)$PlayerObject->_PlayerData->_Inven[$IndexName];
			$BeforeExp = 0;
			if( isset($PlayerObject->_PlayerData->_Inven[$ExpIndexName]) )
				$BeforeExp = (int)$PlayerObject->_PlayerData->_Inven[$ExpIndexName];
			$BeforeLevel = (int)$this->CalcPCardLevel($TargetCardIndex, $BeforeExp);

			$CardItemData = GetItemManager()->GetItem($TargetCardIndex);
			if( is_null($CardItemData) || isset($CardItemData['PcardUpgradeNum']) == false || isset($CardItemData['PcardMaxLV']) == false || SC($CardItemData['Type'],'PCard') == false )
			{
				// 에러
				return false;
			}

			$MaxUpgradeNum = (int)$CardItemData['PcardUpgradeNum'];
			$MaxLevel = (int)$CardItemData['PcardMaxLV'];

			// 강화가 불가능한가?
			if( $MaxUpgradeNum <= 0 )
			{
				// MaxLevel 인가
				if( $BeforeLevel >= $MaxLevel )
				{
					$res_set['gold'] = $CardItemData['AlterGold'];
					$res_set['goldtidx'] = $TargetCardIndex;
				}
				else
				{
					$AfterExp = $BeforeExp+(int)$CardItemData['PCardReturnExp'];
					$AfterLevel = (int)$this->CalcPCardLevel($TargetCardIndex, $AfterExp);
					$res_set['card'] = array('idx' => $TargetCardIndex, 'enchant' => 0, 'exp' => $AfterExp, 'lupxpb' => $BeforeExp, 'lupxpe' => $AfterExp, 'luplvb' => $BeforeLevel, 'luplve' => $AfterLevel, 'enclvb' => 0, 'enclve' => 0);
				}

				break;
			}

			// 기존에 있는 카드를 강화 했을 때 강화등급 max 보다 작은가
			if( $BeforeUpgradeNum + 1 < $MaxUpgradeNum )
			{
				$res_set['card'] = array('idx' => $TargetCardIndex, 'enchant' => $BeforeUpgradeNum + 1, 'exp' => $BeforeExp, 'lupxpb' => $BeforeExp, 'lupxpe' => $BeforeExp, 'luplvb' => $BeforeLevel, 'luplve' => $BeforeLevel, 'enclvb' => $BeforeUpgradeNum, 'enclve' => $BeforeUpgradeNum + 1);
				break;
			}

			// 상위 등급의 카드가 있는가?
			if( isset($CardItemData['PCardUpgrade']) == false )
			{
				// 에러
				return false;
			}

			// 타겟이 바뀌었음
			unset($res_set);
			$res_set = array();

			$res_unset[] = $TargetCardIndex;

			$TargetCardIndex = (int)$CardItemData['PCardUpgrade'];
		}

		$bChangeEquip = false;

		if( count($res_unset) > 0 )
		{
			$key['_id'] = trim($PlayerObject->_PlayerData->_id);
			$unset_datas = array();

			foreach( $res_unset as $cardidx )
			{
				$unset_datas['$unset']['PCard_'.$cardidx] = 0;
				$unset_datas['$unset']['PCardExp_'.$cardidx] = 0;
			}

			MDB()->UpdateData(COLL::ITEM, $key, $unset_datas);

			foreach( $res_unset as $cardidx )
			{
				if( (int)$PlayerObject->_PlayerData->_EquipPCard == (int)$cardidx )
				{
					$bChangeEquip = true;
				}

				unset( $PlayerObject->_PlayerData->_Inven['PCard_'.$cardidx] );
				unset( $PlayerObject->_PlayerData->_Inven['PCardExp_'.$cardidx] );
			}

			$out['DIDX'] = implode('#', $res_unset);
		}

		if( count($res_set) > 0 )
		{
			$key['_id'] = trim($PlayerObject->_PlayerData->_id);

			if( isset($res_set['card']) )
			{
				$cardidx = (int)$res_set['card']['idx'];
				$set_datas['$set']['PCard_'.$cardidx] = (int)$res_set['card']['enchant'];
				$set_datas['$set']['PCardExp_'.$cardidx] = (int)$res_set['card']['exp'];

				MDB()->UpdateData(COLL::ITEM, $key, $set_datas);

				$PlayerObject->_PlayerData->_Inven['PCard_'.$cardidx] = (int)$res_set['card']['enchant'];
				$PlayerObject->_PlayerData->_Inven['PCardExp_'.$cardidx] = (int)$res_set['card']['exp'];

				$out['TIDX'] = $cardidx;
				$out['LUPXPB'] = (int)$res_set['card']['lupxpb'];
				$out['LUPXPE'] = (int)$res_set['card']['lupxpe'];
				$out['LUPLVB'] = (int)$res_set['card']['luplvb'];
				$out['LUPLVE'] = (int)$res_set['card']['luplve'];
				$out['ENCLVB'] = (int)$res_set['card']['enclvb'];
				$out['ENCLVE'] = (int)$res_set['card']['enclve'];

				if( $out['LUPLVB'] != $out['LUPLVE'] )
				{
					$PCardLevelData = $this->CalcPCardLevelData($out['TIDX'], $out['LUPLVE']);
					if( is_array($PCardLevelData) && isset($PCardLevelData['PCardLvRewardIndex']) && isset($PCardLevelData['PCardLvRewardNum']) )
					{
						$RewardIndex = is_numeric($PCardLevelData['PCardLvRewardIndex']) ? (int)$PCardLevelData['PCardLvRewardIndex'] : $PCardLevelData['PCardLvRewardIndex'];
						$RewardAmount = (int)$PCardLevelData['PCardLvRewardNum'];
						if( is_numeric($PCardLevelData['PCardLvRewardIndex']) == false || (int)$PCardLevelData['PCardLvRewardIndex'] > 0 )
							$PlayerObject->SendMessageByAdmin( trim($PlayerObject->_PlayerData->_id), $RewardIndex, $RewardAmount, GetStringManager()->GetString(26053) );
					}
				}

				if( $bChangeEquip )
				{
					$PlayerObject->SetEquipPilotCard($cardidx);

					AddResult('Equip', ConvertPlayerEquipInfo($PlayerObject) );
				}
			}

			if( isset($res_set['gold']) )
			{
				$gold_data['$inc']['Gold'] = (int)$res_set['gold'];
				MDB()->UpdateData(COLL::PLAYER, $key, $gold_data);
				$PlayerObject->_PlayerData->_Gold += (int)$res_set['gold'];

				$out['DUPG'] = (int)$res_set['gold'];

				if( isset($out['TIDX']) == false )
				{
					$out['TIDX'] = (int)$res_set['goldtidx'];
				}
			}
		}

		$out['IDX'] = $PCardIndex;

		return $out;
	}





	public function FinishGame($VicType, $UseItemArray, $Score, $Exp, $Gold, $PlayTime, &$Ref_RewardCVCount)
    {
        $Medal = 0;
        // 아이템 사용목록 부터 처리하자. 이걸 db에 한번에 밀어 넣는 방법을 강구해야 한다.
        // 일단 급하니 하나씩이라도 처리.
        // 1 단계: 아이템 소모
        if ($UseItemArray)
        {
            foreach ($UseItemArray as $ItemIdx => $Count)
            {
                $ItemIdx = (int) $ItemIdx;

                // 실제 아이템인지 보자.
                $ItemOrgData = GetItemManager()->GetItem($ItemIdx);
                if (isset($ItemOrgData) == false)
                {
                    //RaiseError('ERR_WRONG_ITEMIDX');
                    WRITE_FINISH_FAIL('GameFinish Fail UID: ' . GetUserId() . ', Error Code: RBFinishGame WRONG ITEMIDX: ' . $ItemIdx);
                    continue;
                    //return false;
                }

                if ($ItemIdx != 5000000) // 기본 미사일이 아니라면
                {
                    $SelectedItem = GetItemManager()->GetItem((int) $ItemIdx);
                    $MaxAmount    = isset($SelectedItem['MaxAmount']) ? $SelectedItem['MaxAmount'] : 1;
                    $this->ChangeItemAmount($ItemIdx, $MaxAmount, -$Count);
                }
            }
        }
        TRACE("Finish : S($Score), E($Exp), G($Gold)");
        ////////////////////////////////////////////////////////////////////////////////////////////////
        // 이벤트 적용 여부 부터 처리해본다.
        ////////////////////////////////////////////////////////////////////////////////////////////////
        // 피크타임 이벤트
        $EventManager = GetEventManager(false); // 피크타임 이벤트만 로드한다.
        $EventFlag    = $EventManager->IsPeakTimeEventApply();

        $RewardType   = '';
        $RewardAmount = 0;

        if ($EventFlag == 1 || $EventFlag == 2)
        {
            $RewardType   = $EventManager->GetPeakTimeEventRewardType();
            $RewardAmount = (int) $EventManager->GetPeakTimeEventRewardAmount();

            TRACE('피크타임 이벤트 적용 됨:' . $RewardType . ", " . $RewardAmount);
        }

        if ($EventFlag != false && $RewardType == 'G')
        {
            $NewGold = (int) $Gold + (int) (((int) $Gold / 100) * ((int) $RewardAmount));
            TRACE('피크타임 이벤트 골드 증가량 : ' . $Gold . ' -> ' . $NewGold);
            $Gold    = (int) $NewGold;
        }
        else if ($EventFlag != false && $RewardType == 'S')
        {
            $NewScore = (int) $Score + (int) (((int) $Score / 100) * ((int) $RewardAmount));
            TRACE('피크타임 이벤트 스코어 증가량 : ' . $Score . ' -> ' . $NewScore);
            $Score    = (int) $NewScore;
        }
        else if ($EventFlag != false && $RewardType == 'X')
        {
            $NewExp = (int) $Exp + (int) (((int) $Exp / 100) * (int) $RewardAmount);
            TRACE('피크타임 이벤트 경험치 증가량 : ' . $Exp . ' -> ' . $NewExp);
            $Exp    = (int) $NewExp;
        }
        ////////////////////////////////////////////////////////////////////////////////////////////////

        TRACE("Event Finish : S($Score), E($Exp), G($Gold)");


        // 2 단계: 유저 정보 갱신
        $currentTime = GetNowTime();
        $key['_id']  = GetUserId();
        $data        = array();

        //$data['$inc']['Gold'] = (int)$Gold;
        // 2-1 단계: 최근 획득 SCORE를 저장.
        $data['$set']['RecentScore']         = (int) $Score;
        $data['$set']['RecentScoreDateTime'] = (int) $currentTime;

        // 2-2 단계: 지금 획득한 EXP가 TOP보다 크면 TOP 갱신해준다.
        if ($this->_PlayerData->_TopScore < (int) $Score)
        {
            $data['$set']['TopScore']         = (int) $Score;
            $data['$set']['TopScoreDateTime'] = (int) $currentTime;
        }

        // 이번주 기록이면 비교해서 TopScore를 넣어주고..아니면 이번주 최초 기록이므로 Top을 넣어준다.
        $NextResetTime = GetNextResetTime();
        $ThisWeekStart = $NextResetTime - (3600 * 24 * 7);

        // 2-3 단계: 주간 누적 Score
        if ($this->_PlayerData->_WeeklyAccumulateScoreDateTime > $ThisWeekStart) // 이번주 기록에 누적해야 되면
        {
            // 주간 최고 기록이 바뀌면
            if ((int) $Score > $this->_PlayerData->_WeeklyTopScore)     // 이번주 최고 기록보다 크면
            {
                $data['$set']['WeeklyTopScore']         = (int) $Score;
                $data['$set']['WeeklyTopScoreDateTime'] = (int) $currentTime;
            }
            // 기존 기록에 누적시켜준다.
            $data['$inc']['WeeklyAccumulateScore']         = (int) $Score;
            $data['$set']['WeeklyAccumulateScoreDateTime'] = (int) $currentTime;
        }
        else
        {
            // 이번주 전 기록이라면 덮어 쓴다.
            $data['$set']['WeeklyTopScore']                = (int) $Score;
            $data['$set']['WeeklyTopScoreDateTime']        = (int) $currentTime;
            $data['$set']['WeeklyAccumulateScore']         = (int) $Score;
            $data['$set']['WeeklyAccumulateScoreDateTime'] = (int) $currentTime;
        }

        // 2-4 단계: 플레이 전적을 관리하자.
        $data['$inc']['TotalPlayCount']        = 1;     // 전체 플레이 카운트 증가 시키고
        $data['$inc']['TotalPlayTime']         = (int) $PlayTime;    // 전체 플레이 시간 증가 시키고 (ms)
        $data['$set']['TotalPlayTimeDateTime'] = (int) $currentTime; // 전체 플레이 시간 기록 시간을 기록한다! ..; 이건 필요 없는데..그냥..

	    TRACE("========================================================== Play count reference information ======================================================");
	    TRACE("WeeklyPlayCountDateTime : " . date('Ymd H:i:s', $this->_PlayerData->_WeeklyPlayCountDateTime) . "(" . var_export($this->_PlayerData->_WeeklyPlayCountDateTime, true) . ") ThisWeekStart : " . date('Ymd H:i:s', $ThisWeekStart));

        if ($this->_PlayerData->_WeeklyPlayCountDateTime > $ThisWeekStart)     // 이번주 누적
        {
            $data['$inc']['WeeklyPlayCount']         = 1;
            $data['$set']['WeeklyPlayCountDateTime'] = (int) $currentTime;
            $data['$inc']['WeeklyPlayTime']          = (int) $PlayTime;
            $data['$set']['WeeklyPlayTimeDateTime']  = (int) $currentTime;

            if ($VicType == "VIC")
            {
	            TRACE('전체 / 주간 승리 횟수 증가');
                $data['$inc']['TotalVictoryCount']          = 1;  // 전체 승리 횟수 증가
                $data['$inc']['WeeklyVictoryCount']         = 1; // 주간 승리 횟수 증가
                $data['$set']['WeeklyVictoryCountDateTime'] = (int) $currentTime;
            }
            else
            {
	            TRACE('전체 / 주간 패배 횟수 증가');
                $data['$inc']['TotalDefeatCount']          = 1;  // 전체 패배 횟수 증가
                $data['$inc']['WeeklyDefeatCount']         = 1;  // 주간 패배 횟수 증가
                $data['$set']['WeeklyDefeatCountDateTime'] = (int) $currentTime;
            }
        }
        else                    // 이번주 초기화
        {
            $data['$set']['WeeklyPlayCount']         = 1;
            $data['$set']['WeeklyPlayCountDateTime'] = (int) $currentTime;
            $data['$set']['WeeklyPlayTime']          = (int) $PlayTime;
            $data['$set']['WeeklyPlayTimeDateTime']  = (int) $currentTime;

            if ($VicType == "VIC")
            {
                $data['$inc']['TotalVictoryCount']          = 1;  // 전체 승리 횟수 증가
                $data['$set']['WeeklyVictoryCount']         = 1; // 주간 승리 횟수 증가
                $data['$set']['WeeklyDefeatCount']          = 0;  // 주간 패배 횟수 증가
                $data['$set']['WeeklyVictoryCountDateTime'] = (int) $currentTime;
            }
            else
            {
                $data['$inc']['TotalDefeatCount']          = 1;  // 전체 패배 횟수 증가
                $data['$set']['WeeklyVictoryCount']        = 0;  // 주간 패배 횟수 증가
                $data['$set']['WeeklyDefeatCount']         = 1;  // 주간 패배 횟수 증가
                $data['$set']['WeeklyDefeatCountDateTime'] = (int) $currentTime;
            }
        }

        // 3 단계: 레벨업
        $IsLevelUP = false;
        $remainEXP = 0;

        // 최고 레벨이 아니면 EXP에 따른 레벨업 작업을 한다.
        if ($this->_PlayerData->_Level < CONFIG_GAME::MAX_LEVEL)
        {
            GetItemManager()->PreGroupGunMan(); // 건맨 테이블을 만드는 작업을 한다. 평상시에는 하지 않고 꼭 필요할때만 table을 분리하여 레벨별로 가지고 있는다.

            $FutureEXP = (int) $this->_PlayerData->_Exp + (int) $Exp;

            $CurrentGunMan = GetItemManager()->FindGunManData($this->_PlayerData->_Level);
            $FutureGunMan  = GetItemManager()->FindGunManData($this->_PlayerData->_Level + 1);
            if (isset($FutureGunMan) == false || isset($CurrentGunMan) == false)  // 둘 중에 하나라도 없음 안되겠다.
            {
                RaiseError('ERR_DB');
                WRITE_FINISH_FAIL('GameFinish Fail UID: ' . GetUserId() . ', Error Code: RBFinishGame DB ERROR 1');
                return false;
            }
            $orgLevel   = (int) $this->_PlayerData->_Level;
            $RequireEXP = (int) ($CurrentGunMan['XpRequired']);
            TRACE('현재 LEV: ' . (int) $this->_PlayerData->_Level);
            TRACE('현재 EXP: ' . (int) $this->_PlayerData->_Exp);
            TRACE('얻은 EXP: ' . (int) $Exp);
            TRACE('미래 EXP: ' . $FutureEXP);
            TRACE('레벨 업에 필요한 EXP: ' . $RequireEXP);
            if ($FutureEXP >= $RequireEXP)
            {
                TRACE('레벨업 처리를 하고 싶다이..');

                $MReward = GetValInt($FutureGunMan, 'MReward', 0);
                $GReward = GetValInt($FutureGunMan, 'GReward', 0);
                TRACE('LevelUp MReward: ' . $MReward . ', GReward: ' . $GReward);

                if ($MReward > 0)
                {
                    //$data['$inc']['Medal'] = (int)$MReward;
                    $Medal += (int) $MReward;

                    AddResult('lvupRewardType', 'M');
                    AddResult('lvupRewardAmount', $MReward);

                    //////////////////////////////////////////////////////////
                    //
                    // 위메이드 게임로그 - 재화 - 레벨업에 의한 메달 보상
                    //
                    //////////////////////////////////////////////////////////
                    SetWemeLogIsGetProperty(true);
                    SetWemeLogReasonCode(_Wemelog::RCGETMEDALBYLEVELUP);
                    SetWemeLogBeforeMedalAmount($this->GetMedal());
                    SetWemeLogAfterMedalAmount($this->GetMedal() + $MReward);
                    SendLog(_Wemelog::LTMEDAL, $this);
//                    SendPropertyLog(_Wemelog::LTMEDAL, _Wemelog::PGET, _Wemelog::RCGETMEDALBYLEVELUP, $this->GetMedal(), $this->GetMedal() + $MReward, $this);
                }
                else if ($GReward > 0)
                {
                    $Gold += (int) $GReward;

                    AddResult('lvupRewardType', 'G');
                    AddResult('lvupRewardAmount', $GReward);

                    //////////////////////////////////////////////////////////
                    //
                    // 위메이드 게임로그 - 재화 - 레벨업에 의한 골드 보상
                    //
                    //////////////////////////////////////////////////////////
                    SetWemeLogIsGetProperty(true);
                    SetWemeLogReasonCode(_Wemelog::RCGETGOLDBYLEVELUP);
                    SetWemeLogBeforeGoldAmount($this->GetGold());
                    SetWemeLogAfterGoldAmount($this->GetGold() + $GReward);
                    SendLog(_Wemelog::LTGOLD, $this);
                }
                else
                {
                    AddResult('lvupRewardType', 'X');
                    AddResult('lvupRewardAmount', 0);
                }

                $data['$inc']['Level'] = 1;      // 레벨을 1만큼 올려주고
                $remainEXP             = $FutureEXP - $RequireEXP;
                $data['$set']['Exp']   = (int) $remainEXP;  // 남은 경험치를 저장해준다.

                TRACE('레벨업 하고 남은 EXP: ' . $remainEXP);


                if ($this->_PlayerData->_Life < CONFIG_GAME::MAX_LIFE)
                {
                    $data['$set']['Life']            = CONFIG_GAME::MAX_LIFE; // 연료통도 채워준다.
                    $data['$set']['LifeUseDateTime'] = (int) $currentTime;

                    //////////////////////////////////////////////////////////
                    //
                    // 위메이드 게임로그 - 재화 - 레벨업에 의한 연료 보상
                    //
                    //////////////////////////////////////////////////////////
                    SetWemeLogIsGetProperty(true);
                    SetWemeLogReasonCode(_Wemelog::RCGETLIFEBYLEVELUP);
                    SetWemeLogBeforeLifeAmount($this->GetLife());
                    SetWemeLogAfterLifeAmount(CONFIG_GAME::MAX_LIFE);
                    SendLog(_Wemelog::LTLIFE, $this);
                }

                $IsLevelUP = true;
                $newLevel  = (int) $this->_PlayerData->_Level + 1;

                // 우편함으로 주는 레벨업 보상 추가
                if ($this->LevelupReward($newLevel) == true)
                {
                    AddResult('lvuppostboxreward', 1);
                }

                // 위메이드 게임로그 - 레벨업 로그 플래그 On
                SetWemeLogPlayerIsLevelUp(true);

                TRACE('레벨업하고 된 레벨: ' . $newLevel);
				
				// 튜토리얼 레벨 5, 10 카테고리,인덱스 설정
				if($newLevel == CONFIG_GAME::TUTORIAL_CHECKLVF)
				{
					$data['$set']['TutorialStepCategory'] = CONFIG_GAME::TUTORIAL_LVF_CATEGORY;
					$data['$set']['TutorialStepIndex'] = CONFIG_GAME::TUTORIAL_FIRST_INDEX;
				}
				else if($newLevel == CONFIG_GAME::TUTORIAL_CHECKLVT)
				{
					$data['$set']['TutorialStepCategory'] = CONFIG_GAME::TUTORIAL_LVT_CATEGORY;
					$data['$set']['TutorialStepIndex'] = CONFIG_GAME::TUTORIAL_FIRST_INDEX;
				}

            }
            else
            {
                TRACE('레벨 업 안한다이');
                $data['$inc']['Exp'] = (int) $Exp;
            }
        }
        $data['$inc']['Gold'] = (int) $Gold;

        // 4단계: 연승 처리 (디비)
        $OriginalConsecutiveVictory                 = 0;
        $data['$set']['PVP_LastConsecutiveVictory'] = (int) $this->_PlayerData->_PVP_ConsecutiveVictory;   // 직전 연승수를 저장하고
        if ($VicType == "VIC")
        {
            $OriginalConsecutiveVictory = (int) $this->_PlayerData->_PVP_ConsecutiveVictory + 1;

            if ($OriginalConsecutiveVictory > $this->_PlayerData->_PVP_TopConsecutiveVictory) // 최고 연승을 돌파했다믄
            {
                $data['$set']['PVP_TopConsecutiveVictory']         = $OriginalConsecutiveVictory;
                $data['$set']['PVP_TopConsecutiveVictoryDateTime'] = (int) $currentTime;
            }

            // 100연승이 넘으면 리셋요~
            if ($OriginalConsecutiveVictory >= CONFIG_GAME::CONSECUTIVE_VICTORY_MAX) $data['$set']['PVP_ConsecutiveVictory'] = 0;
            else $data['$inc']['PVP_ConsecutiveVictory'] = 1;
        }
        else
        {
            $data['$set']['PVP_ConsecutiveVictory'] = 0;
        }

        $data['$inc']['Medal'] = (int) $Medal;

        // 연승 대전 처리 (공헌도 계산과 디비)
        $GainRBattleSeasonPoint = 0;
        $GainRBattleWarPoint    = 0;
        $LegionScore1           = 0;
        $LegionScore2           = 0;
        if (SC($VicType, "VIC") && $this->_PlayerData->_RBattleWarKey != 0)
        {
            $GainRBattleSeasonPoint = CONFIG_GAME::RBATTLEPOINT_WINDEFAULT;
            GetRewardManager()->PreGroupVicReward();
            $RewardData             = GetRewardManager()->GetVicReward($OriginalConsecutiveVictory);
            if (isset($RewardData) && isset($RewardData['VicAddContribution'])) $GainRBattleSeasonPoint += (int) $RewardData['VicAddContribution'];

            $GainRBattleWarPoint = $GainRBattleSeasonPoint;

            $RBattle = GetRandomBattleManager()->AddWarResult(GetUserId(), $this->_PlayerData->_RBattleSeasonKey, $this->_PlayerData->_RBattleWarKey, $this->_PlayerData->_RBattleLegion,
                $GainRBattleSeasonPoint);
            if (is_array($RBattle))
            {
                if ($RBattle['Success'] == 0) $GainRBattleWarPoint = 0;

                SetWemeLogLegionScore($GainRBattleWarPoint);

                $data['$inc']['RBattleSeasonPoint'] = (int) $GainRBattleSeasonPoint;
                $data['$inc']['RBattleWarPoint']    = (int) $GainRBattleWarPoint;

                $LegionScore1 = $RBattle[0];
                $LegionScore2 = $RBattle[1];
            }
            else
            {
                $GainRBattleSeasonPoint = 0;
                $GainRBattleWarPoint    = 0;

                SetWemeLogLegionScore($GainRBattleWarPoint);
            }
        }
        else
        {
            $LegionScoreArray = GetRandomBattleManager()->GetLegionScore($this->_PlayerData->_RBattleSeasonKey, $this->_PlayerData->_RBattleWarKey);
            $LegionScore1     = $LegionScoreArray[0];
            $LegionScore2     = $LegionScoreArray[1];
        }

        // 최종 : DB처리
        if (MDB()->UpdateData(COLL::PLAYER, $key, $data) == false)
        {
            RaiseError('ERR_DB');
            WRITE_FINISH_FAIL('GameFinish Fail UID: ' . GetUserId() . ', Error Code: RBFinishGame DB ERROR FINAL');
            return false;
        }


        // 획득 EXP, GOLD 저장
        $this->_PlayerData->_Gold += (int) $Gold;

        // 2-1 단계: 최근 획득 Score 저장
        $this->_PlayerData->_RecentScore         = (int) $Score;
        $this->_PlayerData->_RecentScoreDateTime = (int) $currentTime;


        // 2-2 단계: 지금 획득한 Score가 TOP보다 크면 TOP 갱신해준다.
        if ((int) $this->_PlayerData->_TopScore < (int) $Score)
        {
            $this->_PlayerData->_TopScore         = (int) $Score;
            $this->_PlayerData->_TopScoreDateTime = (int) $currentTime;
        }

        // 2-3 단계: 주간 누적 Score
        if ((int) $this->_PlayerData->_WeeklyAccumulateScoreDateTime > $ThisWeekStart) // 이번주 기록에 누적해야 되면
        {
            // 주간 최고 기록이 바뀌면
            if ((int) $Score > (int) $this->_PlayerData->_WeeklyTopScore)     // 이번주 최고 기록보다 크면
            {
                $this->_PlayerData->_WeeklyTopScore         = (int) $Score;
                $this->_PlayerData->_WeeklyTopScoreDateTime = (int) $currentTime;
            }
            // 기존 기록에 누적시켜준다.
            $this->_PlayerData->_WeeklyAccumulateScore += (int) $Score;
            $this->_PlayerData->_WeeklyAccumulateScoreDateTime = (int) $currentTime;
        }
        else
        {
            // 이번주 전 기록이라면 덮어 쓴다.
            $this->_PlayerData->_WeeklyTopScore                = (int) $Score;
            $this->_PlayerData->_WeeklyTopScoreDateTime        = (int) $currentTime;
            $this->_PlayerData->_WeeklyAccumulateScore         = (int) $Score;
            $this->_PlayerData->_WeeklyAccumulateScoreDateTime = (int) $currentTime;
        }

        // 2-4 단계: 플레이 전적을 관리하자.
        $this->_PlayerData->_TotalPlayCount += 1;     // 전체 플레이 카운트 증가 시키고
        $this->_PlayerData->_TotalPlayTime += (int) $PlayTime;    // 전체 플레이 시간 증가 시키고 (ms)
        $this->_PlayerData->_TotalPlayTimeDateTime = (int) $currentTime; // 전체 플레이 시간 기록 시간을 기록한다! ..; 이건 필요 없는데..그냥..

        if ((int) $this->_PlayerData->_WeeklyPlayCountDateTime > (int) $ThisWeekStart)     // 이번주 누적
        {
            $this->_PlayerData->_WeeklyPlayCount += 1;
            $this->_PlayerData->_WeeklyPlayCountDateTime = (int) $currentTime;
            $this->_PlayerData->_WeeklyPlayTime += (int) $PlayTime;
            $this->_PlayerData->_WeeklyPlayTimeDateTime  = (int) $currentTime;

            if ($VicType == "VIC")
            {
                $this->_PlayerData->_TotalVictoryCount += 1;  // 전체 승리 횟수 증가
                $this->_PlayerData->_WeeklyVictoryCount += 1;  // 주간 승리 횟수 증가
                $this->_PlayerData->_WeeklyVictoryCountDateTime = (int) $currentTime;
            }
            else
            {
                $this->_PlayerData->_TotalDefeatCount += 1;  // 전체 패배 횟수 증가
                $this->_PlayerData->_WeeklyDefeatCount += 1;  // 주간 패배 횟수 증가
                $this->_PlayerData->_WeeklyDefeatCountDateTime = (int) $currentTime;
            }
        }
        else                    // 이번주 초기화
        {
            $this->_PlayerData->_WeeklyPlayCount         = 1;
            $this->_PlayerData->_WeeklyPlayCountDateTime = (int) $currentTime;
            $this->_PlayerData->_WeeklyPlayTime          = (int) $PlayTime;
            $this->_PlayerData->_WeeklyPlayTimeDateTime  = (int) $currentTime;

            if ($VicType == "VIC")
            {
                $this->_PlayerData->_TotalVictoryCount += 1;  // 전체 승리 횟수 증가
                $this->_PlayerData->_WeeklyVictoryCount         = 1; // 주간 승리 횟수 증가
                $this->_PlayerData->_WeeklyDefeatCount          = 0;  // 주간 패배 횟수 증가
                $this->_PlayerData->_WeeklyVictoryCountDateTime = (int) $currentTime;
            }
            else
            {
                $this->_PlayerData->_TotalDefeatCount += 1;  // 전체 패배 횟수 증가
                $this->_PlayerData->_WeeklyVictoryCount        = 0; // 주간 승리 횟수 증가
                $this->_PlayerData->_WeeklyDefeatCount         = 1;  // 주간 패배 횟수 증가
                $this->_PlayerData->_WeeklyDefeatCountDateTime = (int) $currentTime;
            }
        }

        if ($IsLevelUP == true)
        {
            $this->_PlayerData->_Level = (int) $newLevel;
            $this->_PlayerData->_Exp   = (int) $remainEXP;

            if ($this->_PlayerData->_Life < CONFIG_GAME::MAX_LIFE)
            {
                $this->_PlayerData->_Life            = CONFIG_GAME::MAX_LIFE; // 연료통도 채워준다.
                $this->_PlayerData->_LifeUseDateTime = (int) $currentTime;
            }
        }
        else
        {
            $this->_PlayerData->_Exp += (int) $Exp;
        }

        // 4단계: 연승 처리 (서버 메모리)
        $this->_PlayerData->_PVP_LastConsecutiveVictory = (int) $this->_PlayerData->_PVP_ConsecutiveVictory;  // 직전 연승수를 저장하고
        if ($VicType == "VIC")
        {
            if ($OriginalConsecutiveVictory > $this->_PlayerData->_PVP_TopConsecutiveVictory) // 최고 연승을 돌파했다믄
            {
                $this->_PlayerData->_PVP_TopConsecutiveVictory         = $OriginalConsecutiveVictory;
                $this->_PlayerData->_PVP_TopConsecutiveVictoryDateTime = (int) $currentTime;
            }

            // 달성한 연승으로 보상받기 위해 세팅함.
            $Ref_RewardCVCount = $OriginalConsecutiveVictory;

            if ($OriginalConsecutiveVictory >= CONFIG_GAME::CONSECUTIVE_VICTORY_MAX) $this->_PlayerData->_PVP_ConsecutiveVictory = 0;
            else $this->_PlayerData->_PVP_ConsecutiveVictory += 1;
        }
        else
        {
            $this->_PlayerData->_PVP_ConsecutiveVictory = 0;
        }
        AddResult('cvc', $OriginalConsecutiveVictory);

//		TRACE("WHWUSS vic:".$VicType);
//		TRACE("WHWUSS cvc:".$OriginalConsecutiveVictory."  id:".GetUserId());
//		global $__RESULT_ARRAY;
//		TRACE("WHWUSS result array :".$__RESULT_ARRAY['cvc']);
//		TRACE("WHWUSS player:".$this->_PlayerData->_PVP_ConsecutiveVictory);


        $this->_PlayerData->_Medal += (int) $Medal;

        // 연승 대전 처리 (서버 메모리)
        if ($GainRBattleSeasonPoint > 0)
        {
            $this->_PlayerData->_RBattleSeasonPoint += $GainRBattleSeasonPoint;
            $this->_PlayerData->_RBattleWarPoint += $GainRBattleWarPoint;

            AddResult('rbattle_point', $GainRBattleSeasonPoint);
            AddResult('rbattle_legionscore1', $LegionScore1);
            AddResult('rbattle_legionscore2', $LegionScore2);
        }

        // RDS에 기록을 쓴다.
        GetScoreBoardObject()->UpdateScoreData(GetUserId(), $this->_PlayerData->_Level, $this->_PlayerData->_WeeklyAccumulateScore, $this->_PlayerData->_RecentScore,
            $this->_PlayerData->_PVP_ConsecutiveVictory, // 현재 연승
            $this->_PlayerData->_EquipAircraft, $this->_PlayerData->_EquipSkin, $this->_PlayerData->_EquipTurret, $this->_PlayerData->_EquipPilot, $this->_PlayerData->_MsgRecvSwitch,
            $this->_PlayerData->_GiftRecvSwitch, $this->_PlayerData->_PPISwitch, $this->_PlayerData->_Nickname, true);

        if (CONFIG_GAME::SWITCH_DISPLAY_GLOBALRANKING == 1)
        {
            GetScoreBoardObject()->SetGlobalRank(GetUserId(), $this->_PlayerData->_WeeklyAccumulateScore);
        }

        return true;
    }
