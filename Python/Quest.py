from datetime import datetime, time, timedelta

from src.utils.DB import *
from application import cache, diff_db_time, SERVER_TYPE, gameTable

from src.utils.Protocol import *
from src.utils.Common import * 
from src.utils.InvenUtil import *
from src.common.Abstract import *
from src.db.Shop import *
from datetime import datetime


#=============================================================================================
# 퀘스트 관련 상수들
#=============================================================================================
class QUEST_CATEGORY(Enum):
    MAIN = 1
    DAY = 2
    DAY_COMPLETE = 3
    WEEK = 4
    WEEK_COMPLETE = 5
    ACHIEVE = 6

class QUEST_GATEGORY_SUB:
    MAIN_PROGRESS = 1
    MAIN_REPEAT = 2
    DAY_NORMAL = 1
    DAY_COMPLETE = 2
    WEEK_NORMAL = 3
    WEEK_COMPLETE = 4


#=============================================================================================
# SP : 퀘스트 정보 얻어오기
#=============================================================================================
class sp_QuestGetData(baseTransaction):
    def __init__(self, useridx):
        self.useridx = useridx
        
    def execute(self, on_pb):

        on_pb.Result = Quest_pb2.OnQuestGetData.ResultType.FAIL

        if QuestRefreshData(self.useridx, on_pb.QuestInfo) == True:
            on_pb.Result = Quest_pb2.OnQuestGetData.ResultType.SUCCESS
            



#=============================================================================================
# SP : 퀘스트 보상 받기
#=============================================================================================
class sp_QuestReward(baseTransaction):
    def __init__(self, req_pb):
        self.useridx = req_pb.Useridx
        self.quest_category = QUEST_CATEGORY(req_pb.QuestCategory)
        self.quest_code = req_pb.QuestCode
        
    def execute(self, on_pb):

        on_pb.Result = Quest_pb2.OnQuestReward.ResultType.FAIL

        if len(self.quest_code) <= 0:
            return

        inven = InvenUtil(self.useridx)

        db_main = {}

        with dbConnector() as commander: 
            rows = commander.execute('SELECT * FROM TB_USER_QUEST_MAIN WHERE useridx = %s', (self.useridx, ))
            if len(rows) <= 0:
                return
            
            db_main = rows[0]

        if self.quest_category == QUEST_CATEGORY.MAIN:
            self.__procMain(inven, db_main, on_pb)
        elif self.quest_category == QUEST_CATEGORY.DAY:
            self.__procDay(inven, db_main, on_pb)
        elif self.quest_category == QUEST_CATEGORY.DAY_COMPLETE:
            self.__procDayComplete(inven, db_main, on_pb)
        elif self.quest_category == QUEST_CATEGORY.WEEK:
            self.__procWeek(inven, db_main, on_pb)
        elif self.quest_category == QUEST_CATEGORY.WEEK_COMPLETE:
            self.__procWeekComplete(inven, db_main, on_pb)
        elif self.quest_category == QUEST_CATEGORY.ACHIEVE:
            self.__procAchieve(inven, db_main, on_pb)

        inven.GetGainListToPb(on_pb.RewardItems)
        inven.GetListToPb(on_pb.ResultItems)

    # 패킷 채우기 : 메인 퀘스트
    def __fillMainPacket(self, pb_quest, db_main):
        data = pb_quest.Main.add()
        data.Code = db_main['idx']
        data.CompleteCount = db_main['complete_count']
        data.RewardStep = db_main['repeat_count'] # 메인 퀘스트 에서는 RewardStep 변수를 repeat_count 로 사용함
        pb_quest.UpdateDayID = db_main['update_day_id']
        pb_quest.RewardDayID = db_main['reward_day_id']
        pb_quest.UpdateWeekID = db_main['update_week_id']
        pb_quest.RewardWeekID = db_main['reward_week_id']

    # 메인 퀘스트
    def __procMain(self, inven:InvenUtil, db_main, on_pb):

        current_code = self.quest_code[0]

        if current_code != db_main["idx"]:
            return
        
        tb_main = gameTable.getDataByName('QuestMain516', db_main["idx"])
        if len(tb_main) <= 0:
            return

        with dbConnector() as commander:
            
            # 퀘스트 완료 검증
            need_count = tb_main['count']
            if db_main["repeat_count"] > 0:
                need_count = tb_main['count'] + (tb_main['count_add'] * (db_main["repeat_count"] - 1))
                if need_count > tb_main['count_max']:
                    need_count = tb_main['count_max']
            
            if QuestVerify(commander, self.useridx, db_main['complete_count'], tb_main['quest_type'], tb_main['is_history'], tb_main['condition'], need_count) == False:
                return

            # 퀘스트 정보 업데이트
            next_quest = current_code + 1
            tb_next = gameTable.getDataByName('QuestMain516', next_quest)
            next_quest_code = 0
            next_repeat_count = db_main["repeat_count"]
            if len(tb_next) > 0:
                next_quest_code = tb_next["code"]
            else:
                next_repeat_count += 1
                tb_repeat_list = gameTable.getDataByName('QuestMain516_type', QUEST_GATEGORY_SUB.MAIN_REPEAT)
                if len(tb_repeat_list) <= 0:
                    return
                next_quest_code = tb_repeat_list[0]["code"]
            
            commander.execute('UPDATE TB_USER_QUEST_MAIN SET idx = %s, complete_count=0, repeat_count=%s WHERE useridx = %s', (next_quest_code, next_repeat_count, self.useridx, ))
            
            db_main['idx'] = next_quest_code
            db_main['complete_count'] = 0
            db_main['repeat_count'] = next_repeat_count
            
            # 보상 지급
            if tb_main["reward_item"] > 0:
                inven.SupplyCommander(commander, tb_main["reward_item"], tb_main["reward_count"])

        # 패킷 채우기 : 메인 퀘스트
        self.__fillMainPacket(on_pb.QuestInfo, db_main)

        on_pb.Result = Quest_pb2.OnQuestReward.ResultType.SUCCESS

    # 일일 퀘스트
    def __procDay(self, inven:InvenUtil, db_main, on_pb):

        dayID = GetDayID()
        
        # 날짜 갱신 됨
        if db_main["update_day_id"] != dayID:
            QuestRefreshData(self.useridx, on_pb.QuestInfo)
            on_pb.Result = Quest_pb2.OnQuestReward.ResultType.RENEW_DATE
            return

        db_day = []

        with dbConnector() as commander:

            db_day = ExecuteSelectList(commander, self.useridx, '*', 'TB_USER_QUEST_DAY', 'idx', self.quest_code)
            if len(db_day) != len(self.quest_code):
                return
            
            values_day = []
            
            # 퀘스트 완료 검증 및 패킷 데이터 구성
            for dbday in db_day:
                if dbday["reward_step"] != 0:
                    return
                tb = gameTable.getDataByName('QuestPeriod517', dbday["idx"])
                if len(tb) <= 0:
                    return
                # 맞는 코드가 아님
                if tb["type"] != QUEST_GATEGORY_SUB.DAY_NORMAL:
                    return
                if QuestVerify(commander, self.useridx, dbday['complete_count'], tb['quest_type'], tb['is_history'], tb['condition'], tb['count']) == False:
                    return
                dbday["reward_step"] += 1
                values_day.append([self.useridx, dbday['idx']])
                data_day = on_pb.QuestInfo.Day.add()
                data_day.Code = dbday['idx']
                data_day.CompleteCount = dbday['complete_count']
                data_day.RewardStep = dbday['reward_step']
                # 보상 수집
                for i in range(0, len(tb["reward_item"])):
                    inven.AddInvenReserve(tb["reward_item"][i], tb["reward_count"][i])

            # 퀘스트 정보 업데이트
            if len(values_day) > 0:
                commander.executemany2("UPDATE TB_USER_QUEST_DAY SET reward_step = reward_step + 1 WHERE useridx = %s AND idx = %s", values_day)
            
            # 보상 지급
            inven.SupplyInvenReserve(commander)

        # 패킷 채우기 : 메인 퀘스트
        self.__fillMainPacket(on_pb.QuestInfo, db_main)

        on_pb.Result = Quest_pb2.OnQuestReward.ResultType.SUCCESS

    # 일일 퀘스트 완료
    def __procDayComplete(self, inven:InvenUtil, db_main, on_pb):

        if len(self.quest_code) != 1:
            return

        dayID = GetDayID()
        
        # 이미 완료 함
        if db_main["reward_day_id"] == dayID:
            return
        
        # 날짜 갱신 됨
        if db_main["update_day_id"] != dayID:
            QuestRefreshData(self.useridx, on_pb.QuestInfo)
            on_pb.Result = Quest_pb2.OnQuestReward.ResultType.RENEW_DATE
            return

        tb = gameTable.getDataByName('QuestPeriod517', self.quest_code[0])
        if len(tb) <= 0:
            return

        # 맞는 코드가 아님
        if tb["type"] != QUEST_GATEGORY_SUB.DAY_COMPLETE:
            return

        with dbConnector() as commander:

            # 퀘스트 완료 검증 및 패킷 데이터 구성
            if QuestVerify(commander, self.useridx, 0, tb['quest_type'], tb['is_history'], tb['condition'], tb['count']) == False:
                return

            # 일일 퀘스트 완료 상태 저장
            commander.execute("UPDATE TB_USER_QUEST_MAIN SET reward_day_id = %s WHERE useridx = %s", (dayID, self.useridx, ))

            db_main["reward_day_id"] = dayID

            # 보상 수집
            for i in range(0, len(tb["reward_item"])):
                inven.AddInvenReserve(tb["reward_item"][i], tb["reward_count"][i])

            # 보상 지급
            inven.SupplyInvenReserve(commander)

        # 패킷 채우기 : 메인 퀘스트
        self.__fillMainPacket(on_pb.QuestInfo, db_main)

        on_pb.Result = Quest_pb2.OnQuestReward.ResultType.SUCCESS

    # 주간 퀘스트
    def __procWeek(self, inven:InvenUtil, db_main, on_pb):
        
        weekID = GetWeekID()
        
        # 날짜 갱신 됨
        if db_main["update_week_id"] != weekID:
            QuestRefreshData(self.useridx, on_pb.QuestInfo)
            on_pb.Result = Quest_pb2.OnQuestReward.ResultType.RENEW_DATE
            return
        
        db_week = []

        with dbConnector() as commander:

            db_week = ExecuteSelectList(commander, self.useridx, '*', 'TB_USER_QUEST_WEEK', 'idx', self.quest_code)
            if len(db_week) != len(self.quest_code):
                return
            
            values_week = []
            
            # 퀘스트 완료 검증 및 패킷 데이터 구성
            for dbweek in db_week:
                if dbweek["reward_step"] != 0:
                    return
                tb = gameTable.getDataByName('QuestPeriod517', dbweek["idx"])
                if len(tb) <= 0:
                    return
                # 맞는 코드가 아님
                if tb["type"] != QUEST_GATEGORY_SUB.WEEK_NORMAL:
                    return
                if QuestVerify(commander, self.useridx, dbweek['complete_count'], tb['quest_type'], tb['is_history'], tb['condition'], tb['count']) == False:
                    return
                dbweek["reward_step"] += 1
                values_week.append([self.useridx, dbweek['idx']])
                data_week = on_pb.QuestInfo.Week.add()
                data_week.Code = dbweek['idx']
                data_week.CompleteCount = dbweek['complete_count']
                data_week.RewardStep = dbweek['reward_step']
                # 보상 수집
                for i in range(0, len(tb["reward_item"])):
                    inven.AddInvenReserve(tb["reward_item"][i], tb["reward_count"][i])

            # 퀘스트 정보 업데이트
            if len(values_week) > 0:
                commander.executemany2("UPDATE TB_USER_QUEST_WEEK SET reward_step = reward_step + 1 WHERE useridx = %s AND idx = %s", values_week)
            
            # 보상 지급
            inven.SupplyInvenReserve(commander)

        # 패킷 채우기 : 메인 퀘스트
        self.__fillMainPacket(on_pb.QuestInfo, db_main)
        
        on_pb.Result = Quest_pb2.OnQuestReward.ResultType.SUCCESS

    # 주간 퀘스트 완료
    def __procWeekComplete(self, inven:InvenUtil, db_main, on_pb):
        
        if len(self.quest_code) != 1:
            return

        weekID = GetWeekID()
        
        # 이미 완료 함
        if db_main["reward_week_id"] == weekID:
            return
        
        # 날짜 갱신 됨
        if db_main["update_week_id"] != weekID:
            QuestRefreshData(self.useridx, on_pb.QuestInfo)
            on_pb.Result = Quest_pb2.OnQuestReward.ResultType.RENEW_DATE
            return

        tb = gameTable.getDataByName('QuestPeriod517', self.quest_code[0])
        if len(tb) <= 0:
            return

        # 맞는 코드가 아님
        if tb["type"] != QUEST_GATEGORY_SUB.WEEK_COMPLETE:
            return

        with dbConnector() as commander:

            # 퀘스트 완료 검증 및 패킷 데이터 구성
            if QuestVerify(commander, self.useridx, 0, tb['quest_type'], tb['is_history'], tb['condition'], tb['count']) == False:
                return

            # 일일 퀘스트 완료 상태 저장
            commander.execute("UPDATE TB_USER_QUEST_MAIN SET reward_week_id = %s WHERE useridx = %s", (weekID, self.useridx, ))

            db_main["reward_week_id"] = weekID

            # 보상 수집
            for i in range(0, len(tb["reward_item"])):
                inven.AddInvenReserve(tb["reward_item"][i], tb["reward_count"][i])

            # 보상 지급
            inven.SupplyInvenReserve(commander)

        # 패킷 채우기 : 메인 퀘스트
        self.__fillMainPacket(on_pb.QuestInfo, db_main)

        on_pb.Result = Quest_pb2.OnQuestReward.ResultType.SUCCESS

    # 업적
    def __procAchieve(self, inven:InvenUtil, db_main, on_pb):
        
        db_achieve = []

        with dbConnector() as commander:

            db_achieve = ExecuteSelectList(commander, self.useridx, '*', 'TB_USER_QUEST_ACHIEVE', 'idx', self.quest_code)
            
            values_insert = []
            values_update = []
            
            # 퀘스트 완료 검증 및 패킷 데이터 구성
            for qcode in self.quest_code:
                tb = gameTable.getDataByName('QuestAchieve518', qcode)
                if len(tb) <= 0:
                    return
                
                # 완료 할 업적 단계 인덱스 찾아내기
                step_idx = 0
                
                userachieve = QuestFindInList(qcode, db_achieve)
                if userachieve != None:
                    step_idx = userachieve["reward_step"]

                # 이미 완료 됨
                if step_idx >= len(tb["count"]):
                    return

                reward_step = step_idx + 1

                if QuestVerify(commander, self.useridx, 0, tb['quest_type'], tb['is_history'], tb['condition'], tb['count'][step_idx]) == False:
                    return
                
                if userachieve == None:
                    values_insert.append([self.useridx, qcode, reward_step])
                else:
                    values_update.append([self.useridx, qcode])
                
                data_week = on_pb.QuestInfo.Achieve.add()
                data_week.Code = qcode
                data_week.CompleteCount = 0
                data_week.RewardStep = reward_step
                
                # 보상 수집 : 해당 스텝의 보상을 수집함
                inven.AddInvenReserve(tb["reward_item"][step_idx], tb["reward_count"][step_idx])

            # 퀘스트 정보 업데이트
            if len(values_insert) > 0:
                commander.executemany2("INSERT INTO TB_USER_QUEST_ACHIEVE (useridx, idx, reward_step) VALUES (%s, %s, %s)", values_insert)
            if len(values_update) > 0:
                commander.executemany2("UPDATE TB_USER_QUEST_ACHIEVE SET reward_step = reward_step + 1 WHERE useridx = %s AND idx = %s", values_update)
            
            # 보상 지급
            inven.SupplyInvenReserve(commander)

        # 패킷 채우기 : 메인 퀘스트
        self.__fillMainPacket(on_pb.QuestInfo, db_main)
        
        on_pb.Result = Quest_pb2.OnQuestReward.ResultType.SUCCESS


#=============================================================================================
# FUNC 목록에서 찾기
#=============================================================================================
def QuestFindInList(find_idx, quest_list):
    for info in quest_list:
        if info['idx'] == find_idx:
            return info
    return None

def QuestContainInList(find_type, quest_list):
    for info in quest_list:
        if info['type'] == find_type:
            return True
    return False


#=============================================================================================
# FUNC 갱신 된 퀘스트 정보 얻기
#=============================================================================================
def QuestRefreshData(useridx, pb_quest):
    
    dayID = GetDayID()
    weekID = GetWeekID()
    
    db_main = {}
    db_day = []
    db_week = []
    db_achieve = []
    db_history = {}

    tb_main_list = gameTable.getDataByName('QuestMain516_type', QUEST_GATEGORY_SUB.MAIN_PROGRESS)
    if len(tb_main_list) <= 0:
        return False
    tb_main = tb_main_list[0]
    
    tb_day = gameTable.getDataByName('QuestPeriod517_type', QUEST_GATEGORY_SUB.DAY_NORMAL)
    tb_week = gameTable.getDataByName('QuestPeriod517_type', QUEST_GATEGORY_SUB.WEEK_NORMAL)
    
    with dbConnector() as commander:
        
        need_main_update = False
        
        # 메인 퀘스트
        rows = commander.execute('SELECT * FROM TB_USER_QUEST_MAIN WHERE useridx = %s', (useridx, ))
        if len(rows) > 0:
            db_main = rows[0]
        else:
            commander.execute('INSERT INTO TB_USER_QUEST_MAIN (useridx, idx, update_day_id, update_week_id) VALUES(%s, %s, %s, %s)', (useridx, tb_main['code'], dayID, weekID, ))
            db_main['idx'] = tb_main['code']
            db_main['complete_count'] = 0
            db_main['repeat_count'] = 0
            db_main['update_day_id'] = dayID
            db_main['reward_day_id'] = 0
            db_main['update_week_id'] = weekID
            db_main['reward_week_id'] = 0

        # 일일 퀘스트 : 목록 검증 후 누락 퀘스트 추가 하기
        db_day = commander.execute('SELECT * FROM TB_USER_QUEST_DAY WHERE useridx = %s', (useridx, ))
        for tb in tb_day:
            userday = QuestFindInList(tb['code'], db_day)
            if userday == None:
                commander.execute('INSERT INTO TB_USER_QUEST_DAY (useridx, idx) VALUES(%s, %s)', (str(useridx), str(tb['code']), ))
        if db_main['update_day_id'] != dayID:
            commander.execute('UPDATE TB_USER_QUEST_DAY SET complete_count = 0, reward_step = 0 WHERE useridx = %s', (useridx, ))
            db_day = []
            need_main_update = True
            db_main['update_day_id'] = dayID
            db_main['reward_day_id'] = 0

        # 주간 퀘스트 : 목록 검증 후 누락 퀘스트 추가 하기
        db_week = commander.execute('SELECT * FROM TB_USER_QUEST_WEEK WHERE useridx = %s', (useridx, ))
        for tb in tb_week:
            userweek = QuestFindInList(tb['code'], db_week)
            if userweek == None:
                commander.execute('INSERT INTO TB_USER_QUEST_WEEK (useridx, idx) VALUES(%s, %s)', (str(useridx), str(tb['code']), ))
        if db_main['update_week_id'] != weekID:
            commander.execute('UPDATE TB_USER_QUEST_WEEK SET complete_count = 0, reward_step = 0 WHERE useridx = %s', (useridx, ))
            db_week = []
            need_main_update = True
            db_main['update_week_id'] = weekID
            db_main['reward_week_id'] = 0
        
        # 업적
        db_achieve = commander.execute('SELECT * FROM TB_USER_QUEST_ACHIEVE WHERE useridx = %s', (useridx, ))
        
        # 히스토리
        rows = commander.execute('SELECT * FROM TB_USER_QUEST_HISTORY WHERE useridx = %s', (useridx, ))
        if len(rows) > 0:
            db_history = rows[0]
        else:
            commander.execute('INSERT INTO TB_USER_QUEST_HISTORY (useridx) VALUES(%s)', (useridx, ))
            db_history['artifact_box_open'] = 0
            db_history['summon_fly_total'] = 0

        if need_main_update == True:
            commander.execute('UPDATE TB_USER_QUEST_MAIN SET update_day_id=%s, reward_day_id=%s, update_week_id=%s, reward_week_id=%s WHERE useridx = %s'
                            , (db_main['update_day_id'], db_main['reward_day_id'], db_main['update_week_id'], db_main['reward_week_id'], useridx, ))


    # 패킷 채우기 : 메인 퀘스트
    data = pb_quest.Main.add()
    data.Code = db_main['idx']
    data.CompleteCount = db_main['complete_count']
    data.RewardStep = db_main['repeat_count'] # 메인 퀘스트 에서는 RewardStep 변수를 repeat_count 로 사용함
    pb_quest.UpdateDayID = db_main['update_day_id']
    pb_quest.RewardDayID = db_main['reward_day_id']
    pb_quest.UpdateWeekID = db_main['update_week_id']
    pb_quest.RewardWeekID = db_main['reward_week_id']
    
    # 패킷 채우기 : 일일 퀘스트
    for info in db_day:
        data = pb_quest.Day.add()
        data.Code = info['idx']
        data.CompleteCount = info['complete_count']
        data.RewardStep = info['reward_step']

    # 패킷 채우기 : 주간 퀘스트
    for info in db_week:
        data = pb_quest.Week.add()
        data.Code = info['idx']
        data.CompleteCount = info['complete_count']
        data.RewardStep = info['reward_step']

    # 패킷 채우기 : 업적
    for info in db_achieve:
        data = pb_quest.Achieve.add()
        data.Code = info['idx']
        data.CompleteCount = 0 # 업적 에서는완료 카운트가 없음
        data.RewardStep = info['reward_step']

    # 패킷 채우기 : 히스토리
    data = pb_quest.History.add()
    data.Type = QUEST_HISTORY_TYPE.ARTIFACT_BOX_OPEN
    data.AddCount = db_history['artifact_box_open']
    
    data = pb_quest.History.add()
    data.Type = QUEST_HISTORY_TYPE.SUMMON_FLY_TOTAL
    data.AddCount = db_history['summon_fly_total']
    
    return True


#=============================================================================================
# FUNC 퀘스트 발생
    # 용례
    # occur_list = []
    # occur_list.append({ "type":QUEST_TYPE.HUNT_MONSTER_KILL, "condition":0, "count":1 })
    # occur_list.append({ "type":QUEST_TYPE.CHARACTER_LEVEL, "condition":0, "count":1 })
    # occur_list.append({ "type":QUEST_TYPE.HUNT_MONSTER_KILL, "condition":0, "count":1 })
    # with dbConnector() as commander:
    #     QuestOccur(commander, self.useridx, gameTable, occur_list, on_pb.QuestInfo)
#=============================================================================================
def QuestOccur(commander:dbConnector, useridx, gameTable, occur_list, pb_quest):

    dayID = GetDayID()
    weekID = GetWeekID()

    tb_day = gameTable.getDataByName('QuestPeriod517_type', 1)
    tb_week = gameTable.getDataByName('QuestPeriod517_type', 3)
    
    rows = commander.execute('SELECT * FROM TB_USER_QUEST_MAIN WHERE useridx = %s', (useridx, ))
    if len(rows) <= 0:
        return

    db_main = rows[0]
    db_day = []
    db_week = []
    db_history = []

    # 일일 퀘스트 리셋 및 수집
    if db_main['update_day_id'] != dayID:
        db_main['update_day_id'] = dayID
        db_main['reward_day_id'] = 0
        db_main['need_update'] = 1
        commander.execute('UPDATE TB_USER_QUEST_DAY SET complete_count = 0, reward_step = 0 WHERE useridx = %s', (useridx, ))
        for tb in tb_day:
            db_day.append({ "idx":tb['code'], "complete_count":0, "reward_step":0, "need_send":1 })
    else:
        if db_main['reward_day_id'] != dayID:
            find_day_list = []
            for tb in tb_day:
                if QuestContainInList(tb['quest_type'], occur_list) == True:
                    find_day_list.append(tb['code'])
            if len(find_day_list) > 0:
                db_day = ExecuteSelectList(commander, useridx, '*', 'TB_USER_QUEST_DAY', 'idx', find_day_list)
    
    # 주간 퀘스트 리셋 및 수집
    if db_main['update_week_id'] != weekID:
        db_main['update_week_id'] = weekID
        db_main['reward_week_id'] = 0
        db_main['need_update'] = 1
        commander.execute('UPDATE TB_USER_QUEST_WEEK SET complete_count = 0, reward_step = 0 WHERE useridx = %s', (useridx, ))
        for tb in tb_week:
            db_week.append({ "idx":tb['code'], "complete_count":0, "reward_step":0, "need_send":1 })
    else:
        if db_main['reward_week_id'] != weekID:
            find_week_list = []
            for tb in tb_week:
                if QuestContainInList(tb['quest_type'], occur_list) == True:
                    find_week_list.append(tb['code'])
            if len(find_week_list) > 0:
                db_week = ExecuteSelectList(commander, useridx, '*', 'TB_USER_QUEST_WEEK', 'idx', find_week_list)

    # 발생
    for occur in occur_list:
        QuestOccurOne(gameTable, db_main, db_day, db_week, db_history, occur['type'], occur['condition'], occur['count'])

    # 저장 : 메인 퀘스트
    if 'need_update' in db_main and db_main['need_update'] == 1:
        commander.execute('UPDATE TB_USER_QUEST_MAIN SET complete_count=%s, update_day_id=%s, reward_day_id=%s, update_week_id=%s, reward_week_id=%s WHERE useridx = %s'
                          , (db_main['complete_count'], db_main['update_day_id'], db_main['reward_day_id'], db_main['update_week_id'], db_main['reward_week_id'], useridx, ))

    data_main = pb_quest.Main.add()
    data_main.Code = db_main['idx']
    data_main.CompleteCount = db_main['complete_count']
    data_main.RewardStep = db_main['repeat_count']
    pb_quest.UpdateDayID = db_main['update_day_id']
    pb_quest.RewardDayID = db_main['reward_day_id']
    pb_quest.UpdateWeekID = db_main['update_week_id']
    pb_quest.RewardWeekID = db_main['reward_week_id']
    
    # 저장 : 일일 퀘스트
    values_day = []
    for info in db_day:
        if 'need_update' in info and info['need_update'] == 1:
            values_day.append([info['complete_count'], useridx, info['idx']])
        if 'need_send' in info and info['need_send'] == 1:
            data_day = pb_quest.Day.add()
            data_day.Code = info['idx']
            data_day.CompleteCount = info['complete_count']
            data_day.RewardStep = info['reward_step']
    if len(values_day) > 0:
        commander.executemany2("UPDATE TB_USER_QUEST_DAY SET complete_count = %s WHERE useridx = %s AND idx = %s", values_day)

    # 저장 : 주간 퀘스트
    values_week = []
    for info in db_week:
        if 'need_update' in info and info['need_update'] == 1:
            values_week.append([info['complete_count'], useridx, info['idx']])
        if 'need_send' in info and info['need_send'] == 1:
            data_week = pb_quest.Week.add()
            data_week.Code = info['idx']
            data_week.CompleteCount = info['complete_count']
            data_week.RewardStep = info['reward_step']
    if len(values_week) > 0:
        commander.executemany2("UPDATE TB_USER_QUEST_WEEK SET complete_count = %s WHERE useridx = %s AND idx = %s", values_week)

    # 저장 : 히스토리
    if len(db_history) > 0:
        history_str = "UPDATE TB_USER_QUEST_HISTORY SET "
        lastidx = len(db_history) - 1
        for i in range(0, len(db_history)):
            history_str += f"{db_history[i]['type_name']} = {db_history[i]['type_name']} + {db_history[i]['add_count']}"
            if i < lastidx:
                history_str += ", "
            data_history = pb_quest.History.add()
            data_history.Type = db_history[i]["type"]
            data_history.AddCount = db_history[i]["add_count"]
        history_str += f" WHERE useridx = {useridx}"
        
        commander.execute(history_str, ())


def QuestOccurOne(gameTable, db_main, db_day, db_week, db_history, occur_type:QUEST_TYPE, occur_condition, occur_count):

    # 메인 퀘스트에서 확인
    tb_main = gameTable.getDataByName('QuestMain516', db_main["idx"])
    if len(tb_main) <= 0:
        return

    if tb_main["quest_type"] == occur_type and tb_main["is_history"] == 0:
        need_count = tb_main['count']
        if db_main["repeat_count"] > 0:
            need_count = tb_main['count'] + (tb_main['count_add'] * (db_main["repeat_count"] - 1))
            if need_count > tb_main['count_max']:
                need_count = tb_main['count_max']
        if db_main["complete_count"] < need_count:
            if tb_main["condition"] == 0 or tb_main["condition"] == occur_condition:
                sum_count = min(need_count, db_main["complete_count"] + occur_count)
                db_main["complete_count"] = sum_count
                db_main["need_update"] = 1

    # 일일 퀘스트에서 확인
    for info in db_day:
        tb = gameTable.getDataByName('QuestPeriod517', info["idx"])
        if tb['quest_type'] == occur_type and tb["is_history"] == 0 and info["complete_count"] < tb["count"]:
            if tb["condition"] == 0 or tb["condition"] == occur_condition:
                sum_count = min(tb["count"], info["complete_count"] + occur_count)
                info["complete_count"] = sum_count
                info['need_update'] = 1
                info['need_send'] = 1

    # 주간 퀘스트에서 확인
    for info in db_week:
        tb = gameTable.getDataByName('QuestPeriod517', info["idx"])
        if tb['quest_type'] == occur_type and tb["is_history"] == 0 and info["complete_count"] < tb["count"]:
            if tb["condition"] == 0 or tb["condition"] == occur_condition:
                sum_count = min(tb["count"], info["complete_count"] + occur_count)
                info["complete_count"] = sum_count
                info['need_update'] = 1
                info['need_send'] = 1

    # 히스토리에서 확인
    if occur_type == QUEST_TYPE.ARTIFACT_BOX_OPEN:
        db_history.append({"type":QUEST_HISTORY_TYPE.ARTIFACT_BOX_OPEN, "type_name":"artifact_box_open", "add_count":occur_count})
    elif occur_type == QUEST_TYPE.SUMMON_FLY_TOTAL:
        db_history.append({"type":QUEST_HISTORY_TYPE.SUMMON_FLY_TOTAL, "type_name":"summon_fly_total", "add_count":occur_count})


#=============================================================================================
# FUNC 퀘스트 검증
#=============================================================================================
def QuestVerify(commander, useridx, complete_count, quest_type:QUEST_TYPE, is_history, condition, need_count):
    return True

def QuestVerify2(commander, useridx, complete_count, quest_type:QUEST_TYPE, is_history, condition, need_count):
    
    # 클라이언트 발생
    if quest_type in (
        QUEST_TYPE.EQUIPMENT_EQUIP,
        QUEST_TYPE.EQUIPMENT_UPGRADE,
        QUEST_TYPE.SKILL_EQUIP,
        QUEST_TYPE.FLY_EQUIP,
        QUEST_TYPE.CHECK_EVENT,
        QUEST_TYPE.CHECK_SHOP,
        QUEST_TYPE.CHECK_COSTUME,
        QUEST_TYPE.CHECK_MAIL,
        QUEST_TYPE.CHECK_PASS,
        QUEST_TYPE.CHECK_MISSION,
        QUEST_TYPE.CHECK_AD_BUFF,
        QUEST_TYPE.CHECK_NAME_CHANGE,
        QUEST_TYPE.CHECK_PORTRAIT_CHANGE,
        QUEST_TYPE.CHECK_RANKING,
        ):
        return True


    # 퀘스트 받은 후 발생
    if is_history == 0:
        return complete_count >= need_count


    # 히스토리
    
    # 캐릭터 관련
    if quest_type == QUEST_TYPE.CHARACTER_LEVEL:
        return QuestVerifyValue(commander, f"SELECT lv FROM TB_USER_CHARACTER WHERE useridx = {useridx}", "lv") >= need_count
    if quest_type == QUEST_TYPE.STAT_GRADE:
        return QuestVerifyValue(commander, f"SELECT stat_grade FROM TB_USER_CHARACTER WHERE useridx = {useridx}", "stat_grade") >= need_count

    # 소환 관련
    if quest_type == QUEST_TYPE.SUMMON_LEVEL_WEAPON:
        return QuestVerifyValue(commander, f"SELECT level FROM TB_USER_SUMMON WHERE useridx = {useridx} AND type = 0", "level") >= need_count
    if quest_type == QUEST_TYPE.SUMMON_LEVEL_ARMOR:
        return QuestVerifyValue(commander, f"SELECT level FROM TB_USER_SUMMON WHERE useridx = {useridx} AND type = 1", "level") >= need_count
    if quest_type == QUEST_TYPE.SUMMON_LEVEL_ACCESSORY:
        return QuestVerifyValue(commander, f"SELECT level FROM TB_USER_SUMMON WHERE useridx = {useridx} AND type = 2", "level") >= need_count
    if quest_type == QUEST_TYPE.SUMMON_FLY_TOTAL:
        return QuestVerifyValue(commander, f"SELECT summon_fly_total FROM TB_USER_QUEST_HISTORY WHERE useridx = {useridx}", "summon_fly_total") >= need_count
    
    # 장비 관련
    if quest_type == QUEST_TYPE.EQUIPMENT_OWN:
        return QuestVerifyValue(commander, f"SELECT idx FROM TB_USER_EQUIPMENT WHERE useridx = {useridx} AND idx = {condition}", "idx") >= need_count
    if quest_type == QUEST_TYPE.EQUIPMENT_LEVEL_TOTAL:
        return QuestVerifyValue(commander, f"SELECT SUM(level) as 'lv_total' FROM TB_USER_EQUIPMENT WHERE useridx = {useridx}", "lv_total") >= need_count

    # 스킬 관련
    if quest_type == QUEST_TYPE.SKILL_UNLOCK:
        return QuestVerifyValue(commander, f"SELECT idx FROM TB_USER_SKILL WHERE useridx = {useridx} AND idx = {condition}", "idx") >= need_count
    if quest_type == QUEST_TYPE.SKILL_LEVEL_TOTAL:
        return QuestVerifyValue(commander, f"SELECT SUM(lv) as 'lv_total' FROM TB_USER_SKILL WHERE useridx = {useridx}", "lv_total") >= need_count
    
    # 날파리 관련
    if quest_type == QUEST_TYPE.FLY_OWN:
        return QuestVerifyValue(commander, f"SELECT idx FROM TB_USER_FLY_NEW WHERE useridx = {useridx} AND idx = {condition}", "idx") >= need_count
    if quest_type == QUEST_TYPE.FLY_STAR_TOTAL:
        return QuestVerifyValue(commander, f"SELECT SUM(star) as 'star_total' FROM TB_USER_FLY_NEW WHERE useridx = {useridx}", "star_total") >= need_count
    if quest_type == QUEST_TYPE.FLY_LEVEL_TOTAL:
        return QuestVerifyValue(commander, f"SELECT SUM(lv) as 'lv_total' FROM TB_USER_FLY_NEW WHERE useridx = {useridx}", "lv_total") >= need_count
    
    # 향신료 관련
    if quest_type == QUEST_TYPE.SPICE_LEVEL_TOTAL:
        return QuestVerifyValue(commander, f"SELECT SUM(lv) as 'lv_total' FROM TB_USER_SPICE_NEW WHERE useridx = {useridx}", "lv_total") >= need_count
    if quest_type == QUEST_TYPE.SPICE_COLECTION:
        return True
        # tb_list = gameTable.getDataByName('SpiceNewCollection621_grade', condition)
        # if len(tb_list) > 0:
        #     idx_filter = 620000 + condition
        #     lv_total = QuestVerifyValue(f"SELECT SUM(lv) as 'lv_total' FROM TB_USER_SPICE_NEW WHERE useridx = {useridx} AND CAST(idx / 1000 AS UNSIGNED) = {idx_filter}", "lv_total")
        #     for tb in tb_list:
        #         if lv_total >= tb['level_total'] and tb['idx'] >= need_count:
        #             return True
        # return False

    # 연구 관련
    if quest_type == QUEST_TYPE.RESEARCH_POINT_USE:
        rows = commander.execute('SELECT * FROM TB_USER_RESEARCH WHERE useridx = %s', (useridx, ))
        if len(rows) <= 0:
            return False
        use_point = 0
        point_list = gameTable.getTableByName('Research122_need_point')
        complete_step = rows[0]['complete_step']
        if complete_step >= len(point_list):
            use_point = point_list[-1]
        else:
            use_point = rows[0]['lv1'] + rows[0]['lv2'] + rows[0]['lv3'] + point_list[complete_step]
        return use_point >= need_count
    
    # 유물 관련
    if quest_type == QUEST_TYPE.ARTIFACT_BOX_OPEN_TOTAL:
        return QuestVerifyValue(commander, f"SELECT artifact_box_open FROM TB_USER_QUEST_HISTORY WHERE useridx = {useridx}", "artifact_box_open") >= need_count
    if quest_type == QUEST_TYPE.ARTIFACT_LEVEL:
        return QuestVerifyValue(commander, f"SELECT artifact_lv FROM TB_USER_RESOURCE WHERE useridx = {useridx}", "artifact_lv") >= need_count

    # 식당 관련
    if quest_type == QUEST_TYPE.RESTAURANT_OPEN:
        return QuestVerifyValue(commander, f"SELECT idx FROM TB_USER_RESTAURANT WHERE useridx = {useridx} AND idx = {condition}", "idx") >= need_count
    if quest_type == QUEST_TYPE.FOOD_OPEN:
        return True
    if quest_type == QUEST_TYPE.FOOD_LEVEL_TOTAL:
        return True

    # 사냥터
    if quest_type == QUEST_TYPE.HUNT_STAGE:
        return QuestVerifyValue(commander, f"SELECT max_clear_stage FROM TB_USER_HUNT WHERE useridx = {useridx}", "max_clear_stage") >= need_count
    
    # 던전 관련
    if quest_type == QUEST_TYPE.DUNGEON_EQUIP_CLEAR:
        return QuestVerifyValue(commander, f"SELECT clear_stage FROM TB_USER_DUNGEON_NEW WHERE useridx = {useridx} AND type = 1", "clear_stage") >= need_count
    if quest_type == QUEST_TYPE.DUNGEON_FOODBOX_CLEAR:
        return QuestVerifyValue(commander, f"SELECT clear_stage FROM TB_USER_DUNGEON_NEW WHERE useridx = {useridx} AND type = 2", "clear_stage") >= need_count
    if quest_type == QUEST_TYPE.DUNGEON_FLY_CLEAR:
        return QuestVerifyValue(commander, f"SELECT clear_stage FROM TB_USER_DUNGEON_NEW WHERE useridx = {useridx} AND type = 3", "clear_stage") >= need_count
    if quest_type == QUEST_TYPE.DUNGEON_ARTIFACT_CLEAR:
        return QuestVerifyValue(commander, f"SELECT clear_stage FROM TB_USER_DUNGEON_NEW WHERE useridx = {useridx} AND type = 4", "clear_stage") >= need_count
    if quest_type == QUEST_TYPE.DUNGEON_SPICE_CLEAR:
        return QuestVerifyValue(commander, f"SELECT clear_stage FROM TB_USER_DUNGEON_NEW WHERE useridx = {useridx} AND type = 5", "clear_stage") >= need_count
    
    # 배달 관련
    if quest_type == QUEST_TYPE.DELIVERY_PLAY_FOOD_TOTAL:
        return True

    # 럭키 쿠폰 관련
    if quest_type == QUEST_TYPE.LUCKY_COUPON_USE_TOTAL:
        return QuestVerifyValue(commander, f"SELECT use_luckycoupon_total FROM TB_USER_HUNT WHERE useridx = {useridx}", "use_luckycoupon_total") >= need_count
    if quest_type == QUEST_TYPE.LUCKY_GOLD_BOX_OPEN_TOTAL:
        return QuestVerifyValue(commander, f"SELECT open_gold_box_total FROM TB_USER_HUNT WHERE useridx = {useridx}", "open_gold_box_total") >= need_count
    
    # 일일 / 주간 퀘스트
    if quest_type == QUEST_TYPE.DAILY_MISSION_COMPLETE:
        return True
    if quest_type == QUEST_TYPE.WEEKLY_MISSION_COMPLETE:
        return True

    return True


def QuestVerifyValue(commander, str_query, str_column):
    rows = commander.execute(str_query, ())
    if len(rows) <= 0:
        return 0
    return rows[0][str_column]