from application import cache, diff_db_time, SERVER_TYPE, gameTable, maf_log, gameEvent

from src.utils.DB import *
from src.utils.Common import *
from src.utils.InvenUtil import *
from src.utils.Protocol import *
from src.common.Abstract import *
from src.cache.User import *
from src.utils.InAppPurchase import *
from src.db.Ads import *

from datetime import datetime, timedelta
import src


#=============================================================================================
# SP : 소환 정보 얻어오기
#=============================================================================================
class sp_SummonGetData(baseTransaction):
    def __init__(self, useridx):
        self.useridx = useridx
        
    def execute(self, on_pb):
        
        on_pb.Result = Summon_pb2.OnSummonGetData.ResultType.FAIL
        
        day = GetDayID()
        
        with dbConnector() as commander:
            
            values_update = []
            
            rows = commander.execute('SELECT * FROM TB_USER_SUMMON WHERE useridx = %s', (str(self.useridx), ))
            if len(rows) > 0:
                for row in rows:
                    data = on_pb.Equipments.add()
                    data.Type = row['type']
                    data.Level = row['level']
                    data.Exp = row['exp']
                    data.RewardStep = row['reward_step']
                    data.AdsCount = row['ads_count']
                    data.AdsResetDay = row['ads_reset_day']
                    
                    if day != row['ads_reset_day']:
                        data.AdsCount = 0
                        data.AdsResetDay = day
                        
                        values_update.append([day, self.useridx, row['type']])
                        
                if len(values_update) > 0:
                    commander.executemany2('UPDATE TB_USER_SUMMON SET ads_count = 0, ads_reset_day = %s WHERE useridx = %s AND type = %s', values_update)

            if len(values_update) > 0:
                values_update.clear()
            
            rows = commander.execute('SELECT * FROM TB_USER_SUMMON_FLY WHERE useridx = %s', (str(self.useridx), ))
            if len(rows) > 0:
                for row in rows:
                    data = on_pb.Flies.add()
                    data.Index = row['idx']
                    data.Mileage = row['mileage']
                    data.AdsCount = row['ads_count']
                    data.AdsResetDay = row['ads_reset_day']
                    
                    if day != row['ads_reset_day']:
                        data.AdsCount = 0
                        data.AdsResetDay = day
                        
                        values_update.append([day, self.useridx, row['idx']])
                        
                if len(values_update) > 0:
                    commander.executemany2('UPDATE TB_USER_SUMMON_FLY SET ads_count = 0, ads_reset_day = %s WHERE useridx = %s AND idx = %s', values_update)
        
        on_pb.Result = Summon_pb2.OnSummonGetData.ResultType.SUCCESS


#=============================================================================================
# SP : 소환하기
#=============================================================================================
class sp_Summon(baseTransaction):
    def __init__(self, req_pb):
        self.useridx = req_pb.Useridx
        self.type = req_pb.Type
        self.index = req_pb.Index
        self.count = req_pb.Count
        self.cost_type = req_pb.CostType

    def execute(self, on_pb):
        
        if self.type == Common_pb2.CSummonType.EQUIPMENT:
            summonEquipment = sp_SummonEquipment(self.useridx, self.index, self.count, self.cost_type)
            summonEquipment.execute(on_pb)
            
            if on_pb.Result == Summon_pb2.OnSummon.ResultType.SUCCESS:
                
                summon_count = len(on_pb.SummonResults)
                
                # 무기 소환 N회 미션 처리 하기
                if self.index == Common_pb2.CSummonEquipmentIdx.WEAPON:
                    with dbConnector() as commander:
                        gameEvent.MissionOccur(commander, self.useridx, 0, "SUMMON_WEAPON", summon_count, False, on_pb.Missions)
                        
                        occur_list = [{ "type":QUEST_TYPE.SUMMON_WEAPON, "condition":0, "count":summon_count }]
                        src.db.Quest.QuestOccur(commander, self.useridx, gameTable, occur_list, on_pb.QuestInfo)
                
                # 방어구 소환 N회 미션 처리 하기
                elif self.index == Common_pb2.CSummonEquipmentIdx.ARMOR:
                    with dbConnector() as commander:
                        gameEvent.MissionOccur(commander, self.useridx, 0, "SUMMON_ARMOR", summon_count, False, on_pb.Missions)
                        
                        occur_list = [{ "type":QUEST_TYPE.SUMMON_ARMOR, "condition":0, "count":summon_count }]
                        src.db.Quest.QuestOccur(commander, self.useridx, gameTable, occur_list, on_pb.QuestInfo)
                
                # 장신구 소환 N회 미션 처리 하기
                elif self.index == Common_pb2.CSummonEquipmentIdx.BAG:
                    with dbConnector() as commander:
                        gameEvent.MissionOccur(commander, self.useridx, 0, "SUMMON_BAG", summon_count, False, on_pb.Missions)
                        
                        occur_list = [{ "type":QUEST_TYPE.SUMMON_ACCESSORY, "condition":0, "count":summon_count }]
                        src.db.Quest.QuestOccur(commander, self.useridx, gameTable, occur_list, on_pb.QuestInfo)

                # 광고 보기 N회 미션 처리 하기
                if self.cost_type == Common_pb2.CSummonCostType.ADS:
                    with dbConnector() as commander:
                        gameEvent.MissionOccur(commander, self.useridx, 0, "WATCH_ADV", 1, False, on_pb.Missions)
            
        elif self.type == Common_pb2.CSummonType.FLY:
            summonFly = sp_SummonFlyNew(self.useridx, self.index, self.count, self.cost_type)
            summonFly.execute(on_pb)
            
            if on_pb.Result == Summon_pb2.OnSummon.ResultType.SUCCESS:
                
                summon_count = len(on_pb.SummonResults)
                
                # 날파리 소환 N회 미션 처리 하기
                with dbConnector() as commander:
                    gameEvent.MissionOccur(commander, self.useridx, 0, "SUMMON_FLY", summon_count, False, on_pb.Missions)
                    
                    occur_list = [{ "type":QUEST_TYPE.SUMMON_FLY, "condition":0, "count":summon_count }]
                    occur_list.append({ "type":QUEST_TYPE.SUMMON_FLY_TOTAL, "condition":0, "count":summon_count })
                    src.db.Quest.QuestOccur(commander, self.useridx, gameTable, occur_list, on_pb.QuestInfo)
                
                # 광고 보기 N회 미션 처리 하기
                if self.cost_type == Common_pb2.CSummonCostType.ADS:
                    with dbConnector() as commander:
                        gameEvent.MissionOccur(commander, self.useridx, 0, "WATCH_ADV", 1, False, on_pb.Missions)
                    
        elif self.type == Common_pb2.CSummonType.PICKUP:
            from src.db.EventSummonPickup import sp_EventSummonPickupSummon
            summonPickup = sp_EventSummonPickupSummon(self.useridx, self.index, self.count, self.cost_type)
            summonPickup.execute(on_pb)
            
            if on_pb.Result == Summon_pb2.OnSummon.ResultType.SUCCESS:
                
                summon_count = len(on_pb.SummonResults)
                
                # 날파리 소환 N회 미션 처리 하기
                with dbConnector() as commander:
                    gameEvent.MissionOccur(commander, self.useridx, 0, "SUMMON_FLY", summon_count, False, on_pb.Missions)

        else:
            on_pb.Result = Summon_pb2.OnSummon.ResultType.FAIL
            return

#=============================================================================================
# SP : 소환하기 - 장비
#=============================================================================================
class sp_SummonEquipment(baseTransaction):
    def __init__(self, useridx, type, count, cost_type):
        self.useridx = useridx
        self.type = type
        self.count = count
        self.cost_type = cost_type
        
    def execute(self, on_pb):
        on_pb.Result = Summon_pb2.OnSummon.ResultType.FAIL
        
        helper = Helper(self.useridx)
        
        with dbConnector() as commander:
            # 1. 소환 정보 가져오기
            rows = commander.execute('SELECT * FROM TB_USER_SUMMON WHERE useridx = %s AND type = %s', (str(self.useridx), str(self.type), ))
            if len(rows) == 0:
                on_pb.Result = Summon_pb2.OnSummon.ResultType.FAIL
                return
            
            level = rows[0]['level']
            exp = rows[0]['exp']
            reward_step = rows[0]['reward_step']
            ads_count = rows[0]['ads_count']
            ads_reset_day = rows[0]['ads_reset_day']
            
            user_equipment_count_dict = helper.GetUserEquipment(commander)
            
            # 2. 소환 레벨 체크
            table700_summon_level_info = gameTable.get700SummonByTypeLevel(self.type, level)
            if len(table700_summon_level_info) == 0:
                table700_summon_level_info = gameTable.get700SummonByTypeLevel(self.type, level - 1)
                if len(table700_summon_level_info) == 0:
                    on_pb.Result = Summon_pb2.OnSummon.ResultType.NOT_CONDITION
                    return
            
            summon_count = self.count
            
            # 3. 비용 계산 / 비용 체크
            summon_cost_idx = RESOURCE_IDX.DIAMOND
            summon_cost = 0
            
            if self.cost_type == Common_pb2.CSummonCostType.COMMON: # 0 : 일반 재화
                if summon_count <= 0:
                    maf_log.addLog('log_alog', self.useridx, { 'msg' : 'sp_SummonEquipment : summon count is 0', 'cost type' : self.cost_type })
                    return
                
                max_summon_count = max(int(DEFINE.SUMMON_EQUIPMENT_1_COUNT), int(DEFINE.SUMMON_EQUIPMENT_2_COUNT), int(DEFINE.SUMMON_EQUIPMENT_3_COUNT))
                if max_summon_count < summon_count:
                    on_pb.Result = Summon_pb2.OnSummon.ResultType.NOT_CONDITION # 최대 횟수 초과
                    return
                
                summon_cost_idx = RESOURCE_IDX.DIAMOND
                summon_cost = summon_count * int(DEFINE.SUMMON_EQUIPMENT_PRICE)
                
                user_resource_count_dict = helper.GetUserResource(commander)
                user_resource_count = user_resource_count_dict.get(summon_cost_idx, 0)
                
                if user_resource_count < summon_cost:
                    on_pb.Result = Summon_pb2.OnSummon.ResultType.LACK_RESOURCE # 재화 부족
                    return
                
            elif self.cost_type == Common_pb2.CSummonCostType.TICKET: # 1 : 티켓
                if summon_count <= 0:
                    maf_log.addLog('log_alog', self.useridx, { 'msg' : 'sp_SummonEquipment : summon count is 0', 'cost type' : self.cost_type })
                    return
                
                max_summon_count = max(int(DEFINE.SUMMON_EQUIPMENT_1_COUNT), int(DEFINE.SUMMON_EQUIPMENT_2_COUNT), int(DEFINE.SUMMON_EQUIPMENT_3_COUNT))
                if max_summon_count < summon_count:
                    on_pb.Result = Summon_pb2.OnSummon.ResultType.NOT_CONDITION # 최대 횟수 초과
                    return
                
                user_item_count = 0
                
                if self.type == Common_pb2.CSummonEquipmentIdx.WEAPON:
                    summon_cost_idx = ITEM_IDX.SUMMON_WEAPON_TICKET
                    summon_cost = summon_count # 소환 1회당 티켓 1장씩 소모
                    
                    user_item_count = helper.GetUserItemCount(commander, summon_cost_idx)
                    
                elif self.type == Common_pb2.CSummonEquipmentIdx.ARMOR:
                    summon_cost_idx = ITEM_IDX.SUMMON_ARMOR_TICKET
                    summon_cost = summon_count # 소환 1회당 티켓 1장씩 소모
                    
                    user_item_count = helper.GetUserItemCount(commander, summon_cost_idx)
                    
                elif self.type == Common_pb2.CSummonEquipmentIdx.BAG:
                    summon_cost_idx = ITEM_IDX.SUMMON_BAG_TICKET
                    summon_cost = summon_count # 소환 1회당 티켓 1장씩 소모
                    
                    user_item_count = helper.GetUserItemCount(commander, summon_cost_idx)
                
                else:
                    maf_log.addLog('log_alog', self.useridx, { 'msg' : 'sp_SummonEquipment : summon type is incorrect', 'cost type' : self.cost_type })
                    return
                    
                if user_item_count < summon_cost:
                    on_pb.Result = Summon_pb2.OnSummon.ResultType.LACK_RESOURCE
                    return
                
            elif self.cost_type == Common_pb2.CSummonCostType.ADS: # 2 : 광고
                summon_count = int(DEFINE.SUMMON_EQUIPMENT_ADS_COUNT)
                
                ads_info = WatchAds(ads_count, ads_reset_day, int(DEFINE.SUMMON_ADS_LIMIT))
                if ads_info['result'] == False:
                    on_pb.Result = Summon_pb2.OnSummon.ResultType.NOT_SUMMON_ADS_COOL_TIME # 광고 소환 불가능
                    return

                ads_count = ads_info['ads_count']
                ads_reset_day = ads_info['ads_reset_day']
                
                summon_cost = 0 # FREE. 광고 소환은 비용이 들지 않음
                
            else:
                maf_log.addLog('log_alog', self.useridx, { 'msg' : 'sp_SummonEquipment : cost type is incorrect', 'cost type' : self.cost_type })
                return
                
        # 4. 소환 처리
        summon_results = []
        for i in range(summon_count):
            result_list = helper.Get107RandomBoxProbResult(table700_summon_level_info['summon_prob'], 1)
            
            if len(result_list) <= 0:
                maf_log.addLog('log_alog', self.useridx, { 'msg' : 'sp_SummonEquipment : summon result not found', 'prob idx' : table700_summon_level_info['summon_prob'] })
                return
            
            exp = exp + 1
            
            if exp >= table700_summon_level_info['next_count']:
                table700_next_summon_level_info = gameTable.get700SummonByTypeLevel(self.type, level + 1)
                if len(table700_next_summon_level_info) > 0:
                    exp = exp - table700_summon_level_info['next_count']
                    level = level + 1
                    
                    table700_summon_level_info = table700_next_summon_level_info

            result_idx = result_list[0]['idx']
            result_count = result_list[0]['count']
            
            if result_idx in user_equipment_count_dict:
                user_equipment_count_dict[result_idx] = user_equipment_count_dict[result_idx] + result_count
            else:
                user_equipment_count_dict[result_idx] = result_count
            
            summon_results.append({ 'idx' : result_idx, 'add_count' : result_count, 'result_count' : user_equipment_count_dict[result_idx], 'uid' : 0 })
            
        with dbConnector() as commander:
            # 5. 비용 차감
            if summon_cost > 0:
                helper.ConsumeUserResourceAsCommander(commander, RESOURCE_TYPE.GOODS, summon_cost_idx, summon_cost)
                if helper.Result == False:
                    commander.rollback()
                    on_pb.Result = Summon_pb2.OnSummon.ResultType.LACK_RESOURCE # 재화 부족
                    return
            
            # 6. 보상 지급 일괄 처리
            merged_summon_results = MergeHelperResultData(copy.deepcopy(summon_results))
            stmt = 'INSERT INTO TB_USER_EQUIPMENT (useridx, idx, `level`, count) VALUES (%s, %s, 1, %s) ON DUPLICATE KEY UPDATE count = VALUES(count)'
            values = [(self.useridx, equipment_idx, merged_summon_results[equipment_idx]['result_count']) for equipment_idx in merged_summon_results] # (유저IDX, 장비 IDX, 보유 장비 개수 + 새로 소환된 장비 개수)
            commander.Cursor.executemany(stmt, values)
            
            # 7. 소환 정보 업데이트
            commander.execute('UPDATE TB_USER_SUMMON SET `level` = %s, exp = %s, ads_count = %s, ads_reset_day = %s WHERE useridx = %s AND `type` = %s', (str(level), str(exp), str(ads_count), str(ads_reset_day), str(self.useridx), str(self.type), ))
            
        for item in helper.ResultDatas:
            data = on_pb.RewardItems.add()
            data.Idx = item['idx']
            data.AddCount = int(item['add_count'])
            data.ResultCount = int(item['result_count'])
            data.Uid = int(item['uid'])
            
        for item in summon_results:
            data = on_pb.SummonResults.add()
            data.Idx = item['idx']
            data.AddCount = int(item['add_count'])
            data.ResultCount = int(item['result_count'])
            data.Uid = int(item['uid'])
        
        data = on_pb.Equipment
        data.Type = self.type
        data.Level = level
        data.Exp = exp
        data.RewardStep = reward_step
        data.AdsCount = ads_count
        data.AdsResetDay = ads_reset_day

        # maf_log.addLog('log_summon_item', self.useridx, result)
        on_pb.Result = Summon_pb2.OnSummon.ResultType.SUCCESS

#=============================================================================================
# SP : 소환하기 - 날파리
#=============================================================================================
class sp_SummonFlyNew(baseTransaction):
    def __init__(self, useridx, index, count, cost_type):
        self.useridx = useridx
        self.index = index
        self.count = count
        self.cost_type = cost_type
        
    def execute(self, on_pb):
        on_pb.Result = Summon_pb2.OnSummon.ResultType.FAIL
        
        helper = Helper(self.useridx)
        
        with dbConnector() as commander:
            # 1. 테이블 체크
            table708_summon_fly_info = gameTable.get708SummonFlyByTypeIdx(0, self.index) # 0 : 날파리 소환
            if len(table708_summon_fly_info) == 0:
                on_pb.Result = Summon_pb2.OnSummon.ResultType.FAIL
                return
            
            summon_code = table708_summon_fly_info['code']
            
            # 2. 소환 정보 가져오기
            rows = commander.execute('SELECT * FROM TB_USER_SUMMON_FLY WHERE useridx = %s AND idx = %s', (str(self.useridx), str(self.index), ))
            if len(rows) == 0:
                on_pb.Result = Summon_pb2.OnSummon.ResultType.FAIL
                return
            
            mileage = rows[0]['mileage']
            ads_count = rows[0]['ads_count']
            ads_reset_day = rows[0]['ads_reset_day']
            
            summon_count = self.count
            
            # 3. 비용 계산 / 비용 체크
            summon_cost_idx = RESOURCE_IDX.DIAMOND
            summon_cost = 0
            
            if self.cost_type == Common_pb2.CSummonCostType.COMMON: # 0 : 일반 재화
                if summon_count <= 0:
                    maf_log.addLog('log_alog', self.useridx, { 'msg' : 'sp_SummonFlyNew : summon count is 0', 'cost type' : self.cost_type })
                    return
                
                max_summon_count = max(int(DEFINE.SUMMON_FLY_1_COUNT), int(DEFINE.SUMMON_FLY_3_COUNT))
                if max_summon_count < summon_count:
                    on_pb.Result = Summon_pb2.OnSummon.ResultType.NOT_CONDITION # 최대 횟수 초과
                    return
                
                summon_cost_idx = RESOURCE_IDX.DIAMOND
                summon_cost = summon_count * int(DEFINE.SUMMON_FLY_PRICE)
                
                user_resource_count_dict = helper.GetUserResource(commander)
                user_resource_count = user_resource_count_dict.get(summon_cost_idx, 0)
                
                if user_resource_count < summon_cost:
                    on_pb.Result = Summon_pb2.OnSummon.ResultType.LACK_RESOURCE # 재화 부족
                    return
            
            elif self.cost_type == Common_pb2.CSummonCostType.TICKET: # 1 : 소환 티켓
                if summon_count <= 0:
                    maf_log.addLog('log_alog', self.useridx, { 'msg' : 'sp_SummonFlyNew : summon count is 0', 'cost type' : self.cost_type })
                    return
                
                max_summon_count = max(int(DEFINE.SUMMON_FLY_1_COUNT), int(DEFINE.SUMMON_FLY_3_COUNT))
                if max_summon_count < summon_count:
                    on_pb.Result = Summon_pb2.OnSummon.ResultType.NOT_CONDITION # 최대 횟수 초과
                    return
                
                summon_cost_idx = ITEM_IDX.SUMMON_FLY_TICKET
                summon_cost = summon_count # 소환 1회당 티켓 1장씩 소모
                
                user_item_count = helper.GetUserItemCount(commander, summon_cost_idx)

                if user_item_count < summon_cost:
                    on_pb.Result = Summon_pb2.OnSummon.ResultType.LACK_RESOURCE
                    return
            
            elif self.cost_type == Common_pb2.CSummonCostType.ADS: # 2 : 광고
                summon_count = int(DEFINE.SUMMON_FLY_ADS_COUNT)
                
                ads_info = WatchAds(ads_count, ads_reset_day, int(DEFINE.SUMMON_ADS_LIMIT))
                if ads_info['result'] == False:
                    on_pb.Result = Summon_pb2.OnSummon.ResultType.NOT_SUMMON_ADS_COOL_TIME # 광고 소환 불가능
                    return

                ads_count = ads_info['ads_count']
                ads_reset_day = ads_info['ads_reset_day']
                
                summon_cost = 0 # FREE. 광고 소환은 비용이 들지 않음

            else:
                maf_log.addLog('log_alog', self.useridx, { 'msg' : 'sp_SummonFlyNew : cost type is incorrect', 'cost type' : self.cost_type })
                return
                
        # 4. 소환 처리
        summon_list = []
        for i in range(summon_count):
            fly_result = helper.Get708SummonFlyResultByCode(summon_code)
            if fly_result['result'] == False:
                maf_log.addLog('log_alog', self.useridx, { 'msg' : 'sp_SummonFlyNew : summon result not found', 'summon code' : summon_code })
                return
            mileage = mileage + 1
            for fly_idx in fly_result['items']:
                summon_list.append(fly_idx)
        
        with dbConnector() as commander:
            # 5. 비용 차감
            if summon_cost > 0:
                helper.ConsumeUserResourceAsCommander(commander, RESOURCE_TYPE.GOODS, summon_cost_idx, summon_cost)
                if helper.Result == False:
                    commander.rollback()
                    on_pb.Result = Summon_pb2.OnSummon.ResultType.LACK_RESOURCE
                    return
            
            # 6. 보상 지급 일괄 처리
            summon_results = []
            db_add_dic = {}
            for summon_fly_idx in summon_list:
                summon_results.append({'uid' : 0, 'idx' : summon_fly_idx, 'add_count' : 1, 'result_count' : 1})
                
                if summon_fly_idx in db_add_dic:
                    db_add_dic[summon_fly_idx] += 1
                else:
                    db_add_dic[summon_fly_idx] = 1

            for db_fly_idx, db_fly_cnt in db_add_dic.items():
                args = (self.useridx, db_fly_idx, db_fly_cnt, 0, 0)
                commander.callProc('SP_USER_FLY_NEW_ADD', args)
                
                db_result = commander.getResultArg(3)
                
                if db_result != 1: # 실패
                    maf_log.addLog('log_alog', self.useridx, { 'msg' : 'sp_SummonFlyNew : fly add fail', 'result' : db_result, 'idx' : db_fly_idx, 'cnt' : db_fly_cnt })
            
            # 7. 소환 정보 업데이트
            commander.execute('UPDATE TB_USER_SUMMON_FLY SET mileage = %s, ads_count = %s, ads_reset_day = %s WHERE useridx = %s AND idx = %s', (str(mileage), str(ads_count), str(ads_reset_day), str(self.useridx), str(self.index), ))
        
        for item in helper.ResultDatas:
            data = on_pb.RewardItems.add()
            data.Idx = item['idx']
            data.AddCount = int(item['add_count'])
            data.ResultCount = int(item['result_count'])
            data.Uid = int(item['uid'])
        
        for item in summon_results:
            data = on_pb.SummonResults.add()
            data.Idx = item['idx']
            data.AddCount = int(item['add_count'])
            data.ResultCount = int(item['result_count'])
            data.Uid = int(item['uid'])

        data = on_pb.Fly
        data.Index = self.index
        data.Mileage = mileage
        data.AdsCount = ads_count
        data.AdsResetDay = ads_reset_day

        on_pb.Result = Summon_pb2.OnSummon.ResultType.SUCCESS


#=============================================================================================
# SP : 소환 보상 받기
#=============================================================================================
class sp_SummonReward(baseTransaction):
    def __init__(self, req_pb):
        self.useridx = req_pb.Useridx
        self.type = req_pb.Type
        self.index = req_pb.Index
        
    def execute(self, on_pb):
        on_pb.Result = Summon_pb2.OnSummonReward.ResultType.FAIL
        on_pb.Type = self.type
        on_pb.Index = self.index
        
        helper = Helper(self.useridx)
        
        with dbConnector() as commander:
            if self.type == Common_pb2.CSummonType.EQUIPMENT:
                rows = commander.execute('SELECT `level`, exp, reward_step FROM TB_USER_SUMMON WHERE useridx = %s AND type = %s', (str(self.useridx), str(self.index), ))
                if len(rows) == 0:
                    on_pb.Result = Summon_pb2.OnSummonReward.ResultType.FAIL
                    return
                
                level = rows[0]['level']
                exp = rows[0]['exp']
                reward_step = rows[0]['reward_step']
                
                table700_summon_info_list = gameTable.get700SummonByType(self.index)
                if len(table700_summon_info_list) == 0:
                    on_pb.Result = Summon_pb2.OnSummonReward.ResultType.FAIL
                    return
                
                table700_summon_max_level_info = table700_summon_info_list[-1] # 소환 최종 레벨 정보
                
                is_reward = False # 천장 보상 수령 여부
                
                # 장비 천장 보상 수령 가능할 경우 수령
                if reward_step < level:
                    for step_level in range(reward_step + 1, level + 1):
                        table700_summon_level_info = gameTable.get700SummonByTypeLevel(self.index, step_level)
                        if len(table700_summon_level_info) == 0:
                            continue
                        
                        if table700_summon_level_info['reward_code'] == 0:
                            continue
                        
                        helper.SupplyUserResourceAsCommander(commander, IdxToResourceType(table700_summon_level_info['reward_code']), table700_summon_level_info['reward_code'], table700_summon_level_info['reward_count'])
                    
                    reward_step = level # 천장 수령 단계 갱신
                    
                    is_reward = True
                
                # 장비 천장 보상 모두 받았으며, 소환 최고 레벨 달성한 경우 천장 반복 보상 수령
                if reward_step == level and table700_summon_max_level_info['level'] == level:
                    need_exp = table700_summon_max_level_info['next_count']
                    
                    reward_count = int(exp / need_exp)
                    if reward_count > 0:
                        for i in range(reward_count): # exp -> 최고 레벨 소환 천장 보상으로 교환
                            exp = exp - need_exp
                            
                            helper.SupplyUserResourceAsCommander(commander, IdxToResourceType(table700_summon_max_level_info['reward_code']), table700_summon_max_level_info['reward_code'], table700_summon_max_level_info['reward_count'])
                            
                        is_reward = True
                    
                # 보상 수령 했으면 갱신
                if is_reward == True:
                    commander.execute('UPDATE TB_USER_SUMMON SET exp = %s, reward_step = %s WHERE useridx = %s AND `type` = %s', (str(exp), str(reward_step), str(self.useridx), str(self.index), ))
                    
                    # result['exp'] = exp
                    # result['reward_step'] = reward_step
                    # result['reward_item']  = helper.ResultDatas
                    
                    # maf_log.addLog('log_summon_item', self.useridx, result)
                    
                else:
                    on_pb.RewardStep = reward_step
                    on_pb.Result = Summon_pb2.OnSummonReward.ResultType.ALREADY_REWARD
                    return
                
                on_pb.Exp = exp
                on_pb.RewardStep = reward_step
                
                for item in helper.ResultDatas:
                    data = on_pb.RewardItems.add()
                    data.Idx = item['idx']
                    data.AddCount = int(item['add_count'])
                    data.ResultCount = int(item['result_count'])
                    data.Uid = int(item['uid'])
                
            elif self.type == Common_pb2.CSummonType.FLY:
                rows = commander.execute('SELECT mileage FROM TB_USER_SUMMON_FLY WHERE useridx = %s AND idx = %s', (str(self.useridx), str(self.index), ))
                if len(rows) == 0:
                    on_pb.Result = Summon_pb2.OnSummonReward.ResultType.FAIL
                    return

                need_mileage = int(DEFINE.SUMMON_FLY_CEILING_REWARD)
                mileage = rows[0]['mileage']
                
                reward_count = int(mileage / need_mileage)
                if reward_count > 0: # 날파리 천장 보상 수령 가능할 경우
                    fly_codes = []
                    for i in range(reward_count): # mileage -> 날파리 N 등급 교환
                        mileage = mileage - need_mileage
                        
                        fly_reward_list = helper.Get107RandomBoxProbResult(int(DEFINE.SUMMON_FLY_CEILING_REWARD_GROUP), 1)
                        if len(fly_reward_list) == 0:
                            on_pb.Result = Summon_pb2.OnSummonReward.ResultType.FAIL
                            return
                        
                        fly_reward = fly_reward_list[0]
                        fly_idx = fly_reward['idx']
                        
                        helper.SupplyUserResourceAsCommander(commander, RESOURCE_TYPE.FLY, fly_idx, 1)
                        fly_codes.append(fly_idx)
                else:
                    on_pb.Mileage = mileage
                    on_pb.Result = Summon_pb2.OnSummonReward.ResultType.ALREADY_REWARD
                    return

                commander.execute('UPDATE TB_USER_SUMMON_FLY SET mileage = %s WHERE useridx = %s AND idx = %s', (str(mileage), str(self.useridx), str(self.index), ))

                # result['mileage'] = mileage
                # result['reward_item']  = helper.ResultDatas
                
                on_pb.Mileage = mileage
                
                for item in helper.ResultDatas:
                    data = on_pb.RewardItems.add()
                    data.Idx = item['idx']
                    data.AddCount = int(item['add_count'])
                    data.ResultCount = int(item['result_count'])
                    data.Uid = int(item['uid'])
                
                # maf_log.addLog('log_summon_item', self.useridx, result)
                
            elif self.type == Common_pb2.CSummonType.PICKUP:
                from src.db.EventSummonPickup import sp_EventSummonPickupReward
                summonPickupReward = sp_EventSummonPickupReward(self.useridx, self.type, self.index)
                summonPickupReward.executeAsCommander(commander, on_pb)
                
                if on_pb.Result != Summon_pb2.OnSummonReward.ResultType.SUCCESS:
                    return
                
            else:
                return
            
        on_pb.Result = Summon_pb2.OnSummonReward.ResultType.SUCCESS