//====================================================================================================================================
	// Step 3-3 : ���Ϸ�(Pilot)�� ���� ���׷��̵� ���� ���� Ȯ�� �� ���׷��̵�
	//====================================================================================================================================
	
	else if( $UpgradeItemType == "Pilot" )		// ���Ϸ� ���׷��̵��.
	{
		$CurrentPilotageLevel	= $PlayerObject->GetPlayerData()->_Inven['PilotageLevel_'.$UpgradeItemId];
		$CurrentEvadeLevel		= $PlayerObject->GetPlayerData()->_Inven['EvadeLevel_'.$UpgradeItemId];
		$CurItem				= $ItemManager->GetItem($UpgradeItemId);
		$MaxUpgradeLevel		= CONFIG_GAME::MAX_PILOTEVADE_LEVEL;// ���� �ִ� ������ 21������. //(int)$PlayerObject->GetLevel();	
	
/*		if( $UpgradeItemStats == "PT" && (int)$CurrentPilotageLevel == $MaxUpgradeLevel )
		{
			// �̹� �ְ� �ִ� ������ �����.
			RaiseError('ERR_UPGRADE_ALREADY_MAX');
			return false;
		}
*/
		if( $UpgradeItemStats == "EV" && (int)$CurrentEvadeLevel == $MaxUpgradeLevel )	
		{
			// �̹� �ְ� �ִ� ������ �����.
			RaiseError('ERR_UPGRADE_ALREADY_MAX');
			return false;
		}

		if( $UpgradeItemStats == "EV" )	// ȸ�� �⵿ ���׷��̵��
		{
			$NextLevel	= $CurrentEvadeLevel + 1;
			$EvadeInfoIndex = 9000300 + $NextLevel;
			
			$EvadeInfo	= $ItemManager->GetItem($EvadeInfoIndex);
			$RealPrice	= $EvadeInfo['DgPctUpGold'];			// ��¥ �Ҹ�Ǵ� Gold��

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
	// Step 4 : ���׷��̵� ����� �÷��̾� �κ� ����� ����
	//====================================================================================================================================
	AddResult('Type', $UpgradeItemType );
	AddResult('RemainGold', $PlayerObject->GetGold() );
	AddResult('ItemIdx', $UpgradeItemId );
	AddResult('Inven', ConvertPlayerInvenInfo($PlayerObject) );
	AddResult('Equip', ConvertPlayerEquipInfo($PlayerObject) );
	//====================================================================================================================================
	// ���� : ������� ������ ���� �� ���̴�.
	//====================================================================================================================================
	RaiseError('SUCCESS');	// ������ ����. �� ����Ǿ���. SUCCESS�� �����ϸ� �ȴ�.
	return true;
}




function OPERATE_TUTORIALCHECK()
{
	TRACE('====================================Start Tutorial Fighter, Pilot Check======================================');

	if( CONFIG_DEBUG::ENABLE_VALIDATION_CHECK == 1 )
	{
		//====================================================================================================================================
		// Step 1 : ������ ��ȿ�� üũ
		//====================================================================================================================================
		
		if( IsValidRequestArguments() == false )
		{
			// TODO ERROR LOG : Arguments �� !! Ʋ�ȴ�. 
			// TODO ADD_DB_LOG
			RaiseError('ERR_VARIABLE');
			return false;
		}
	}	
	
	//====================================================================================================================================
	// Step 2 : �÷��̾� ��ü ��� �����غ���
	//====================================================================================================================================
	$PlayerObject = GetPlayerObject();
	$nRet = GetAuthModule()->CheckAuth( GetUserId(), GetClientAccessToken(), GetClientProtocol() );
	if ( $nRet != GetErrorCode('SUCCESS') )
	{
		// ���� ������ �߻��ߴ�. 
		// ������ ������ GetAuthModule()->CheckAuth ���� RaiseError �ϰ� ������ ������ ���� �ڵ�� ���� �ʴ´�.
		return false;
	}

	TRACE("User ID : " .GetUserID());

	//====================================================================================================================================
	// Step 3 : Ʃ�丮�� ��ü, ���Ϸ� üũ
	//====================================================================================================================================
	$FighterCheck = GetTutorialFighterCheck();
	$PilotCheck = GetTutorialPilotCheck();
	$IntroCheck = GetTutorialIntroCheck();
	

	TRACE("====== ��üüũ : $FighterCheck ==== ���Ϸ� üũ : $PilotCheck === ��Ʈ�� üũ : $IntroCheck ====");
	
	if($PlayerObject->TutorialCheck($FighterCheck, $PilotCheck, $IntroCheck) == false)
	{
		TRACE("FighterCheck : $FighterCheck === PilotCheck : $PilotCheck === IntroCheck : $IntroCheck");
		return false;
	}
	
	//====================================================================================================================================
	// ���� : ������� ������ ���� �� ���̴�.
	//====================================================================================================================================
	RaiseError('SUCCESS');	// ������ ����. �� ����Ǿ���. SUCCESS�� �����ϸ� �ȴ�.
	return true;
}

function OPERATE_TUTORIALPROGRESS()
{
	TRACE("================ Tutorial Progress ===================");
	if( CONFIG_DEBUG::ENABLE_VALIDATION_CHECK == 1 )
	{
		//====================================================================================================================================
		// Step 1 : ������ ��ȿ�� üũ
		//====================================================================================================================================
		
		if( IsValidRequestArguments() == false )
		{
			// TODO ERROR LOG : Arguments �� !! Ʋ�ȴ�. 
			// TODO ADD_DB_LOG
			RaiseError('ERR_VARIABLE');
			return false;
		}
	}	
	
	//====================================================================================================================================
	// Step 2 : �÷��̾� ��ü ��� �����غ���
	//====================================================================================================================================
	$PlayerObject = GetPlayerObject();
	$nRet = GetAuthModule()->CheckAuth( GetUserId(), GetClientAccessToken(), GetClientProtocol() );
	if ( $nRet != GetErrorCode('SUCCESS') )
	{
		// ���� ������ �߻��ߴ�. 
		// ������ ������ GetAuthModule()->CheckAuth ���� RaiseError �ϰ� ������ ������ ���� �ڵ�� ���� �ʴ´�.
		TRACE("=============== Tutorial Player Auth Error =================");
		return false;
	}

	//TRACE("RegDate : $PlayerData['RegDateTime'];
	//====================================================================================================================================
	// Step 3 : Ʃ�丮�� �ѹ��� �� 
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
		
	TRACE("===================Ʃ�丮�� ����Ÿ�� : $TutorialFinishType ====================");


	if((SC($TutorialFinishType, 'NR') == true && $Index == 1 ) && 
		($Category == CHAPTER1_CON || $Category == CHAPTER2_END))		//������ ���� �������� ���൵�� ����
	{
		$StageIdx			= 100001;
		
		TRACE("====================== �������� : $StageIdx ======================");
		
		if($PlayerObject->TutorialStage($StageIdx) == false)
		{
			TRACE("Tutorial Stage Fail... FinishType : $TutorialFinishType");
			return false;
		}			
	}
	else if(SC($TutorialFinishType, 'LUP') == true)	// ���� ������
	{
		if($PlayerObject->TutorialForceLv($TutorialStepTime) == false)
		{
			TRACE("Tutorial Force LevelUp FAIL.. FinishType : $TutorialFinishType");
			return false;
		}
	}
		
	
	//====================================================================================================================================
	// ���� : ������� ������ ���� �� ���̴�.
	//====================================================================================================================================
	RaiseError('SUCCESS');	// ������ ����. �� ����Ǿ���. SUCCESS�� �����ϸ� �ȴ�.
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

			// ó�� ȹ�� �ΰ�?
			if( isset($PlayerObject->_PlayerData->_Inven[$IndexName]) == false )
			{
				$res_set['card'] = array('idx' => $TargetCardIndex, 'enchant' => 0, 'exp' => 0, 'lupxpb' => 0, 'lupxpe' => 0, 'luplvb' => 1, 'luplve' => 1, 'enclvb' => 0, 'enclve' => 0);
				break;
			}

			// ī�� ���� ���
			$BeforeUpgradeNum = (int)$PlayerObject->_PlayerData->_Inven[$IndexName];
			$BeforeExp = 0;
			if( isset($PlayerObject->_PlayerData->_Inven[$ExpIndexName]) )
				$BeforeExp = (int)$PlayerObject->_PlayerData->_Inven[$ExpIndexName];
			$BeforeLevel = (int)$this->CalcPCardLevel($TargetCardIndex, $BeforeExp);

			$CardItemData = GetItemManager()->GetItem($TargetCardIndex);
			if( is_null($CardItemData) || isset($CardItemData['PcardUpgradeNum']) == false || isset($CardItemData['PcardMaxLV']) == false || SC($CardItemData['Type'],'PCard') == false )
			{
				// ����
				return false;
			}

			$MaxUpgradeNum = (int)$CardItemData['PcardUpgradeNum'];
			$MaxLevel = (int)$CardItemData['PcardMaxLV'];

			// ��ȭ�� �Ұ����Ѱ�?
			if( $MaxUpgradeNum <= 0 )
			{
				// MaxLevel �ΰ�
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

			// ������ �ִ� ī�带 ��ȭ ���� �� ��ȭ��� max ���� ������
			if( $BeforeUpgradeNum + 1 < $MaxUpgradeNum )
			{
				$res_set['card'] = array('idx' => $TargetCardIndex, 'enchant' => $BeforeUpgradeNum + 1, 'exp' => $BeforeExp, 'lupxpb' => $BeforeExp, 'lupxpe' => $BeforeExp, 'luplvb' => $BeforeLevel, 'luplve' => $BeforeLevel, 'enclvb' => $BeforeUpgradeNum, 'enclve' => $BeforeUpgradeNum + 1);
				break;
			}

			// ���� ����� ī�尡 �ִ°�?
			if( isset($CardItemData['PCardUpgrade']) == false )
			{
				// ����
				return false;
			}

			// Ÿ���� �ٲ����
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
        // ������ ����� ���� ó������. �̰� db�� �ѹ��� �о� �ִ� ����� �����ؾ� �Ѵ�.
        // �ϴ� ���ϴ� �ϳ����̶� ó��.
        // 1 �ܰ�: ������ �Ҹ�
        if ($UseItemArray)
        {
            foreach ($UseItemArray as $ItemIdx => $Count)
            {
                $ItemIdx = (int) $ItemIdx;

                // ���� ���������� ����.
                $ItemOrgData = GetItemManager()->GetItem($ItemIdx);
                if (isset($ItemOrgData) == false)
                {
                    //RaiseError('ERR_WRONG_ITEMIDX');
                    WRITE_FINISH_FAIL('GameFinish Fail UID: ' . GetUserId() . ', Error Code: RBFinishGame WRONG ITEMIDX: ' . $ItemIdx);
                    continue;
                    //return false;
                }

                if ($ItemIdx != 5000000) // �⺻ �̻����� �ƴ϶��
                {
                    $SelectedItem = GetItemManager()->GetItem((int) $ItemIdx);
                    $MaxAmount    = isset($SelectedItem['MaxAmount']) ? $SelectedItem['MaxAmount'] : 1;
                    $this->ChangeItemAmount($ItemIdx, $MaxAmount, -$Count);
                }
            }
        }
        TRACE("Finish : S($Score), E($Exp), G($Gold)");
        ////////////////////////////////////////////////////////////////////////////////////////////////
        // �̺�Ʈ ���� ���� ���� ó���غ���.
        ////////////////////////////////////////////////////////////////////////////////////////////////
        // ��ũŸ�� �̺�Ʈ
        $EventManager = GetEventManager(false); // ��ũŸ�� �̺�Ʈ�� �ε��Ѵ�.
        $EventFlag    = $EventManager->IsPeakTimeEventApply();

        $RewardType   = '';
        $RewardAmount = 0;

        if ($EventFlag == 1 || $EventFlag == 2)
        {
            $RewardType   = $EventManager->GetPeakTimeEventRewardType();
            $RewardAmount = (int) $EventManager->GetPeakTimeEventRewardAmount();

            TRACE('��ũŸ�� �̺�Ʈ ���� ��:' . $RewardType . ", " . $RewardAmount);
        }

        if ($EventFlag != false && $RewardType == 'G')
        {
            $NewGold = (int) $Gold + (int) (((int) $Gold / 100) * ((int) $RewardAmount));
            TRACE('��ũŸ�� �̺�Ʈ ��� ������ : ' . $Gold . ' -> ' . $NewGold);
            $Gold    = (int) $NewGold;
        }
        else if ($EventFlag != false && $RewardType == 'S')
        {
            $NewScore = (int) $Score + (int) (((int) $Score / 100) * ((int) $RewardAmount));
            TRACE('��ũŸ�� �̺�Ʈ ���ھ� ������ : ' . $Score . ' -> ' . $NewScore);
            $Score    = (int) $NewScore;
        }
        else if ($EventFlag != false && $RewardType == 'X')
        {
            $NewExp = (int) $Exp + (int) (((int) $Exp / 100) * (int) $RewardAmount);
            TRACE('��ũŸ�� �̺�Ʈ ����ġ ������ : ' . $Exp . ' -> ' . $NewExp);
            $Exp    = (int) $NewExp;
        }
        ////////////////////////////////////////////////////////////////////////////////////////////////

        TRACE("Event Finish : S($Score), E($Exp), G($Gold)");


        // 2 �ܰ�: ���� ���� ����
        $currentTime = GetNowTime();
        $key['_id']  = GetUserId();
        $data        = array();

        //$data['$inc']['Gold'] = (int)$Gold;
        // 2-1 �ܰ�: �ֱ� ȹ�� SCORE�� ����.
        $data['$set']['RecentScore']         = (int) $Score;
        $data['$set']['RecentScoreDateTime'] = (int) $currentTime;

        // 2-2 �ܰ�: ���� ȹ���� EXP�� TOP���� ũ�� TOP �������ش�.
        if ($this->_PlayerData->_TopScore < (int) $Score)
        {
            $data['$set']['TopScore']         = (int) $Score;
            $data['$set']['TopScoreDateTime'] = (int) $currentTime;
        }

        // �̹��� ����̸� ���ؼ� TopScore�� �־��ְ�..�ƴϸ� �̹��� ���� ����̹Ƿ� Top�� �־��ش�.
        $NextResetTime = GetNextResetTime();
        $ThisWeekStart = $NextResetTime - (3600 * 24 * 7);

        // 2-3 �ܰ�: �ְ� ���� Score
        if ($this->_PlayerData->_WeeklyAccumulateScoreDateTime > $ThisWeekStart) // �̹��� ��Ͽ� �����ؾ� �Ǹ�
        {
            // �ְ� �ְ� ����� �ٲ��
            if ((int) $Score > $this->_PlayerData->_WeeklyTopScore)     // �̹��� �ְ� ��Ϻ��� ũ��
            {
                $data['$set']['WeeklyTopScore']         = (int) $Score;
                $data['$set']['WeeklyTopScoreDateTime'] = (int) $currentTime;
            }
            // ���� ��Ͽ� ���������ش�.
            $data['$inc']['WeeklyAccumulateScore']         = (int) $Score;
            $data['$set']['WeeklyAccumulateScoreDateTime'] = (int) $currentTime;
        }
        else
        {
            // �̹��� �� ����̶�� ���� ����.
            $data['$set']['WeeklyTopScore']                = (int) $Score;
            $data['$set']['WeeklyTopScoreDateTime']        = (int) $currentTime;
            $data['$set']['WeeklyAccumulateScore']         = (int) $Score;
            $data['$set']['WeeklyAccumulateScoreDateTime'] = (int) $currentTime;
        }

        // 2-4 �ܰ�: �÷��� ������ ��������.
        $data['$inc']['TotalPlayCount']        = 1;     // ��ü �÷��� ī��Ʈ ���� ��Ű��
        $data['$inc']['TotalPlayTime']         = (int) $PlayTime;    // ��ü �÷��� �ð� ���� ��Ű�� (ms)
        $data['$set']['TotalPlayTimeDateTime'] = (int) $currentTime; // ��ü �÷��� �ð� ��� �ð��� ����Ѵ�! ..; �̰� �ʿ� ���µ�..�׳�..

	    TRACE("========================================================== Play count reference information ======================================================");
	    TRACE("WeeklyPlayCountDateTime : " . date('Ymd H:i:s', $this->_PlayerData->_WeeklyPlayCountDateTime) . "(" . var_export($this->_PlayerData->_WeeklyPlayCountDateTime, true) . ") ThisWeekStart : " . date('Ymd H:i:s', $ThisWeekStart));

        if ($this->_PlayerData->_WeeklyPlayCountDateTime > $ThisWeekStart)     // �̹��� ����
        {
            $data['$inc']['WeeklyPlayCount']         = 1;
            $data['$set']['WeeklyPlayCountDateTime'] = (int) $currentTime;
            $data['$inc']['WeeklyPlayTime']          = (int) $PlayTime;
            $data['$set']['WeeklyPlayTimeDateTime']  = (int) $currentTime;

            if ($VicType == "VIC")
            {
	            TRACE('��ü / �ְ� �¸� Ƚ�� ����');
                $data['$inc']['TotalVictoryCount']          = 1;  // ��ü �¸� Ƚ�� ����
                $data['$inc']['WeeklyVictoryCount']         = 1; // �ְ� �¸� Ƚ�� ����
                $data['$set']['WeeklyVictoryCountDateTime'] = (int) $currentTime;
            }
            else
            {
	            TRACE('��ü / �ְ� �й� Ƚ�� ����');
                $data['$inc']['TotalDefeatCount']          = 1;  // ��ü �й� Ƚ�� ����
                $data['$inc']['WeeklyDefeatCount']         = 1;  // �ְ� �й� Ƚ�� ����
                $data['$set']['WeeklyDefeatCountDateTime'] = (int) $currentTime;
            }
        }
        else                    // �̹��� �ʱ�ȭ
        {
            $data['$set']['WeeklyPlayCount']         = 1;
            $data['$set']['WeeklyPlayCountDateTime'] = (int) $currentTime;
            $data['$set']['WeeklyPlayTime']          = (int) $PlayTime;
            $data['$set']['WeeklyPlayTimeDateTime']  = (int) $currentTime;

            if ($VicType == "VIC")
            {
                $data['$inc']['TotalVictoryCount']          = 1;  // ��ü �¸� Ƚ�� ����
                $data['$set']['WeeklyVictoryCount']         = 1; // �ְ� �¸� Ƚ�� ����
                $data['$set']['WeeklyDefeatCount']          = 0;  // �ְ� �й� Ƚ�� ����
                $data['$set']['WeeklyVictoryCountDateTime'] = (int) $currentTime;
            }
            else
            {
                $data['$inc']['TotalDefeatCount']          = 1;  // ��ü �й� Ƚ�� ����
                $data['$set']['WeeklyVictoryCount']        = 0;  // �ְ� �й� Ƚ�� ����
                $data['$set']['WeeklyDefeatCount']         = 1;  // �ְ� �й� Ƚ�� ����
                $data['$set']['WeeklyDefeatCountDateTime'] = (int) $currentTime;
            }
        }

        // 3 �ܰ�: ������
        $IsLevelUP = false;
        $remainEXP = 0;

        // �ְ� ������ �ƴϸ� EXP�� ���� ������ �۾��� �Ѵ�.
        if ($this->_PlayerData->_Level < CONFIG_GAME::MAX_LEVEL)
        {
            GetItemManager()->PreGroupGunMan(); // �Ǹ� ���̺��� ����� �۾��� �Ѵ�. ���ÿ��� ���� �ʰ� �� �ʿ��Ҷ��� table�� �и��Ͽ� �������� ������ �ִ´�.

            $FutureEXP = (int) $this->_PlayerData->_Exp + (int) $Exp;

            $CurrentGunMan = GetItemManager()->FindGunManData($this->_PlayerData->_Level);
            $FutureGunMan  = GetItemManager()->FindGunManData($this->_PlayerData->_Level + 1);
            if (isset($FutureGunMan) == false || isset($CurrentGunMan) == false)  // �� �߿� �ϳ��� ���� �ȵǰڴ�.
            {
                RaiseError('ERR_DB');
                WRITE_FINISH_FAIL('GameFinish Fail UID: ' . GetUserId() . ', Error Code: RBFinishGame DB ERROR 1');
                return false;
            }
            $orgLevel   = (int) $this->_PlayerData->_Level;
            $RequireEXP = (int) ($CurrentGunMan['XpRequired']);
            TRACE('���� LEV: ' . (int) $this->_PlayerData->_Level);
            TRACE('���� EXP: ' . (int) $this->_PlayerData->_Exp);
            TRACE('���� EXP: ' . (int) $Exp);
            TRACE('�̷� EXP: ' . $FutureEXP);
            TRACE('���� ���� �ʿ��� EXP: ' . $RequireEXP);
            if ($FutureEXP >= $RequireEXP)
            {
                TRACE('������ ó���� �ϰ� �ʹ���..');

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
                    // �����̵� ���ӷα� - ��ȭ - �������� ���� �޴� ����
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
                    // �����̵� ���ӷα� - ��ȭ - �������� ���� ��� ����
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

                $data['$inc']['Level'] = 1;      // ������ 1��ŭ �÷��ְ�
                $remainEXP             = $FutureEXP - $RequireEXP;
                $data['$set']['Exp']   = (int) $remainEXP;  // ���� ����ġ�� �������ش�.

                TRACE('������ �ϰ� ���� EXP: ' . $remainEXP);


                if ($this->_PlayerData->_Life < CONFIG_GAME::MAX_LIFE)
                {
                    $data['$set']['Life']            = CONFIG_GAME::MAX_LIFE; // �����뵵 ä���ش�.
                    $data['$set']['LifeUseDateTime'] = (int) $currentTime;

                    //////////////////////////////////////////////////////////
                    //
                    // �����̵� ���ӷα� - ��ȭ - �������� ���� ���� ����
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

                // ���������� �ִ� ������ ���� �߰�
                if ($this->LevelupReward($newLevel) == true)
                {
                    AddResult('lvuppostboxreward', 1);
                }

                // �����̵� ���ӷα� - ������ �α� �÷��� On
                SetWemeLogPlayerIsLevelUp(true);

                TRACE('�������ϰ� �� ����: ' . $newLevel);
				
				// Ʃ�丮�� ���� 5, 10 ī�װ�,�ε��� ����
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
                TRACE('���� �� ���Ѵ���');
                $data['$inc']['Exp'] = (int) $Exp;
            }
        }
        $data['$inc']['Gold'] = (int) $Gold;

        // 4�ܰ�: ���� ó�� (���)
        $OriginalConsecutiveVictory                 = 0;
        $data['$set']['PVP_LastConsecutiveVictory'] = (int) $this->_PlayerData->_PVP_ConsecutiveVictory;   // ���� ���¼��� �����ϰ�
        if ($VicType == "VIC")
        {
            $OriginalConsecutiveVictory = (int) $this->_PlayerData->_PVP_ConsecutiveVictory + 1;

            if ($OriginalConsecutiveVictory > $this->_PlayerData->_PVP_TopConsecutiveVictory) // �ְ� ������ �����ߴٹ�
            {
                $data['$set']['PVP_TopConsecutiveVictory']         = $OriginalConsecutiveVictory;
                $data['$set']['PVP_TopConsecutiveVictoryDateTime'] = (int) $currentTime;
            }

            // 100������ ������ ���¿�~
            if ($OriginalConsecutiveVictory >= CONFIG_GAME::CONSECUTIVE_VICTORY_MAX) $data['$set']['PVP_ConsecutiveVictory'] = 0;
            else $data['$inc']['PVP_ConsecutiveVictory'] = 1;
        }
        else
        {
            $data['$set']['PVP_ConsecutiveVictory'] = 0;
        }

        $data['$inc']['Medal'] = (int) $Medal;

        // ���� ���� ó�� (���嵵 ���� ���)
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

        // ���� : DBó��
        if (MDB()->UpdateData(COLL::PLAYER, $key, $data) == false)
        {
            RaiseError('ERR_DB');
            WRITE_FINISH_FAIL('GameFinish Fail UID: ' . GetUserId() . ', Error Code: RBFinishGame DB ERROR FINAL');
            return false;
        }


        // ȹ�� EXP, GOLD ����
        $this->_PlayerData->_Gold += (int) $Gold;

        // 2-1 �ܰ�: �ֱ� ȹ�� Score ����
        $this->_PlayerData->_RecentScore         = (int) $Score;
        $this->_PlayerData->_RecentScoreDateTime = (int) $currentTime;


        // 2-2 �ܰ�: ���� ȹ���� Score�� TOP���� ũ�� TOP �������ش�.
        if ((int) $this->_PlayerData->_TopScore < (int) $Score)
        {
            $this->_PlayerData->_TopScore         = (int) $Score;
            $this->_PlayerData->_TopScoreDateTime = (int) $currentTime;
        }

        // 2-3 �ܰ�: �ְ� ���� Score
        if ((int) $this->_PlayerData->_WeeklyAccumulateScoreDateTime > $ThisWeekStart) // �̹��� ��Ͽ� �����ؾ� �Ǹ�
        {
            // �ְ� �ְ� ����� �ٲ��
            if ((int) $Score > (int) $this->_PlayerData->_WeeklyTopScore)     // �̹��� �ְ� ��Ϻ��� ũ��
            {
                $this->_PlayerData->_WeeklyTopScore         = (int) $Score;
                $this->_PlayerData->_WeeklyTopScoreDateTime = (int) $currentTime;
            }
            // ���� ��Ͽ� ���������ش�.
            $this->_PlayerData->_WeeklyAccumulateScore += (int) $Score;
            $this->_PlayerData->_WeeklyAccumulateScoreDateTime = (int) $currentTime;
        }
        else
        {
            // �̹��� �� ����̶�� ���� ����.
            $this->_PlayerData->_WeeklyTopScore                = (int) $Score;
            $this->_PlayerData->_WeeklyTopScoreDateTime        = (int) $currentTime;
            $this->_PlayerData->_WeeklyAccumulateScore         = (int) $Score;
            $this->_PlayerData->_WeeklyAccumulateScoreDateTime = (int) $currentTime;
        }

        // 2-4 �ܰ�: �÷��� ������ ��������.
        $this->_PlayerData->_TotalPlayCount += 1;     // ��ü �÷��� ī��Ʈ ���� ��Ű��
        $this->_PlayerData->_TotalPlayTime += (int) $PlayTime;    // ��ü �÷��� �ð� ���� ��Ű�� (ms)
        $this->_PlayerData->_TotalPlayTimeDateTime = (int) $currentTime; // ��ü �÷��� �ð� ��� �ð��� ����Ѵ�! ..; �̰� �ʿ� ���µ�..�׳�..

        if ((int) $this->_PlayerData->_WeeklyPlayCountDateTime > (int) $ThisWeekStart)     // �̹��� ����
        {
            $this->_PlayerData->_WeeklyPlayCount += 1;
            $this->_PlayerData->_WeeklyPlayCountDateTime = (int) $currentTime;
            $this->_PlayerData->_WeeklyPlayTime += (int) $PlayTime;
            $this->_PlayerData->_WeeklyPlayTimeDateTime  = (int) $currentTime;

            if ($VicType == "VIC")
            {
                $this->_PlayerData->_TotalVictoryCount += 1;  // ��ü �¸� Ƚ�� ����
                $this->_PlayerData->_WeeklyVictoryCount += 1;  // �ְ� �¸� Ƚ�� ����
                $this->_PlayerData->_WeeklyVictoryCountDateTime = (int) $currentTime;
            }
            else
            {
                $this->_PlayerData->_TotalDefeatCount += 1;  // ��ü �й� Ƚ�� ����
                $this->_PlayerData->_WeeklyDefeatCount += 1;  // �ְ� �й� Ƚ�� ����
                $this->_PlayerData->_WeeklyDefeatCountDateTime = (int) $currentTime;
            }
        }
        else                    // �̹��� �ʱ�ȭ
        {
            $this->_PlayerData->_WeeklyPlayCount         = 1;
            $this->_PlayerData->_WeeklyPlayCountDateTime = (int) $currentTime;
            $this->_PlayerData->_WeeklyPlayTime          = (int) $PlayTime;
            $this->_PlayerData->_WeeklyPlayTimeDateTime  = (int) $currentTime;

            if ($VicType == "VIC")
            {
                $this->_PlayerData->_TotalVictoryCount += 1;  // ��ü �¸� Ƚ�� ����
                $this->_PlayerData->_WeeklyVictoryCount         = 1; // �ְ� �¸� Ƚ�� ����
                $this->_PlayerData->_WeeklyDefeatCount          = 0;  // �ְ� �й� Ƚ�� ����
                $this->_PlayerData->_WeeklyVictoryCountDateTime = (int) $currentTime;
            }
            else
            {
                $this->_PlayerData->_TotalDefeatCount += 1;  // ��ü �й� Ƚ�� ����
                $this->_PlayerData->_WeeklyVictoryCount        = 0; // �ְ� �¸� Ƚ�� ����
                $this->_PlayerData->_WeeklyDefeatCount         = 1;  // �ְ� �й� Ƚ�� ����
                $this->_PlayerData->_WeeklyDefeatCountDateTime = (int) $currentTime;
            }
        }

        if ($IsLevelUP == true)
        {
            $this->_PlayerData->_Level = (int) $newLevel;
            $this->_PlayerData->_Exp   = (int) $remainEXP;

            if ($this->_PlayerData->_Life < CONFIG_GAME::MAX_LIFE)
            {
                $this->_PlayerData->_Life            = CONFIG_GAME::MAX_LIFE; // �����뵵 ä���ش�.
                $this->_PlayerData->_LifeUseDateTime = (int) $currentTime;
            }
        }
        else
        {
            $this->_PlayerData->_Exp += (int) $Exp;
        }

        // 4�ܰ�: ���� ó�� (���� �޸�)
        $this->_PlayerData->_PVP_LastConsecutiveVictory = (int) $this->_PlayerData->_PVP_ConsecutiveVictory;  // ���� ���¼��� �����ϰ�
        if ($VicType == "VIC")
        {
            if ($OriginalConsecutiveVictory > $this->_PlayerData->_PVP_TopConsecutiveVictory) // �ְ� ������ �����ߴٹ�
            {
                $this->_PlayerData->_PVP_TopConsecutiveVictory         = $OriginalConsecutiveVictory;
                $this->_PlayerData->_PVP_TopConsecutiveVictoryDateTime = (int) $currentTime;
            }

            // �޼��� �������� ����ޱ� ���� ������.
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

        // ���� ���� ó�� (���� �޸�)
        if ($GainRBattleSeasonPoint > 0)
        {
            $this->_PlayerData->_RBattleSeasonPoint += $GainRBattleSeasonPoint;
            $this->_PlayerData->_RBattleWarPoint += $GainRBattleWarPoint;

            AddResult('rbattle_point', $GainRBattleSeasonPoint);
            AddResult('rbattle_legionscore1', $LegionScore1);
            AddResult('rbattle_legionscore2', $LegionScore2);
        }

        // RDS�� ����� ����.
        GetScoreBoardObject()->UpdateScoreData(GetUserId(), $this->_PlayerData->_Level, $this->_PlayerData->_WeeklyAccumulateScore, $this->_PlayerData->_RecentScore,
            $this->_PlayerData->_PVP_ConsecutiveVictory, // ���� ����
            $this->_PlayerData->_EquipAircraft, $this->_PlayerData->_EquipSkin, $this->_PlayerData->_EquipTurret, $this->_PlayerData->_EquipPilot, $this->_PlayerData->_MsgRecvSwitch,
            $this->_PlayerData->_GiftRecvSwitch, $this->_PlayerData->_PPISwitch, $this->_PlayerData->_Nickname, true);

        if (CONFIG_GAME::SWITCH_DISPLAY_GLOBALRANKING == 1)
        {
            GetScoreBoardObject()->SetGlobalRank(GetUserId(), $this->_PlayerData->_WeeklyAccumulateScore);
        }

        return true;
    }