from src.utils.DB import *
from application import cache, diff_db_time, SERVER_TYPE, gameTable

from src.utils.Common import *
from src.utils.Protocol import *
from src.common.Abstract import *
from datetime import datetime
import src


#=============================================================================================
# SP : 스킬 정보 얻어오기
#=============================================================================================
class sp_SkillGetData(baseTransaction):
    def __init__(self, useridx):
        self.useridx = useridx
        
    def execute(self, on_pb):

        on_pb.Result = Skill_pb2.OnSkillGetData.ResultType.FAIL
        
        with dbConnector() as commander:
            
            rows = commander.execute('SELECT idx, lv FROM TB_USER_SKILL WHERE useridx = %s', (self.useridx, ))
            for row in rows:
                data = on_pb.Skills.add()
                data.Idx = row['idx']
                data.Lv = row['lv']
            
            preset_rows = commander.execute('SELECT * FROM TB_USER_SKILL_PRESET WHERE useridx = %s', (self.useridx, ))
            if len(preset_rows) > 0:
                sk = preset_rows[0]
                on_pb.Equips[:] = [sk['skill_idx1'], sk['skill_idx2'], sk['skill_idx3'], sk['skill_idx4'], sk['skill_idx5'], sk['skill_idx6']]
            else:
                on_pb.Equips[:] = [0, 0, 0, 0, 0, 0]

        on_pb.Result = Skill_pb2.OnSkillGetData.ResultType.SUCCESS


#=============================================================================================
# SP : 스킬 획득
#=============================================================================================
class sp_SkillOpen(baseTransaction):
    def __init__(self, req_pb):
        self.useridx = req_pb.Useridx
        self.skillidx = req_pb.SkillIdx
        
    def execute(self, on_pb):

        on_pb.Result = Skill_pb2.OnSkillOpen.ResultType.FAIL

        tableInfo = gameTable.getDataByName('SkillNew120', self.skillidx)
        if len(tableInfo) <= 0:
            on_pb.Result = Skill_pb2.OnSkillOpen.ResultType.NON_EXIST_SKILL
            return
        
        helper = Helper(self.useridx)
        
        with dbConnector() as commander:
            
            rows = commander.execute('SELECT idx, lv FROM TB_USER_SKILL WHERE useridx = %s AND idx = %s', (self.useridx, self.skillidx, ))
            if len(rows) > 0:
                on_pb.Result = Skill_pb2.OnSkillOpen.ResultType.ALREADY_OPEN
                return

            cost_idx = tableInfo['skillbook_code']
            cost_count = 1
            
            # 스킬북 차감
            helper.ConsumeUserResourceAsCommander(commander, RESOURCE_TYPE.GOODS, cost_idx, cost_count)

            if helper.Result == False: # 스킬북 부족하면
                commander.rollback()
                on_pb.Result =  Skill_pb2.OnSkillOpen.ResultType.NOT_ENOUGH_RESOURCE
                return
            
            commander.execute('INSERT INTO TB_USER_SKILL (useridx, idx) VALUES(%s, %s)', (self.useridx, self.skillidx, ))
            
            on_pb.Skill.Idx = self.skillidx
            on_pb.Skill.Lv = 1
            
            for item in helper.ResultDatas:
                data = on_pb.ResultItems.add()
                data.Idx = item['idx']
                data.AddCount = int(item['add_count'])
                data.ResultCount = int(item['result_count'])
                
            # 퀘스트 : 스킬 획득
            # occur_list = [{ "type":QUEST_TYPE.SKILL_UNLOCK, "condition":self.skillidx, "count":1 }]
            # src.db.Quest.QuestOccur(commander, self.useridx, gameTable, occur_list, on_pb.QuestInfo)

        on_pb.Result = Skill_pb2.OnSkillOpen.ResultType.SUCCESS


#=============================================================================================
# SP : 스킬 장착
#=============================================================================================
class sp_SkillEquip(baseTransaction):
    def __init__(self, req_pb):
        self.useridx = req_pb.Useridx
        self.equips = []
        self.equips[:] = req_pb.Equips
        
    def execute(self, on_pb):

        on_pb.Result = Skill_pb2.OnSkillEquip.ResultType.FAIL
        
        # 잘못 된 데이타
        if len(self.equips) != 6:
            on_pb.Result = Skill_pb2.OnSkillEquip.ResultType.INCORRECT_EQUIP_COUNT
            return
        
        skill_list = []
        
        # 중복 검사
        for equip in self.equips:
            if equip > 0:
                if self.equips.count(equip) >= 2:
                    on_pb.Result = Skill_pb2.OnSkillEquip.ResultType.DUPLICATE_EQUIP
                    return
                skillinfo = gameTable.getDataByName('SkillNew120', equip)
                if len(skillinfo) <= 0 or skillinfo['type'] != 1 or skillinfo['category'] != "ACTIVE":
                    on_pb.Result = Skill_pb2.OnSkillEquip.ResultType.NON_EXIST_SKILL
                    return
                skill_list.append(equip)
        
        with dbConnector() as commander:
            
            # 스킬 소유 여부
            if len(skill_list) > 0:
                skill_rows = ExecuteSelectList(commander, self.useridx, 'idx', 'TB_USER_SKILL', 'idx', skill_list)
                if (len(skill_rows) != len(skill_list)):
                    on_pb.Result = Skill_pb2.OnSkillEquip.ResultType.NON_OWNED_SKILL
                    return
            
            e = self.equips
            commander.execute('UPDATE TB_USER_SKILL_PRESET SET skill_idx1=%s,skill_idx2=%s,skill_idx3=%s,skill_idx4=%s,skill_idx5=%s,skill_idx6=%s WHERE useridx = %s AND idx = 1', (e[0], e[1], e[2], e[3], e[4], e[5], self.useridx, ))
            on_pb.Equips[:] = [e[0], e[1], e[2], e[3], e[4], e[5]]

        on_pb.Result = Skill_pb2.OnSkillEquip.ResultType.SUCCESS


#=============================================================================================
# SP : 스킬 레벨업
#=============================================================================================
class sp_SkillLevelUp(baseTransaction):
    def __init__(self, req_pb):
        self.useridx = req_pb.Useridx
        self.skillidx = req_pb.SkillIdx
        
    def execute(self, on_pb):

        on_pb.Result = Skill_pb2.OnSkillLevelUp.ResultType.FAIL

        # 잘못 된 스킬 번호
        skillinfo = gameTable.getDataByName('SkillNew120', self.skillidx)
        if len(skillinfo) <= 0 or skillinfo['type'] != 1:
            on_pb.Result = Skill_pb2.OnSkillLevelUp.ResultType.NON_EXIST_SKILL
            return

        helper = Helper(self.useridx)
        
        with dbConnector() as commander:
            
            # 보유 여부 확인
            rows = commander.execute('SELECT * FROM TB_USER_SKILL WHERE useridx = %s AND idx = %s', (self.useridx, self.skillidx, ))
            if len(rows) <= 0:
                on_pb.Result = Skill_pb2.OnSkillLevelUp.ResultType.NON_OWNED_SKILL
                return
            
            lv = rows[0]['lv']
            
            
            # 최대 레벨
            if (lv >= skillinfo['max_level']):
                on_pb.Result = Skill_pb2.OnSkillLevelUp.ResultType.MAX_LV
                return
            
            
            # 레벨업 테이블 정보
            upgradeinfo = gameTable.get302SkillUpgradeByRarityGrade(1, lv)
            if len(upgradeinfo) <= 0:
                on_pb.Result = Skill_pb2.OnSkillLevelUp.ResultType.NON_EXIST_SKILL
                return
            
            
            cost_idx = skillinfo['skillbook_code']
            cost_count = upgradeinfo['skillbook_count']
            
            
            # 스킬북 차감
            helper.ConsumeUserResourceAsCommander(commander, RESOURCE_TYPE.GOODS, cost_idx, cost_count)
            
            if helper.Result == False: # 스킬북 부족하면
                commander.rollback()
                on_pb.Result =  Skill_pb2.OnSkillLevelUp.ResultType.NOT_ENOUGH_RESOURCE
                return
            
            
            # 레벨업
            lv = lv + 1
            
            commander.execute('UPDATE TB_USER_SKILL SET lv = %s WHERE useridx = %s AND idx = %s', (lv, self.useridx, self.skillidx, ))
        
            on_pb.Skill.Idx = self.skillidx
            on_pb.Skill.Lv = lv
            
            for item in helper.ResultDatas:
                data = on_pb.ResultItems.add()
                data.Idx = item['idx']
                data.AddCount = int(item['add_count'])
                data.ResultCount = int(item['result_count'])
                
            # 퀘스트 : 스킬 레벨업
            occur_list = [{ "type":QUEST_TYPE.SKILL_LEVEL_UP, "condition":0, "count":1 }]
            src.db.Quest.QuestOccur(commander, self.useridx, gameTable, occur_list, on_pb.QuestInfo)

        on_pb.Result = Skill_pb2.OnSkillLevelUp.ResultType.SUCCESS