from src.utils.DB import *
from application import cache, diff_db_time, SERVER_TYPE, gameTable, gameEvent

from src.utils.Common import *
from src.utils.Protocol import *

from src.common.Abstract import *
from src.common.RESOURCE import *
from src.common.EVENT_TYPE import *

from src.db.EventAttendance import *
from src.db.EventBettingKnight import *
from src.db.EventSeasonPass import *
from src.db.EventExchangeShop import *
from src.db.EventSummonPickup import *
from src.db.EventDefence import *
from src.db.EventThreeMatchB import *
from src.db.EventSurvival import *

import src

def GetEventCommonTableInfo(event_type):
    '''
    #### 이벤트 공용 기능 사용 관련 테이블 정의
    - db_table_name -> DB 테이블 이름
    '''
    
    db_table_name = ""
    
    # 진상 디펜스
    if event_type in EVENT_TYPE.DEFENCE:
        db_table_name = "TB_USER_EVENT_DEFENCE"
    
    # 쓰리 매치 배틀
    elif event_type in EVENT_TYPE.THREE_MATCH_B:
        db_table_name = "TB_USER_EVENT_THREE_MATCH_B"
    
    # 딜리버리 서바이벌
    elif event_type in EVENT_TYPE.SURVIVAL:
        db_table_name = "TB_USER_EVENT_SURVIVAL"
    
    return db_table_name


def GetEventCommonMasteryInfo(event_type):
    '''
    #### 이벤트 마스터리 공용 기능 사용 관련 필요 테이블 컬럼 정의
    - db_table_name -> DB 테이블 이름
    - db_column_max_wave -> 최대 웨이브 DB 컬럼
    - db_column_mastery_point -> 마스터리 포인트 DB 컬럼
    - db_column_items_mastery -> 아이템 마스터리 DB 컬럼
    '''
    
    result = {}
    
    table_name = GetEventCommonTableInfo(event_type)
    if table_name != "":        
        result['db_table_name'] = table_name
        result['db_column_max_wave'] = "play_max_wave"
        result['db_column_mastery_point'] = "mastery_point"
        result['db_column_items_mastery'] = "items_mastery"
    
    return result


def GetEventCommonRankingInfo(event_type):
    '''
    #### 이벤트 랭킹 공용 기능 사용 관련 필요 테이블 컬럼 정의
    - db_table_name -> DB 테이블 이름
    - log_table_name -> LOG 테이블 이름
    - redis_table_name -> Redis 랭킹 테이블 이름
    '''
    
    result = {}
    
    table_name = GetEventCommonTableInfo(event_type)
    if table_name != "":
    
        # 진상 디펜스
        if event_type in EVENT_TYPE.DEFENCE:
            result['db_table_name'] = table_name
            result['log_table_name'] = "log_event_defence"
            result['redis_table_name'] = "defence"
        
        # 쓰리 매치 배틀
        elif event_type in EVENT_TYPE.THREE_MATCH_B:
            result['db_table_name'] = table_name
            result['log_table_name'] = "log_event_three_match_b"
            result['redis_table_name'] = "three_match_b"

        # 딜리버리 서바이벌
        elif event_type in EVENT_TYPE.SURVIVAL:
            result['db_table_name'] = table_name
            result['log_table_name'] = "log_event_survival"
            result['redis_table_name'] = "survival"
            
    return result

def GetEventCommonRankingRewardInfo(event_type):
    '''
    #### 이벤트 랭킹 보상 공용 기능 사용 관련 필요 테이블 컬럼 정의
    - db_table_name -> DB 테이블 이름
    - log_table_name -> LOG 테이블 이름
    - redis_table_name -> Redis 랭킹 테이블 이름
    - db_column_is_reward -> 랭킹 보상 수령 여부
    '''
    
    result = GetEventCommonRankingInfo(event_type)
    
    if len(result) > 0:
    
        # 진상 디펜스
        if event_type in EVENT_TYPE.DEFENCE:
            result['db_column_is_reward'] = "is_reward"
        
        # 쓰리 매치 배틀
        elif event_type in EVENT_TYPE.THREE_MATCH_B:
            result['db_column_is_reward'] = "is_reward"

        # 딜리버리 서바이벌
        elif event_type in EVENT_TYPE.SURVIVAL:
            result['db_column_is_reward'] = "is_reward"
            
        else:
            result = {}
            
    else:
        result = {}
            
    return result

def GetEventCommonExchangeInfo(event_type):
    '''
    #### 이벤트 교환 상점 공용 기능 사용 관련 필요 테이블 컬럼 정의
    - db_table_name -> DB 테이블 이름
    - db_column_exchange_point -> 교환 상점 포인트
    - db_column_items_exchange -> 교환 상점 구매 정보
    '''
    
    result = {}
    
    table_name = GetEventCommonTableInfo(event_type)
    if table_name != "":
    
        result['db_table_name'] = table_name
        result['db_column_exchange_point'] = "exchange_point"
        result['db_column_items_exchange'] = "items_exchange"
        
    return result

def GetEventCommonPassInfo(event_type):
    '''
    #### 이벤트 패스 공용 기능 사용 관련 필요 테이블 컬럼 정의
    - db_table_name -> DB 테이블 이름
    - db_column_is_purchase -> 패스 구매 여부 컬럼
    - db_column_reward_step_free -> 패스 보상 단계 (무료)
    - db_column_reward_step_paid -> 패스 보상 단계 (유료)
    - db_column_play_count -> 패스 보상 기준 횟수
    '''
    
    result = {}
    
    table_name = GetEventCommonTableInfo(event_type)
    if table_name != "":
    
        # 진상 디펜스
        if event_type in EVENT_TYPE.DEFENCE:
            result['db_table_name'] = table_name
            result['db_column_is_purchase'] = "is_purchase"
            result['db_column_reward_step_free'] = "reward_step_free"
            result['db_column_reward_step_paid'] = "reward_step_paid"
            result['db_column_play_count'] = "play_count"
            
        # 쓰리 매치 배틀
        if event_type in EVENT_TYPE.THREE_MATCH_B:
            result['db_table_name'] = table_name
            result['db_column_is_purchase'] = "is_purchase"
            result['db_column_reward_step_free'] = "reward_step_free"
            result['db_column_reward_step_paid'] = "reward_step_paid"
            result['db_column_play_count'] = "play_count"
            
        # 딜리버리 서바이벌
        if event_type in EVENT_TYPE.SURVIVAL:
            result['db_table_name'] = table_name
            result['db_column_is_purchase'] = "is_purchase"
            result['db_column_reward_step_free'] = "reward_step_free"
            result['db_column_reward_step_paid'] = "reward_step_paid"
            result['db_column_play_count'] = "play_count"
        
    return result


#=============================================================================================
# SP 활성화 된 이벤트 전체 정보 얻어오기
#=============================================================================================
class sp_EventGetData(baseTransaction):
    def __init__(self, req_pb):
        self.req_pb = req_pb
        self.useridx = req_pb.Useridx

    def execute(self, on_pb):
        
        for e in gameEvent.event_list:
        
            event_type = e['event_type']
            
            # 로그인에서 이미 끝난 이벤트의 보상을 처리하기 위해서 GetEventStatus 대신 GetEventRewardStatus 를 씀
            event_status = gameEvent.GetEventRewardStatus(event_type)

            if event_status == False:
                continue
            
            # 배달 레이스
            if event_type in EVENT_TYPE.DELIVERY_RACE:
                pass
                # spGetDeliveryRaceInfo = sp_GetDeliveryRaceInfo(self.useridx)  
                # result = spGetDeliveryRaceInfo.execute()
                
                # on_pb.DeliveryRaceInfo.Result = spGetDeliveryRaceInfo.out_result
                
                # on_pb.DeliveryRaceInfo.EventIdx = result['EventIdx']
                # on_pb.DeliveryRaceInfo.EventUid = result['EventUid']
                # on_pb.DeliveryRaceInfo.EventDayResetDate = result['EventDayResetDate']
                # on_pb.DeliveryRaceInfo.UserScore = result['UserScore']
                # on_pb.DeliveryRaceInfo.UserRank = result['UserRank']
                # on_pb.DeliveryRaceInfo.RaceRound = result['RaceRound']
                # on_pb.DeliveryRaceInfo.IsGetReward = result['IsGetReward']
                # on_pb.DeliveryRaceInfo.RaceElapsedSec = result['RaceElapsedSec']
                # on_pb.DeliveryRaceInfo.ScoreElapsedSec = result['ScoreElapsedSec']

                # for dummyIdx in result['DummyIdx']:
                #     on_pb.DeliveryRaceInfo.DummyIdx.append(dummyIdx)
            
            # 이벤트 출석체크
            elif event_type in EVENT_TYPE.ATTENDANCE:
                eventAttendanceGetData = sp_EventAttendanceGetData(self.useridx)
                eventAttendanceGetData.execute(on_pb.EventAttendanceGetData)
            
            # 배당의 기사
            elif event_type in EVENT_TYPE.BETTING_KNIGHT:
                eventBettingKnightGetData = sp_EventBettingKnightGetData(self.useridx)
                eventBettingKnightGetData.execute(on_pb.EventBettingKnightGetData)
                
            # 시즌 패스
            elif event_type in EVENT_TYPE.SEASON_PASS:
                seasonPassData = on_pb.EventSeasonPassGetData.add()
                eventSeasonPassGetData = sp_EventSeasonPassGetData(self.req_pb, event_type)
                eventSeasonPassGetData.execute(seasonPassData)

            # 교환 상점
            elif event_type in EVENT_TYPE.EXCHANGE_SHOP:
                exchangeShopData = on_pb.EventExchangeShopGetData.add()
                eventExchangeShopGetData = sp_EventExchangeShopGetData(self.req_pb, event_type)
                eventExchangeShopGetData.execute(exchangeShopData)
                
            # 픽업 소환
            elif event_type in EVENT_TYPE.SUMMON_PICKUP:
                summonPickupData = on_pb.EventSummonPickupGetData.add()
                eventSummonPickupGetData = sp_EventSummonPickupGetData(self.req_pb, event_type)
                eventSummonPickupGetData.execute(summonPickupData)
                
            # 진상 디펜스
            elif event_type in EVENT_TYPE.DEFENCE:
                defenceData = on_pb.EventDefenceGetData.add()
                eventDefenceGetData = sp_EventDefenceGetData(self.req_pb, event_type)
                eventDefenceGetData.execute(defenceData)

            # 쓰리 매치 배틀
            elif event_type in EVENT_TYPE.THREE_MATCH_B:
                threeMatchBData = on_pb.EventThreeMatchBGetData.add()
                eventThreeMatchBGetData = sp_EventThreeMatchBGetData(self.req_pb, event_type)
                eventThreeMatchBGetData.execute(threeMatchBData)
                
            # 딜리버리 서바이벌
            elif event_type in EVENT_TYPE.SURVIVAL:
                survivalData = on_pb.EventSurvivalGetData.add()
                survivalDataGetData = sp_EventSurvivalGetData(self.req_pb, event_type)
                survivalDataGetData.execute(survivalData)
        
        on_pb.Result = OnGetUserEvent_pb2.OnGetUserEvent.ResultType.SUCCESS


#=============================================================================================
# SP 이벤트 미션 보상 받기
#=============================================================================================
class sp_EventMissionReward(baseTransaction):
    def __init__(self, req_pb):
        self.useridx = req_pb.Useridx
        self.event_type = req_pb.EventType
        self.mission_idx = req_pb.MissionIdx
        self.is_all = req_pb.IsAll

    def execute(self, on_pb):
        
        # 현재 진행 중인 이벤트 인지 먼저 알아보자
        event_status = gameEvent.GetEventStatus(self.event_type)
        
        if event_status == False:
            on_pb.Result = OnEventMissionReward_pb2.OnEventMissionReward.ResultType.NOT_EVENT_DATE
            return
        
        with dbConnector() as commander:
            
            table_name = ""
            
            # 배당의 기사
            if self.event_type in EVENT_TYPE.BETTING_KNIGHT:
                table_name = "TB_USER_EVENT_BETTING_KNIGHT"

            if table_name != "":
                if gameEvent.EventIsCurrentEvent(commander, table_name, self.useridx, self.event_type) == False:
                    on_pb.Result = OnEventMissionReward_pb2.OnEventMissionReward.ResultType.NOT_EVENT_INVOLVE
                    return
        
            if self.is_all == False:
                reward_result = gameEvent.MissionReward(commander, self.useridx, self.event_type, event_status['mission_type'], self.mission_idx, on_pb.ResultItems, on_pb.Missions)
            else:
                reward_result = gameEvent.MissionRewardAll(commander, self.useridx, self.event_type, event_status['mission_type'], on_pb.ResultItems, on_pb.Missions)
            
            if reward_result['res_code'] == "TableNotFound":
                on_pb.Result = OnEventMissionReward_pb2.OnEventMissionReward.ResultType.FAIL
            elif reward_result['res_code'] == "NotComplete":
                on_pb.Result = OnEventMissionReward_pb2.OnEventMissionReward.ResultType.NOT_COMPLETE
            elif reward_result['res_code'] == "AlreadyReward":
                on_pb.Result = OnEventMissionReward_pb2.OnEventMissionReward.ResultType.ALREADY_REWARD
            elif reward_result['res_code'] == "ResetDailyMission":
                on_pb.Result = OnEventMissionReward_pb2.OnEventMissionReward.ResultType.RESET_DAILY_MISSION
            elif reward_result['res_code'] == "Success":
                on_pb.Result = OnEventMissionReward_pb2.OnEventMissionReward.ResultType.SUCCESS


#=============================================================================================
# SP 이벤트 마스터리 업그레이드 하기
#=============================================================================================
class sp_EventCommonMasteryUpgrade(baseTransaction):
    def __init__(self, req_pb):
        self.useridx = req_pb.Useridx
        self.event_type = req_pb.EventType
        self.mastery_number = req_pb.MasteryNumber

    def execute(self, on_pb):
        
        # 현재 진행 중인 이벤트 인지 먼저 알아보자
        event_status = gameEvent.GetEventStatus(self.event_type)
        
        if event_status == False:
            on_pb.Result = EventCommon_pb2.EventCommonResultType.NOT_EVENT_DATE # 이벤트 기간이 아님
            return

        on_pb.EventType = self.event_type
        on_pb.EventIdx = event_status['event_idx']
        on_pb.MasteryNumber = self.mastery_number
        
        # DB 테이블 정보 가져오기
        info = GetEventCommonMasteryInfo(self.event_type)
        if len(info) <= 0:
            on_pb.Result = EventCommon_pb2.EventCommonResultType.FAIL # 실패
            return
        
        db_table_name = info['db_table_name']
        db_column_max_wave = info['db_column_max_wave']
        db_column_mastery_point = info['db_column_mastery_point']
        db_column_items_mastery = info['db_column_items_mastery']
        
        table882_mastery_info = gameTable.get882EventMastery(MakeIdx(882, self.event_type, self.mastery_number))
    
        if len(table882_mastery_info) <= 0:
            on_pb.Result = EventCommon_pb2.EventCommonResultType.INCORRECT_DATA # 잘못 된 데이터
            return
        
        mastery_number = table882_mastery_info['idx']
        mastery_number_str = str(mastery_number)
        
        with dbConnector() as commander:
            
            rows = commander.execute("SELECT * FROM " + db_table_name + " WHERE useridx = %s AND event_type = %s", (self.useridx, self.event_type, ))
                
            if len(rows) <= 0:
                return
        
            row = rows[0]
            
            # DB 테이블에 컬럼이 있는 지 검사
            if "event_idx" not in row or \
                "event_uid" not in row or \
                db_column_max_wave not in row or \
                db_column_mastery_point not in row or \
                db_column_items_mastery not in row:
                on_pb.Result = EventCommon_pb2.EventCommonResultType.INCORRECT_DATA # 잘못 된 데이터
                return
            
            # 유저의 정보가 현재 진행중인 이벤트가 아님
            if gameEvent.IsInvolvedEvent(self.event_type, row['event_idx'], row['event_uid']) == False:
                on_pb.Result = EventCommon_pb2.EventCommonResultType.NOT_EVENT_DATE # 이벤트 기간이 아님
                return
            
            # DB 정보 가져오기
            db_max_wave = row[db_column_max_wave]
            db_mastery_point = row[db_column_mastery_point]
            db_items_mastery = row[db_column_items_mastery]
            
            # 마스터리 정보 가져오기
            mastery_dict = {}
            if db_items_mastery != None:
                mastery_dict = json.loads(db_items_mastery)
            
            # 마스터리 레벨 정보 가져오기
            mastery_lv = 0
            if mastery_number_str in mastery_dict:
                mastery_lv = mastery_dict[mastery_number_str]
            
            # 최대 레벨이면 리턴
            if mastery_lv >= table882_mastery_info['max_lv']:
                on_pb.Result = EventCommon_pb2.EventCommonResultType.MAX_LEVEL # 최대 레벨
                return
            
            # 마스터리 데이터 검사
            if len(table882_mastery_info['need_point']) <= mastery_lv:
                return
            
            # 필요 마스터리 검사
            need_mastery_point = table882_mastery_info['need_point'][mastery_lv]
            if db_mastery_point < need_mastery_point:
                on_pb.Result = EventCommon_pb2.EventCommonResultType.NOT_ENOUGH_MASTERY_POINT # 마스터리 포인트 부족
                return
            
            # 필요 웨이브 검사
            need_max_wave = table882_mastery_info['condition_wave_clear']
            if need_max_wave > 0: # 0 보다 크면 검사
                if db_max_wave < need_max_wave: # 웨이브 미 충족 시
                    on_pb.Result = EventCommon_pb2.EventCommonResultType.NOT_ENOUGH_WAVE # 웨이브 부족
                    return
            
            # 필요 마스터리 충족 검사
            need_mastery = table882_mastery_info['condition_mastery_code']
            if need_mastery > 0: # 0 보다 크면 검사
                need_mastery_str = str(need_mastery)
                if need_mastery_str not in mastery_dict:
                    on_pb.Result = EventCommon_pb2.EventCommonResultType.INCORRECT_DATA # 필요 마스터리 충족 못함
                    return
                
                need_mastery_lv = table882_mastery_info['condition_mastery_lv']
                if mastery_dict[need_mastery_str] < need_mastery_lv:
                    on_pb.Result = EventCommon_pb2.EventCommonResultType.INCORRECT_DATA # 필요 마스터리 레벨 충족 못함
                    return
            
            # 마스터리 레벨 증가 처리
            mastery_lv = mastery_lv + 1
            mastery_dict[mastery_number_str] = mastery_lv
            
            # DB 업데이트
            stmt = f"UPDATE {db_table_name} SET {db_column_mastery_point}={db_column_mastery_point}-%s, {db_column_items_mastery}=%s WHERE useridx=%s AND event_type=%s"
            commander.execute(stmt, (need_mastery_point, json.dumps(mastery_dict, ensure_ascii=False), self.useridx, self.event_type, ))
            
            on_pb.MasteryLv = mastery_lv
            on_pb.MasteryPoint = db_mastery_point - need_mastery_point
            on_pb.MasteryPointUsed = need_mastery_point
        
        on_pb.Result = EventCommon_pb2.EventCommonResultType.SUCCESS


#=============================================================================================
# SP 이벤트 랭킹 정보 가져오기
#=============================================================================================
class sp_EventCommonRanking(baseTransaction):
    def __init__(self, req_pb):
        self.useridx = req_pb.Useridx
        self.event_type = req_pb.EventType

    def execute(self, on_pb):

        # 현재 진행 중인 이벤트 인지 먼저 알아보자
        event_status = gameEvent.GetEventRewardStatus(self.event_type)
        
        if event_status == False:
            on_pb.Result = EventCommon_pb2.EventCommonResultType.NOT_EVENT_DATE # 이벤트 기간이 아님
            return

        on_pb.EventType = self.event_type
        on_pb.EventIdx = event_status['event_idx']
        
        # DB 테이블 정보 가져오기
        info = GetEventCommonRankingInfo(self.event_type)
        if len(info) <= 0:
            on_pb.Result = EventCommon_pb2.EventCommonResultType.FAIL # 실패
            return
        
        db_table_name = info['db_table_name']
        redis_table_name = info['redis_table_name']
        
        with dbConnector() as commander:
            
            rows = commander.execute("SELECT * FROM " + db_table_name + " WHERE useridx = %s AND event_type = %s", (self.useridx, self.event_type, ))
            
            if len(rows) <= 0:
                return
            
            row = rows[0]
            
            # DB 테이블에 컬럼이 있는 지 검사
            if "event_idx" not in row or \
                "event_uid" not in row:
                return
            
            # 유저의 정보가 현재 진행중인 이벤트가 아님
            if gameEvent.IsInvolvedEvent(self.event_type, row['event_idx'], row['event_uid']) == False:
                on_pb.Result = EventCommon_pb2.EventCommonResultType.NOT_EVENT_INVOLVE # 이벤트에 참여한 상태가 아님
                return
            
            user_cache = UserCache(self.useridx)
                
            cache_server = user_cache.GetUserServer(commander)
            if cache_server == None:
                return

            # 랭킹 가져오기
            ranks = cache.GetRankRange(f"{redis_table_name}_{row['event_uid']}", 50, 'rank', cache_server)
            for i in range(len(ranks)):
                ranker = on_pb.Rankers.add()
                ranker.Rank = i + 1
                ranker.Score = int(ranks[i][1])
                ranker.Nickname = user_cache.GetUserName(commander, ranks[i][0])
                if ranker.Nickname == None:
                    ranker.Nickname = ""
                ranker.Emblem = 0
                emblem_idx = user_cache.GetUserEmblemOther(commander, ranks[i][0])
                if emblem_idx != None:
                    ranker.Emblem = int(emblem_idx)

            myrank = cache.GetRank(f"{redis_table_name}_{row['event_uid']}", self.useridx, 'rank', cache_server)
            
            on_pb.MyRank = int(myrank)
            
        on_pb.Result = EventCommon_pb2.EventCommonResultType.SUCCESS


#=============================================================================================
# SP 이벤트 랭킹 보상 받기
#=============================================================================================
class sp_EventCommonRankingReward(baseTransaction):
    def __init__(self, useridx, event_type):
        self.useridx = useridx
        self.event_type = event_type
    
    def execute(self):
        with dbConnector() as commander:
            self.executeAsCommander(commander)
    
    def executeAsCommander(self, commander):
        
        # DB 테이블 정보 가져오기
        info = GetEventCommonRankingRewardInfo(self.event_type)
        if len(info) <= 0:
            return
        
        db_table_name = info['db_table_name']
        log_table_name = info['log_table_name']
        redis_table_name = info['redis_table_name']
        db_column_is_reward = info['db_column_is_reward']
            
        event_rows = commander.execute("SELECT * FROM " + db_table_name + " WHERE useridx = %s AND event_type = %s", (self.useridx, self.event_type, ))
    
        if len(event_rows) <= 0:
            return
        
        event_row = event_rows[0]
        
        # DB 테이블에 컬럼이 있는 지 검사
        if "event_idx" not in event_row or \
            "event_uid" not in event_row or \
            db_column_is_reward not in event_row:
            return
        
        if event_row[db_column_is_reward] == 1:
            return
        
        stmt = f"UPDATE {db_table_name} SET {db_column_is_reward}=1 WHERE useridx = %s AND event_type = %s"
        
        commander.execute(stmt, (self.useridx, self.event_type, ))

        # 이하 로그를 잘 남겨야 함 is_reward = 1 로 해버렸기 때문

        rank_info = gameEvent.GetEventRewardInfo(self.event_type, event_row['event_uid'])
        
        if len(rank_info) <= 0:
            maf_log.addLog(log_table_name, self.useridx, { 'msg' : 'EventRankingReward : rank reward info not found', 'event_type' : self.event_type, 'event_uid' : event_row['event_uid'] })
            return
        
        reward_list = gameTable.get880EventRankingListByType(rank_info['rank_reward_type'])
        
        if len(reward_list) <= 0:
            maf_log.addLog(log_table_name, self.useridx, { 'msg' : 'EventRankingReward : rank reward table info not found', 'event_type' : self.event_type, 'event_uid' : event_row['event_uid'], 'reward_type' : rank_info['rank_reward_type'] })
            return
        
        user_cache = UserCache(self.useridx)
        
        cache_server = user_cache.GetUserServer(commander)
        if cache_server == None:
            maf_log.addLog(log_table_name, self.useridx, { 'msg' : 'EventRankingReward : server not found', 'event_type' : self.event_type, 'event_uid' : event_row['event_uid'], 'reward_type' : rank_info['rank_reward_type'] })
            return

        myrank = cache.GetRank(f"{redis_table_name}_{event_row['event_uid']}", self.useridx, 'rank', cache_server)
        
        rankcount = cache.GetRankCount(f"{redis_table_name}_{event_row['event_uid']}", 'rank', cache_server)
        
        if myrank == 0 or rankcount == 0:
            maf_log.addLog(log_table_name, self.useridx, { 'msg' : 'EventRankingReward : my rank not found', 'event_type' : self.event_type, 'event_uid' : event_row['event_uid'], 'reward_type' : rank_info['rank_reward_type'] })
            return
        
        myrankratio = myrank / rankcount
        
        reward_info = {}
        
        for reward in reward_list:
            if reward['rank_min'] != 0 and reward['rank_max'] != 0:
                if myrank >= reward['rank_min'] and myrank <= reward['rank_max']:
                    reward_info = reward
                    break
            else:
                if myrankratio >= reward['rank_ratio'][0] and myrankratio <= reward['rank_ratio'][1]:
                    reward_info = reward
                    break
                
        if len(reward_info) <= 0:
            maf_log.addLog(log_table_name, self.useridx, { 'msg' : 'EventRankingReward : rank reward selected info not found', 'event_type' : self.event_type, 'event_uid' : event_row['event_uid'], 'reward_type' : rank_info['rank_reward_type'], 'myrank' : myrank, 'rankcount' : rankcount })
            return
        
        # 우편으로 지급
        post_title = str(reward_info['post_msg'])
        
        items = []
        for rcode, rcnt in zip(reward_info['reward_code'], reward_info['reward_cnt']):
            items.append({"idx" : rcode, "count" : rcnt})
        
        from src.db.Post import sp_CreateUserPost
        createPost = sp_CreateUserPost(self.useridx, post_title, items)
        createPost.execute()
        
        maf_log.addLog(log_table_name, self.useridx, { 'msg' : 'EventRankingReward : success', 'event_type' : self.event_type, 'event_uid' : event_row['event_uid'], 'reward_type' : rank_info['rank_reward_type'], 'myrank' : myrank, 'rankcount' : rankcount })


#=============================================================================================
# SP 이벤트 패스 결제 하기
#=============================================================================================
class sp_EventCommonPassPurchase(baseTransaction):
    def __init__(self, req_pb):
        self.useridx = req_pb.Useridx
        self.billcode = req_pb.BillCode
        self.platform = req_pb.Platform
        self.is_freepass = req_pb.IsFreePass
        
        orderid       = req_pb.Receipt.OrderId
        packageName   = req_pb.Receipt.PackageName
        productId     = req_pb.Receipt.ProductId
        purchaseTime  = req_pb.Receipt.PurchaseTime
        purchaseState = req_pb.Receipt.PurchaseState
        purchaseToken = req_pb.Receipt.PurchaseToken
        price         = req_pb.Receipt.Price
        priceCode     = req_pb.Receipt.PriceCode
        signature     = req_pb.Receipt.Signature
        receipt       = req_pb.Receipt.Receipt
        acknowledged  = req_pb.Receipt.Acknowledged

        self.receipt = { 'orderId' : orderid, 
            'packageName' : packageName, 
            'productId' : productId, 
            'purchaseTime' : purchaseTime,
            'purchaseState' : purchaseState,  
            'purchaseToken' : purchaseToken, 
            'price' : price, 
            'priceCode' : priceCode, 
            'signature' : signature, 
            'receipt' : receipt,
            'acknowledged' : acknowledged,
            'itemIdx' :  req_pb.BillCode }
        
        # Event800 기준임
        self.event_type = req_pb.EventType

    def execute(self, on_pb):
        # 현재 진행 중인 이벤트 인지 먼저 알아보자
        event_status = gameEvent.GetEventRewardStatus(self.event_type)
        
        if event_status == False:
            on_pb.Result = EventCommon_pb2.EventCommonResultType.NOT_EVENT_DATE # 이벤트 기간이 아님
            maf_log.addLog('log_shop_fail', self.useridx, { 'msg' : 'event_pass_fail (not event date)', 'event_type' : self.event_type })
            return

        on_pb.EventType = self.event_type
        on_pb.EventIdx = event_status['event_idx']
        
        # DB 테이블 정보 가져오기
        info = GetEventCommonPassInfo(self.event_type)
        if len(info) <= 0:
            on_pb.Result = EventCommon_pb2.EventCommonResultType.INCORRECT_DATA # 실패
            maf_log.addLog('log_shop_fail', self.useridx, { 'msg' : 'event_pass_fail (table not found 1)', 'event_type' : self.event_type })
            return
        
        db_table_name = info['db_table_name']
        db_column_is_purchase = info['db_column_is_purchase']
        
        with dbConnector() as commander:
            
            rows = commander.execute("SELECT * FROM " + db_table_name + " WHERE useridx = %s AND event_type = %s", (self.useridx, self.event_type, ))
                
            if len(rows) <= 0:
                on_pb.Result = EventCommon_pb2.EventCommonResultType.NOT_EVENT_INVOLVE # 참여 중이 아님
                return
        
            row = rows[0]
            
            # DB 테이블에 컬럼이 있는 지 검사
            if "event_idx" not in row or \
                "event_uid" not in row or \
                db_column_is_purchase not in row:
                on_pb.Result = EventCommon_pb2.EventCommonResultType.NOT_EVENT_INVOLVE # 참여 중이 아님
                return
            
            # 유저의 정보가 현재 진행중인 이벤트가 아님
            if gameEvent.IsInvolvedEvent(self.event_type, row['event_idx'], row['event_uid']) == False:
                on_pb.Result = EventCommon_pb2.EventCommonResultType.NOT_EVENT_INVOLVE # 이벤트에 참여한 상태가 아님
                return
            
            # 이미 결제함
            if row[db_column_is_purchase] == 1:
                on_pb.Result = EventCommon_pb2.EventCommonResultType.ALREADY_PURCHASE # 이미 결제한 상품
                return
        
        
        event_pass_idx = MakeIdx(881, self.event_type, 1)
        event_pass_info = gameTable.get881EventPass(event_pass_idx)
        
        # 테이블 정보를 찾을 수 없음
        if len(event_pass_info) <= 0:
            on_pb.Result = EventCommon_pb2.EventCommonResultType.INCORRECT_DATA # 실패
            maf_log.addLog('log_shop_fail', self.useridx, { 'msg' : 'event_pass_fail (table not found 2)', 'event_type' : self.event_type })
            return
        
        iap_info = gameTable.get808EventIAPByIdx(event_pass_info['event808iap_code'])
        
        # 테이블 정보를 찾을 수 없음
        if len(iap_info) <= 0:
            on_pb.Result = EventCommon_pb2.EventCommonResultType.INCORRECT_DATA # 실패
            maf_log.addLog('log_shop_fail', self.useridx, { 'msg' : 'event_pass_fail (table not found 3)', 'event_type' : self.event_type })
            return

        bill_code = iap_info['billcode_aos']
        if self.platform == 1:
            bill_code = iap_info['billcode_ios']
        
        # 결제 검증
        shopInApp = src.db.Shop.sp_ShopInApp()
        inAppResult = shopInApp.verify(self.useridx, bill_code, self.receipt, self.platform, self.is_freepass)
        if inAppResult == PURCHASE_CODE.NOT_CORRECT_PURCHASE:
            on_pb.Result = EventCommon_pb2.EventCommonResultType.INCORRECT_DATA
            maf_log.addLog('log_shop_fail', self.useridx, { 'msg' : 'event_pass_fail (table not found 3)', 'event_type' : self.event_type, 'iap_result' : inAppResult })
            return False
        elif inAppResult == PURCHASE_CODE.ALREADY_PURCHASE:
            on_pb.Result = EventCommon_pb2.EventCommonResultType.ALREADY_PURCHASE
            maf_log.addLog('log_shop_fail', self.useridx, { 'msg' : 'event_pass_fail (table not found 3)', 'event_type' : self.event_type, 'iap_result' : inAppResult })
            return False
        elif inAppResult == PURCHASE_CODE.BAD_REQUEST:
            on_pb.Result = EventCommon_pb2.EventCommonResultType.BAD_PURCHASE_REQUEST
            maf_log.addLog('log_shop_fail', self.useridx, { 'msg' : 'event_pass_fail (table not found 3)', 'event_type' : self.event_type, 'iap_result' : inAppResult })
            return False
        
        with dbConnector() as commander:
            # 패스 구매 처리
            stmt = f"UPDATE {db_table_name} SET {db_column_is_purchase} = 1 WHERE useridx = %s AND event_type = %s AND event_idx = %s AND event_uid = %s"
            commander.execute(stmt, (self.useridx, self.event_type, event_status['event_idx'], event_status['event_uid'], ))
        
        on_pb.IsPurchase = True
        
        on_pb.Result = EventCommon_pb2.EventCommonResultType.SUCCESS


#=============================================================================================
# SP 이벤트 패스 보상 받기
#=============================================================================================
class sp_EventCommonPassReward(baseTransaction):
    def __init__(self, req_pb):
        self.useridx = req_pb.Useridx
        self.event_type = req_pb.EventType

    def execute(self, on_pb):
        
        # 현재 진행 중인 이벤트 인지 먼저 알아보자
        event_status = gameEvent.GetEventRewardStatus(self.event_type)
        
        if event_status == False:
            on_pb.Result = EventCommon_pb2.EventCommonResultType.NOT_EVENT_DATE # 이벤트 기간이 아님
            return

        on_pb.EventType = self.event_type
        on_pb.EventIdx = event_status['event_idx']
        
        # DB 테이블 정보 가져오기
        info = GetEventCommonPassInfo(self.event_type)
        if len(info) <= 0:
            on_pb.Result = EventCommon_pb2.EventCommonResultType.FAIL # 실패
            return
        
        db_table_name = info['db_table_name']
        db_column_is_purchase = info['db_column_is_purchase']
        db_column_reward_step_free = info['db_column_reward_step_free']
        db_column_reward_step_paid = info['db_column_reward_step_paid']
        db_column_play_count = info['db_column_play_count']
        
        pass_reward_list = gameTable.get881EventPassListByType(self.event_type)
        if len(pass_reward_list) <= 0:
            return
        
        with dbConnector() as commander:
            
            rows = commander.execute("SELECT * FROM " + db_table_name + " WHERE useridx = %s AND event_type = %s", (self.useridx, self.event_type, ))
            
            if len(rows) <= 0:
                return

            row = rows[0]
            
            # DB 테이블에 컬럼이 있는 지 검사
            if "event_idx" not in row or \
                "event_uid" not in row or \
                db_column_is_purchase not in row or \
                db_column_reward_step_free not in row or \
                db_column_reward_step_paid not in row or \
                db_column_play_count not in row:
                return
            
            # 유저의 정보가 현재 진행중인 이벤트가 아님
            if gameEvent.IsInvolvedEvent(self.event_type, row['event_idx'], row['event_uid']) == False:
                on_pb.Result = EventCommon_pb2.EventCommonResultType.NOT_EVENT_INVOLVE # 이벤트에 참여한 상태가 아님
                return
            
            db_play_count = row[db_column_play_count] # 플레이 횟수
            db_is_purchase = row[db_column_is_purchase] # 구매 여부
            db_reward_step_free = row[db_column_reward_step_free] # 패스 보상 단계 (무료)
            db_reward_step_paid = row[db_column_reward_step_paid] # 패스 보상 단계 (유료)
            
            # 받을 수 있는 레벨 산출 (플레이 횟수 기준)
            reward_step_lv = 0
            for pass_reward in pass_reward_list:
                if pass_reward['condition'] <= db_play_count:
                    reward_step_lv = pass_reward['idx']
            
            is_update = False
            total_reward_dict = {}
            
            # 패스 무료 보상을 받을 수 있는 지
            if db_reward_step_free < reward_step_lv:
                for i in range(db_reward_step_free, reward_step_lv):
                    reward_idx = MakeIdx(881, self.event_type, i + 1)
                    reward_info = gameTable.get881EventPass(reward_idx)
                    
                    # 테이블 정보를 찾을 수 없음
                    if len(reward_info) <= 0:
                        maf_log.addLog('log_alog', self.useridx, { 'msg' : 'sp_EventPassReward : reward_info is 0', 'reward_idx' : reward_idx })
                        return
                    
                    reward_code = reward_info['free_reward_code']
                    reward_cnt = reward_info['free_reward_cnt']
                    
                    if reward_code in total_reward_dict:
                        total_reward_dict[reward_code] += reward_cnt
                    else:
                        total_reward_dict[reward_code] = reward_cnt
                
                db_reward_step_free = reward_step_lv
                
                is_update = True
                
            # 패스 유료 보상을 받을 수 있는 지
            if db_reward_step_paid < reward_step_lv and db_is_purchase == True:
                for i in range(db_reward_step_paid, reward_step_lv):
                    reward_idx = MakeIdx(881, self.event_type, i + 1)
                    reward_info = gameTable.get881EventPass(reward_idx)
                    
                    # 테이블 정보를 찾을 수 없음
                    if len(reward_info) <= 0:
                        maf_log.addLog('log_alog', self.useridx, { 'msg' : 'sp_EventPassReward : reward_info is 0', 'reward_idx' : reward_idx })
                        return
                    
                    reward_code = reward_info['pass_reward_code']
                    reward_cnt = reward_info['pass_reward_cnt']
                    
                    if reward_code in total_reward_dict:
                        total_reward_dict[reward_code] += reward_cnt
                    else:
                        total_reward_dict[reward_code] = reward_cnt
                
                db_reward_step_paid = reward_step_lv
                
                is_update = True
                
            if is_update == True:
                # 패스 정보 업데이트
                stmt = f"UPDATE {db_table_name} SET {db_column_reward_step_free} = %s, {db_column_reward_step_paid} = %s WHERE useridx = %s AND event_type = %s AND event_idx = %s AND event_uid = %s"
                commander.execute(stmt, (db_reward_step_free, db_reward_step_paid, self.useridx, self.event_type, row['event_idx'], row['event_uid'], ))
                
                helper = Helper(self.useridx)
                
                # 패스 보상 처리
                for total_reward_code, total_reward_cnt in total_reward_dict.items():
                    if RESOURCE_IDX.CARROT_OIL == total_reward_code:
                        helper.SupplyUserResourceExAsCommander(commander, IdxToResourceType(total_reward_code), total_reward_code, total_reward_cnt, False, True)
                    else:
                        helper.SupplyUserResourceExAsCommander(commander, IdxToResourceType(total_reward_code), total_reward_code, total_reward_cnt)
                    
                    if helper.Result == False:
                        commander.rollback()
                        return
                
                for r in helper.ResultDatas:
                    ritem = on_pb.RewardItems.add()
                    ritem.Idx = r['idx']
                    ritem.AddCount = r['add_count']
                    ritem.ResultCount = r['result_count']
            
            else: # 받을 수 있는 보상이 없음
                on_pb.Result = EventCommon_pb2.EventCommonResultType.ALREADY_REWARD # 받을 수 있는 보상이 없음
                return
            
            on_pb.RewardStepFree = db_reward_step_free
            on_pb.RewardStepPaid = db_reward_step_paid
            
        on_pb.Result = EventCommon_pb2.EventCommonResultType.SUCCESS


#=============================================================================================
# SP 이벤트 교환 상점 아이템 구매 하기
#=============================================================================================
class sp_EventCommonExchangeBuy(baseTransaction):
    def __init__(self, req_pb):
        self.useridx = req_pb.Useridx
        self.event_type = req_pb.EventType
        self.buy_slot_number = req_pb.BuySlotNumber
        self.buy_count = req_pb.BuyCount

    def execute(self, on_pb):
    
        # 현재 진행 중인 이벤트 인지 먼저 알아보자
        event_status = gameEvent.GetEventRewardStatus(self.event_type)
        
        if event_status == False:
            on_pb.Result = EventCommon_pb2.EventCommonResultType.NOT_EVENT_DATE # 이벤트 기간이 아님
            return

        on_pb.EventType = self.event_type
        on_pb.EventIdx = event_status['event_idx']
        on_pb.EventUid = event_status['event_uid']
        on_pb.BuySlotNumber = self.buy_slot_number
        
        if self.buy_count <= 0:
            return
        
        # DB 테이블 정보 가져오기
        info = GetEventCommonExchangeInfo(self.event_type)
        if len(info) <= 0:
            on_pb.Result = EventCommon_pb2.EventCommonResultType.FAIL # 실패
            return
        
        db_table_name = info['db_table_name']
        db_column_exchange_point = info['db_column_exchange_point']
        db_column_items_exchange = info['db_column_items_exchange']
        
        with dbConnector() as commander:
            
            rows = commander.execute("SELECT * FROM " + db_table_name + " WHERE useridx = %s AND event_type = %s", (self.useridx, self.event_type, ))
            
            if len(rows) <= 0:
                return
        
            row = rows[0]
            
            # DB 테이블에 컬럼이 있는 지 검사
            if "event_idx" not in row or \
                "event_uid" not in row or \
                db_column_exchange_point not in row or \
                db_column_items_exchange not in row:
                return
            
            # 유저의 정보가 현재 진행중인 이벤트가 아님
            if gameEvent.IsInvolvedEvent(self.event_type, row['event_idx'], row['event_uid']) == False:
                on_pb.Result = EventCommon_pb2.EventCommonResultType.NOT_EVENT_INVOLVE # 이벤트에 참여한 상태가 아님
                return
            
            db_exchange_point = row[db_column_exchange_point]
            db_items_exchange = row[db_column_items_exchange]
            
            # 테이블
            table883_exchange_info = gameTable.get883EventExchange(MakeIdx(883, self.event_type, self.buy_slot_number))

            # 잘못된 테이블 정보
            if len(table883_exchange_info) <= 0:
                on_pb.Result = EventCommon_pb2.EventCommonResultType.INCORRECT_DATA # 잘못 된 데이터
                return
            
            # 구매 상황
            item_code = table883_exchange_info['item_code']
            item_count = table883_exchange_info['item_count']
            price = table883_exchange_info['price']
            exchange_count = table883_exchange_info['exchange_count']
            
            slot_number_str = str(self.buy_slot_number)
            
            buyitem_dict = {}
            if db_items_exchange != None:
                buyitem_dict = json.loads(db_items_exchange)
            
            # 초과 구매 되었는지?
            bought_count = 0
            if slot_number_str in buyitem_dict:
                bought_count = buyitem_dict[slot_number_str]
            
            # 구매 가능한 상품이 남아있는 지 확인
            if bought_count + self.buy_count > exchange_count:
                on_pb.Result = EventCommon_pb2.EventCommonResultType.BUY_COUNT_LIMIT # 구매 최대 수량 초과
                return
            
            bought_count = bought_count + self.buy_count
            
            # 구매 포인트 보유 확인
            consume_point = price * self.buy_count
            if consume_point > db_exchange_point:
                on_pb.Result = EventCommon_pb2.EventCommonResultType.NOT_ENOUGH_EXCHANGE_POINT # 교환 재화 부족
                return
            
            # 포인트 소모 및 구매 상황 업데이트
            buyitem_dict[slot_number_str] = bought_count
            
            stmt = f"UPDATE {db_table_name} SET {db_column_exchange_point} = {db_column_exchange_point} - %s, {db_column_items_exchange} = %s WHERE useridx = %s AND event_type = %s"
            rows = commander.execute(stmt, (consume_point, json.dumps(buyitem_dict, ensure_ascii=False), self.useridx, self.event_type, ))

            on_pb.ExchangePoint = db_exchange_point - consume_point
            on_pb.BuyCount = bought_count
            
            from src.utils.InvenUtil import InvenUtil
            inven = InvenUtil(self.useridx)
            inven.SupplyCommander(commander, item_code, item_count * self.buy_count)
            inven.GetListToPb(on_pb.ResultItems)
            inven.WriteLog(0)
        
        on_pb.Result = EventCommon_pb2.EventCommonResultType.SUCCESS


#=============================================================================================
# SP 이벤트 게임 소탕 하기
#=============================================================================================
class sp_EventCommonGameAutoClear(baseTransaction):
    def __init__(self, req_pb):
        self.req_pb = req_pb
        self.event_type = req_pb.EventType

    def execute(self, on_pb):
        
        on_pb.EventType = self.event_type
        
        # 진상 디펜스
        if self.event_type in EVENT_TYPE.DEFENCE:
            sp_Packet = sp_EventDefenceGameAutoClearNew(self.req_pb)
            sp_Packet.execute(on_pb, on_pb.Defence)
            
        # 쓰리 매치 배틀
        elif self.event_type in EVENT_TYPE.THREE_MATCH_B:
            sp_Packet = sp_EventThreeMatchBGameAutoClearNew(self.req_pb)
            sp_Packet.execute(on_pb, on_pb.ThreeMatchB)

        # 딜리버리 서바이벌
        elif self.event_type in EVENT_TYPE.SURVIVAL:
            sp_Packet = sp_EventSurvivalGameAutoClearNew(self.req_pb)
            sp_Packet.execute(on_pb, on_pb.Survival)
            
        else:
            on_pb.Result = EventCommon_pb2.EventCommonResultType.FAIL # 실패
            return
            
        on_pb.Result = EventCommon_pb2.EventCommonResultType.SUCCESS
