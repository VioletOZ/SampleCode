<?php

namespace RpgA\GmSite;

use EToyLab\Database\MsSqlModel;
use EToyLab\Database\SmartLookupInterface;
use EToyLab\LangCode;
use EToyLab\Utils\TimeUtil;
use RpgA\InstanceFactory;
use RpgA\Meanings\RewardType;
use RpgA\Meanings\UserDbNodeIndex;
use RpgA\Models\Account\AccountAuthDB;
use RpgA\Models\Account\AdminLog;
use RpgA\Models\GameLog\UserBadgeLog;
use RpgA\Models\GameLog\UserGuerrillaMissionLog;
use RpgA\Models\GameLog\UserLobbyThemeLog;
use RpgA\Models\GameLog\UserPassMissionLog;
use RpgA\Models\GameLogModel;
use RpgA\Models\Kpi\DailyKpi;
use RpgA\Models\Kpi\DailyRetentionN;
use RpgA\Models\Kpi\HourPayment;
use RpgA\Models\Statistics\RankDau;
use RpgA\Models\Kpi\UserPaymentRanking;
use RpgA\Models\Kpi\UserPaymentRankingByUuid;
use RpgA\Models\Statistics\DailyUserGoods;
use RpgA\Models\Kpi\WeeklyKpi;
use RpgA\Models\Kpi\MonthlyKpi;
use RpgA\Models\Statistics\DailyUserGoods23;
use RpgA\Models\Statistics\DailyUserGoods45;
use RpgA\Models\Statistics\DailyUserGoods6;
use RpgA\Models\Statistics\RankPlaySeconds;
use RpgA\Models\Statistics\UserCashRanking;
use RpgA\Models\Statistics\UserGoldRanking;
use RpgA\Models\Statistics\UserSupporterPointRanking;
use RpgA\Models\UserData\UserData;
use RpgA\Models\UserData\UserInAppPayment;
use RpgA\Models\UserData\UserMail;
use RpgA\Models\UserData\UserPushNoti;
use RpgA\Models\UserDataModel;
use RpgA\Models\UserSingleDataModel;

use RpgA\Models\GameLog\AccountAuthDBLog;
use RpgA\Models\GameLog\AccountLinkDBLog;
use RpgA\Models\GameLog\CharDataLog;
use RpgA\Models\GameLog\CharSkillLog;
use RpgA\Models\GameLog\InfoUserCountLog;
use RpgA\Models\GameLog\SvrCommonLog;
use RpgA\Models\GameLog\TimeAtkStageRankLog;
use RpgA\Models\GameLog\UserAchieveLog;
use RpgA\Models\GameLog\UserArenaLog;
use RpgA\Models\GameLog\UserCardLog;
use RpgA\Models\GameLog\UserCardBookLog;
use RpgA\Models\GameLog\UserCostumeLog;
use RpgA\Models\GameLog\UserDataLog;
use RpgA\Models\GameLog\UserEventRewardLog;
use RpgA\Models\GameLog\UserFacilityLog;
use RpgA\Models\GameLog\UserGemLog;
use RpgA\Models\GameLog\UserGoodsLog;
use RpgA\Models\GameLog\UserItemLog;
use RpgA\Models\GameLog\UserMailLog;
use RpgA\Models\GameLog\UserMarkLog;
use RpgA\Models\GameLog\UserMonsterBookLog;
use RpgA\Models\GameLog\UserRoomFigureSlotLog;
use RpgA\Models\GameLog\UserRoomPurchaseLog;
use RpgA\Models\GameLog\UserRoomThemeSlotLog;
use RpgA\Models\GameLog\UserStageLog;
use RpgA\Models\GameLog\UserStageClearLog;
use RpgA\Models\GameLog\UserStoreSaleLog;
use RpgA\Models\GameLog\UserTimeAtkStageRecLog;
use RpgA\Models\GameLog\UserWeaponLog;
use RpgA\Models\GameLog\UserWeaponBookLog;
use RpgA\Models\GameLog\UserWeekMissionLog;
use RpgA\Models\Kpi\Atspdau;


class SmartLookupTableConfig
{
    const CONFIG_NAME_ADMIN_LOG = 'adminLog';

    const CONFIG_NAME_ACCOUNT_AUTH_DB_LOG = 'AccountAuthDBLog';
    const CONFIG_NAME_ACCOUNT_LINK_DB_LOG = 'AccountLinkDBLog';
    const CONFIG_NAME_CHAR_DATA_LOG = 'CharDataLog';
    const CONFIG_NAME_CHAR_SKILL_LOG = 'CharSkillLog';
    const CONFIG_NAME_INFO_USER_COUNT_LOG = 'InfoUserCountLog';
    const CONFIG_NAME_SVR_COMMON_LOG = 'SvrCommonLog';
    const CONFIG_NAME_TIME_ATK_STAGE_RANK_LOG = 'TimeAtkStageRankLog';
    const CONFIG_NAME_USER_ACHIEVE_LOG = 'UserAchieveLog';
    const CONFIG_NAME_USER_ARENA_LOG = 'UserArenaLog';
    const CONFIG_NAME_USER_BADGE_LOG = 'UserBadgeLog';
    const CONFIG_NAME_USER_CARD_LOG = 'UserCardLog';
    const CONFIG_NAME_USER_CARD_BOOK_LOG = 'UserCardBookLog';
    const CONFIG_NAME_USER_COSTUME_LOG = 'UserCostumeLog';
    const CONFIG_NAME_USER_DATA_LOG = 'UserDataLog';
    const CONFIG_NAME_USER_EVENT_REWARD_LOG = 'UserEventRewardLog';
    const CONFIG_NAME_USER_FACILITY_LOG = 'UserFacilityLog';
    const CONFIG_NAME_USER_GEM_LOG = 'UserGemLog';
    const CONFIG_NAME_USER_GOODS_LOG = 'UserGoodsLog';
    const CONFIG_NAME_USER_GUERRILLA_MISSION_LOG = 'UserGuerrillaMissionLog';
    const CONFIG_NAME_USER_ITEM_LOG = 'UserItemLog';
    const CONFIG_NAME_USER_MAIL_LOG = 'UserMailLog';
    const CONFIG_NAME_USER_MARK_LOG = 'UserMarkLog';
    const CONFIG_NAME_USER_MONSTER_BOOK_LOG = 'UserMonsterBookLog';
    const CONFIG_NAME_USER_PUSH_NOTI_LOG = 'UserPushNotiLog';
    const CONFIG_NAME_USER_ROOM_FIGURE_SLOT_LOG = 'UserRoomFigureSlotLog';
    const CONFIG_NAME_USER_ROOM_PURCHASE_LOG = 'UserRoomPurchaseLog';
    const CONFIG_NAME_USER_ROOM_THEME_SLOT_LOG = 'UserRoomThemeSlotLog';
    const CONFIG_NAME_USER_STAGE_LOG = 'UserStageLog';
    const CONFIG_NAME_USER_STAGE_CLEAR_LOG = 'UserStageClearLog';
    const CONFIG_NAME_USER_STORE_SALE_LOG = 'UserStoreSaleLog';
    const CONFIG_NAME_USER_TIME_ATK_STAGE_REC_LOG = 'UserTimeAtkStageRecLog';
    const CONFIG_NAME_USER_WEAPON_LOG = 'UserWeaponLog';
    const CONFIG_NAME_USER_WEAPON_BOOK_LOG = 'UserWeaponBookLog';
    const CONFIG_NAME_USER_WEEK_MISSION_LOG = 'UserWeekMissionLog';
    const CONFIG_NAME_USER_PASS_MISSION_LOG = 'UserPassMissionLog';
    const CONFIG_NAME_USER_LOBBY_THEME_LOG = 'UserLobbyThemeLog';

    const CONFIG_NAME_CCU = 'Ccu';
    const CONFIG_NAME_RAMK_DAU = 'RankDau';
    const CONFIG_NAME_HOUR_PAYMENT = 'HourPayment';
    const CONFIG_NAME_DAILY_KPI = 'DailyKpi';
    const CONFIG_NAME_WEEKLY_KPI = 'WeeklyKpi';
    const CONFIG_NAME_MONTHLY_KPI = 'MonthlyKpi';
    const CONFIG_NAME_DAILY_RETENTION_7 = 'DailyRetention7';
    const CONFIG_NAME_DAILY_RETENTION_15 = 'DailyRetention15';
    const CONFIG_NAME_DAILY_RETENTION_30 = 'DailyRetention30';
    const CONFIG_NAME_DAILY_RETENTION_BY_UUID_7 = 'DailyRetentionByUuid7';
    const CONFIG_NAME_DAILY_RETENTION_BY_UUID_15 = 'DailyRetentionByUuid15';
    const CONFIG_NAME_DAILY_RETENTION_BY_UUID_30 = 'DailyRetentionByUuid30';
    const CONFIG_NAME_USER_PAYMENT_RANKING = 'UserPaymentRanking';
    const CONFIG_NAME_USER_PAYMENT_RANKING_BY_UUID = 'UserPaymentRankingByUuid';
    const CONFIG_NAME_ATSPDAU = 'Atspdau';

    const CONFIG_NAME_DAILY_USER_GOODS = 'DailyUserGoods';
    const CONFIG_NAME_DAILY_USER_GOODS23 = 'DailyUserGoods23';
    const CONFIG_NAME_DAILY_USER_GOODS45 = 'DailyUserGoods45';
    const CONFIG_NAME_DAILY_USER_GOODS6 = 'DailyUserGoods6';
    const CONFIG_NAME_USER_CASH_RANKING = 'cashRanking';
    const CONFIG_NAME_USER_GOLD_RANKING = 'goldRanking';
    const CONFIG_NAME_USER_SUPPORTER_POINT_RANKING = 'SupporterPointRanking';

    const CONFIG_NAME_TOTAL_USER_DATA_LOOKUP = 'totalUserDataLookup';
    const CONFIG_NAME_USER_DATA_LOOKUP = 'userDataLookup';
    const CONFIG_NAME_USER_IN_APP_PAYMENT_LOOKUP = 'userInAppPaymentLookup';
    const CONFIG_NAME_USER_PUSH_NOTI_LOOKUP = 'userPushNotiLookup';
    const CONFIG_NAME_USER_MAIL_LOOKUP = 'userMailLookup';

    const CONFIG_NAME_RANK_PLAY_SECONDS = 'rankPlaySeconds';

    protected $instanceFactory;

    protected $configArr;

    protected $smartLookupTableConfigNamesChainedWithCategory = [];    // 한 카테고리를 참조하는 smartLookupTableConfigName 들의 목록

    protected static function getFilterWarningMessage()
    {
        $langCode = getDeafultAdminLangCode();
        if ($langCode == LangCode::KO) {
            return '필터링 사용시 DB 에 부하가 있을 수 있으니, 필요할 때만 사용하세요';
        }

        return 'There may be a load on the DB when using filtering, so use it only when necessary.';
    }

    /**
     *
     * @param InstanceFactory $instanceFactory
     */
    public function __construct($instanceFactory)
    {
        $this->instanceFactory = $instanceFactory;

        $this->configArr = [];

        $this->configArr[self::CONFIG_NAME_ADMIN_LOG] = [
            'modelClass' => $this->instanceFactory->adminLogModel(),
            'bUseStringResourceId' => true,
            'jsClassName' => 'RpgAAdminLogLookupTable',
//            'orderFieldInfo' => array(            // orderFieldInfo 속성이 있을 경우 자동으로 맨 앞에 순서 필드가 포함된다
//                'defaultTitle' => '순서',
//            ),
            'fieldDefaultTitles' => array(
                AdminLog::FIELD_LOG_NO => '로그 번호',
                AdminLog::FIELD_COMMAND_SUCCESS => '성공 여부',
                AdminLog::FIELD_COMMAND => '명령',
                AdminLog::FIELD_COMMAND_PARAMS => '명령 인자',
                AdminLog::FIELD_COMMAND_RESULT => '명령 결과',
                AdminLog::FIELD_TARGET_UUID => '대상 UUID',
                AdminLog::FIELD_ADMIN_ID => '관리자 ID',
                AdminLog::FIELD_ADMIN_NAME => '(실행 시점)관리자 이름',
                AdminLog::FIELD_ADMIN_ROLE => '(실행 시점)관리자 역할',
                AdminLog::FIELD_COMMENT => '메모',
                AdminLog::FIELD_REG_TIME => '생성 일시',
            ),
            'valueCategoryInfos' => [
                AdminLog::FIELD_COMMAND_PARAMS => ValueCommentator::CATEGORY_JSON,
                AdminLog::FIELD_COMMAND_RESULT => ValueCommentator::CATEGORY_JSON,
                AdminLog::FIELD_COMMAND_SUCCESS => ValueCommentator::CATEGORY_BOOLEAN_SUCCESS,
                AdminLog::FIELD_ADMIN_ROLE => ValueCommentator::CATEGORY_ADMIN_ROLE,
                AdminLog::FIELD_REG_TIME => ValueCommentator::CATEGORY_TIME_KR,
            ],
            'sortingFiledNames' => [        // 정렬용으로 사용 가능한 필드명 배열
                AdminLog::FIELD_LOG_NO,
                AdminLog::FIELD_COMMAND,
                AdminLog::FIELD_COMMAND_SUCCESS,
                AdminLog::FIELD_TARGET_UUID,
                AdminLog::FIELD_ADMIN_ID,
                AdminLog::FIELD_ADMIN_NAME,
                AdminLog::FIELD_REG_TIME,
            ],
        ];

        // game log table 설정. 시작
        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_ACCOUNT_AUTH_DB_LOG] = [
            'modelClass' => $this->instanceFactory->accountAuthDBLogModel(),
            'bUseStringResourceId' => false,
            'jsClassName' => 'RpgAAccountAuthDBLogLookupTable',
            'bUseDownloadExcel' => true,
            'orderFieldInfo' => [
                'defaultTitle' => '순서',
                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => [
                AccountAuthDBLog::FIELD_FUUID => 'fUUID',
                AccountAuthDBLog::FIELD_FLOGKIND => 'fLogKind',
                AccountAuthDBLog::FIELD_FKINDSTR => 'fKindStr',
                AccountAuthDBLog::FIELD_FTIME => 'fTime',
            ],
            'valueCategoryInfos' => [
                AccountAuthDBLog::FIELD_FLOGKIND => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                AccountAuthDBLog::FIELD_FTIME => ValueCommentator::CATEGORY_DATE_TIME_STR,
            ],
            'sortingFiledNames' => [
                AccountAuthDBLog::FIELD_FUUID,
                AccountAuthDBLog::FIELD_FLOGKIND,
                AccountAuthDBLog::FIELD_FTIME,
            ],
            'filterComment' => self::getFilterWarningMessage(),
            'filterCategoryInfos' => [
                AccountAuthDBLog::FIELD_FUUID => [
                    'label' => 'UUID',
                    //'category' => ValueCommentator::CATEGORY_,
                ],
                AccountAuthDBLog::FIELD_FLOGKIND => [
                    'label' => 'LogKind',
                    'category' => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                ],
                GameLogModel::createFilterKeyNameWithOperator(AccountAuthDBLog::FIELD_FKINDSTR, GameLogModel::OPERATOR_TEXT_LIKE) => [
                    'label' => 'KindStr',
                ],
                GameLogModel::createFilterKeyNameWithOperator(AccountAuthDBLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_GE) => [
                    'label' => 'StartDate(>=)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
                GameLogModel::createFilterKeyNameWithOperator(AccountAuthDBLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_LT) => [
                    'label' => 'EndDate(<)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_ACCOUNT_LINK_DB_LOG] = [
            'modelClass' => $this->instanceFactory->accountLinkDBLogModel(),
            'bUseStringResourceId' => false,
            'jsClassName' => 'RpgAAccountLinkDBLogLookupTable',
            'bUseDownloadExcel' => true,
            'orderFieldInfo' => [
                'defaultTitle' => '순서',
                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => [
                AccountLinkDBLog::FIELD_FLINKID => 'fLinkID',
                AccountLinkDBLog::FIELD_FLOGKIND => 'fLogKind',
                AccountLinkDBLog::FIELD_FACCOUNTTYPE => 'fAccountType',
                AccountLinkDBLog::FIELD_FUUID => 'fUUID',
                AccountLinkDBLog::FIELD_FTIME => 'fTime',
            ],
            'valueCategoryInfos' => [
                AccountLinkDBLog::FIELD_FLOGKIND => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                AccountLinkDBLog::FIELD_FTIME => ValueCommentator::CATEGORY_DATE_TIME_STR,
            ],
            'sortingFiledNames' => [
                AccountLinkDBLog::FIELD_FUUID,
                AccountLinkDBLog::FIELD_FLOGKIND,
                AccountLinkDBLog::FIELD_FTIME,
            ],
            'filterComment' => self::getFilterWarningMessage(),
            'filterCategoryInfos' => [
                AccountLinkDBLog::FIELD_FUUID => [
                    'label' => 'UUID',
                    //'category' => ValueCommentator::CATEGORY_,
                ],
                AccountLinkDBLog::FIELD_FLOGKIND => [
                    'label' => 'LogKind',
                    'category' => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                ],
                GameLogModel::createFilterKeyNameWithOperator(AccountLinkDBLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_GE) => [
                    'label' => 'StartDate(>=)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
                GameLogModel::createFilterKeyNameWithOperator(AccountLinkDBLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_LT) => [
                    'label' => 'EndDate(<)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_CHAR_DATA_LOG] = [
            'modelClass' => $this->instanceFactory->charDataLogModel(),
            'bUseStringResourceId' => false,
            'jsClassName' => 'RpgACharDataLogLookupTable',
            'bUseDownloadExcel' => true,
            'orderFieldInfo' => [
                'defaultTitle' => '순서',
                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => [
                CharDataLog::FIELD_FCUID => 'fCUID',
                CharDataLog::FIELD_FUUID => 'fUUID',
                CharDataLog::FIELD_FLOGKIND => 'fLogKind',
                CharDataLog::FIELD_FKINDSTR => 'fKindStr',
                CharDataLog::FIELD_FTIME => 'fTime',
            ],
            'valueCategoryInfos' => [
                CharDataLog::FIELD_FLOGKIND => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                CharDataLog::FIELD_FTIME => ValueCommentator::CATEGORY_DATE_TIME_STR,
            ],
            'sortingFiledNames' => [
                CharDataLog::FIELD_FUUID,
                CharDataLog::FIELD_FLOGKIND,
                CharDataLog::FIELD_FTIME,
            ],
            'filterComment' => self::getFilterWarningMessage(),
            'filterCategoryInfos' => [
                CharDataLog::FIELD_FUUID => [
                    'label' => 'UUID',
                    //'category' => ValueCommentator::CATEGORY_,
                ],
                CharDataLog::FIELD_FLOGKIND => [
                    'label' => 'LogKind',
                    'category' => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                ],
                GameLogModel::createFilterKeyNameWithOperator(CharDataLog::FIELD_FKINDSTR, GameLogModel::OPERATOR_TEXT_LIKE) => [
                    'label' => 'KindStr',
                ],
                GameLogModel::createFilterKeyNameWithOperator(CharDataLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_GE) => [
                    'label' => 'StartDate(>=)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
                GameLogModel::createFilterKeyNameWithOperator(CharDataLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_LT) => [
                    'label' => 'EndDate(<)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_CHAR_SKILL_LOG] = [
            'modelClass' => $this->instanceFactory->charSkillLogModel(),
            'bUseStringResourceId' => false,
            'jsClassName' => 'RpgACharSkillLogLookupTable',
            'bUseDownloadExcel' => true,
            'orderFieldInfo' => [
                'defaultTitle' => '순서',
                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => [
                CharSkillLog::FIELD_FUUID => 'fUUID',
                CharSkillLog::FIELD_FCUID => 'fCUID',
                CharSkillLog::FIELD_FSKILLTABLEID => 'fSkillTableID',
                CharSkillLog::FIELD_FLOGKIND => 'fLogKind',
                CharSkillLog::FIELD_FKINDSTR => 'fKindStr',
                CharSkillLog::FIELD_FTIME => 'fTime',
            ],
            'valueCategoryInfos' => [
                CharSkillLog::FIELD_FLOGKIND => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                CharSkillLog::FIELD_FTIME => ValueCommentator::CATEGORY_DATE_TIME_STR,
                CharSkillLog::FIELD_FSKILLTABLEID => ValueCommentator::CATEGORY_CHARACTER_SKILL_PASSIVE_ID,
            ],
            'sortingFiledNames' => [
                CharSkillLog::FIELD_FUUID,
                CharSkillLog::FIELD_FLOGKIND,
                CharSkillLog::FIELD_FTIME,
            ],
            'filterComment' => self::getFilterWarningMessage(),
            'filterCategoryInfos' => [
                CharSkillLog::FIELD_FUUID => [
                    'label' => 'UUID',
                    //'category' => ValueCommentator::CATEGORY_,
                ],
                CharSkillLog::FIELD_FLOGKIND => [
                    'label' => 'LogKind',
                    'category' => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                ],
                GameLogModel::createFilterKeyNameWithOperator(CharSkillLog::FIELD_FKINDSTR, GameLogModel::OPERATOR_TEXT_LIKE) => [
                    'label' => 'KindStr',
                ],
                GameLogModel::createFilterKeyNameWithOperator(CharSkillLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_GE) => [
                    'label' => 'StartDate(>=)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
                GameLogModel::createFilterKeyNameWithOperator(CharSkillLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_LT) => [
                    'label' => 'EndDate(<)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_INFO_USER_COUNT_LOG] = [
            'modelClass' => $this->instanceFactory->infoUserCountLogModel(),
            'bUseStringResourceId' => false,
            'jsClassName' => 'RpgAInfoUserCountLogLookupTable',
            'bUseDownloadExcel' => true,
            'orderFieldInfo' => [
                'defaultTitle' => '순서',
                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => [
                InfoUserCountLog::FIELD_FTOTAL => 'fTotal',
                InfoUserCountLog::FIELD_FGUEST => 'fGuest',
                InfoUserCountLog::FIELD_FGOOGLEPLAY => 'fGooglePlay',
                InfoUserCountLog::FIELD_FGAMECENTER => 'fGameCenter',
                InfoUserCountLog::FIELD_FFACEBOOK => 'fFacebook',
                InfoUserCountLog::FIELD_FANDROID => 'fAndroid',
                InfoUserCountLog::FIELD_FIOS => 'fIOS',
                InfoUserCountLog::FIELD_FTIME => 'fTime',
            ],
            'valueCategoryInfos' => [
                InfoUserCountLog::FIELD_FTIME => ValueCommentator::CATEGORY_DATE_TIME_STR,
            ],
            'sortingFiledNames' => [
                InfoUserCountLog::FIELD_FTIME,
            ],
            'filterComment' => self::getFilterWarningMessage(),
            'filterCategoryInfos' => [
                GameLogModel::createFilterKeyNameWithOperator(InfoUserCountLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_GE) => [
                    'label' => 'StartDate(>=)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
                GameLogModel::createFilterKeyNameWithOperator(InfoUserCountLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_LT) => [
                    'label' => 'EndDate(<)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_SVR_COMMON_LOG] = [
            'modelClass' => $this->instanceFactory->svrCommonLogModel(),
            'bUseStringResourceId' => false,
            'jsClassName' => 'RpgASvrCommonLogLookupTable',
            'bUseDownloadExcel' => true,
            'orderFieldInfo' => [
                'defaultTitle' => '순서',
                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => [
                SvrCommonLog::FIELD_FLOGKIND => 'fLogKind',
                SvrCommonLog::FIELD_FTIME => 'fTime',
                SvrCommonLog::FIELD_FKINDSTR => 'fKindStr',
            ],
            'valueCategoryInfos' => [
                SvrCommonLog::FIELD_FLOGKIND => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                SvrCommonLog::FIELD_FTIME => ValueCommentator::CATEGORY_DATE_TIME_STR,
            ],
            'sortingFiledNames' => [
                SvrCommonLog::FIELD_FLOGKIND,
                SvrCommonLog::FIELD_FTIME,
            ],
            'filterComment' => self::getFilterWarningMessage(),
            'filterCategoryInfos' => [
                SvrCommonLog::FIELD_FLOGKIND => [
                    'label' => 'LogKind',
                    'category' => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                ],
                GameLogModel::createFilterKeyNameWithOperator(SvrCommonLog::FIELD_FKINDSTR, GameLogModel::OPERATOR_TEXT_LIKE) => [
                    'label' => 'KindStr',
                ],
                GameLogModel::createFilterKeyNameWithOperator(SvrCommonLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_GE) => [
                    'label' => 'StartDate(>=)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
                GameLogModel::createFilterKeyNameWithOperator(SvrCommonLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_LT) => [
                    'label' => 'EndDate(<)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_TIME_ATK_STAGE_RANK_LOG] = [
            'modelClass' => $this->instanceFactory->timeAtkStageRankLogModel(),
            'bUseStringResourceId' => false,
            'jsClassName' => 'RpgATimeAtkStageRankLogLookupTable',
            'bUseDownloadExcel' => true,
            'orderFieldInfo' => [
                'defaultTitle' => '순서',
                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => [
                TimeAtkStageRankLog::FIELD_FUUID => 'fUUID',
                TimeAtkStageRankLog::FIELD_FSTAGEID => 'fStageID',
                TimeAtkStageRankLog::FIELD_FRECORD => 'fRecord',
                TimeAtkStageRankLog::FIELD_FRANK => 'fRank',
                TimeAtkStageRankLog::FIELD_FLOGKIND => 'fLogKind',
                TimeAtkStageRankLog::FIELD_FKINDSTR => 'fKindStr',
                TimeAtkStageRankLog::FIELD_FTIME => 'fTime',
            ],
            'valueCategoryInfos' => [
                TimeAtkStageRankLog::FIELD_FLOGKIND => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                TimeAtkStageRankLog::FIELD_FTIME => ValueCommentator::CATEGORY_DATE_TIME_STR,
                TimeAtkStageRankLog::FIELD_FSTAGEID => ValueCommentator::CATEGORY_STAGE_ID,
            ],
            'sortingFiledNames' => [
                TimeAtkStageRankLog::FIELD_FUUID,
                TimeAtkStageRankLog::FIELD_FLOGKIND,
                TimeAtkStageRankLog::FIELD_FTIME,
            ],
            'filterComment' => self::getFilterWarningMessage(),
            'filterCategoryInfos' => [
                TimeAtkStageRankLog::FIELD_FUUID => [
                    'label' => 'UUID',
                    //'category' => ValueCommentator::CATEGORY_,
                ],
                TimeAtkStageRankLog::FIELD_FLOGKIND => [
                    'label' => 'LogKind',
                    'category' => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                ],
                GameLogModel::createFilterKeyNameWithOperator(TimeAtkStageRankLog::FIELD_FKINDSTR, GameLogModel::OPERATOR_TEXT_LIKE) => [
                    'label' => 'KindStr',
                ],
                GameLogModel::createFilterKeyNameWithOperator(TimeAtkStageRankLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_GE) => [
                    'label' => 'StartDate(>=)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
                GameLogModel::createFilterKeyNameWithOperator(TimeAtkStageRankLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_LT) => [
                    'label' => 'EndDate(<)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_USER_ACHIEVE_LOG] = [
            'modelClass' => $this->instanceFactory->userAchieveLogModel(),
            'bUseStringResourceId' => false,
            'jsClassName' => 'RpgAUserAchieveLogLookupTable',
            'bUseDownloadExcel' => true,
            'orderFieldInfo' => [
                'defaultTitle' => '순서',
                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => [
                UserAchieveLog::FIELD_FUUID => 'fUUID',
                UserAchieveLog::FIELD_FTIME => 'fTime',
                UserAchieveLog::FIELD_FLOGKIND => 'fLogKind',
                UserAchieveLog::FIELD_FGROUPID => 'fGroupID',
                UserAchieveLog::FIELD_FCONDIVAL => 'fCondiVal',
                UserAchieveLog::FIELD_FNOWSTEP => 'fNowStep',
            ],
            'valueCategoryInfos' => [
                UserAchieveLog::FIELD_FLOGKIND => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                UserAchieveLog::FIELD_FTIME => ValueCommentator::CATEGORY_DATE_TIME_STR,
                UserAchieveLog::FIELD_FGROUPID => ValueCommentator::CATEGORY_ACHIEVE_GROUP_ID,
            ],
            'sortingFiledNames' => [
                UserAchieveLog::FIELD_FUUID,
                UserAchieveLog::FIELD_FLOGKIND,
                UserAchieveLog::FIELD_FTIME,
            ],
            'filterComment' => self::getFilterWarningMessage(),
            'filterCategoryInfos' => [
                UserAchieveLog::FIELD_FUUID => [
                    'label' => 'UUID',
                    //'category' => ValueCommentator::CATEGORY_,
                ],
                UserAchieveLog::FIELD_FLOGKIND => [
                    'label' => 'LogKind',
                    'category' => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserAchieveLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_GE) => [
                    'label' => 'StartDate(>=)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserAchieveLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_LT) => [
                    'label' => 'EndDate(<)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_USER_ARENA_LOG] = [
            'modelClass' => $this->instanceFactory->userArenaLogModel(),
            'bUseStringResourceId' => false,
            'jsClassName' => 'RpgAUserArenaLogLookupTable',
            'bUseDownloadExcel' => true,
            'orderFieldInfo' => [
                'defaultTitle' => '순서',
                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => [
                UserArenaLog::FIELD_FUUID => 'fUUID',
                UserArenaLog::FIELD_FLOGKIND => 'fLogKind',
                UserArenaLog::FIELD_FKINDSTR => 'fKindStr',
                UserArenaLog::FIELD_FTIME => 'fTime',

            ],
            'valueCategoryInfos' => [
                UserArenaLog::FIELD_FLOGKIND => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                UserArenaLog::FIELD_FTIME => ValueCommentator::CATEGORY_DATE_TIME_STR,
            ],
            'sortingFiledNames' => [
                UserArenaLog::FIELD_FUUID,
                UserArenaLog::FIELD_FLOGKIND,
                UserArenaLog::FIELD_FTIME,
            ],
            'filterComment' => self::getFilterWarningMessage(),
            'filterCategoryInfos' => [
                UserArenaLog::FIELD_FUUID => [
                    'label' => 'UUID',
                    //'category' => ValueCommentator::CATEGORY_,
                ],
                UserArenaLog::FIELD_FLOGKIND => [
                    'label' => 'LogKind',
                    'category' => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserArenaLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_GE) => [
                    'label' => 'StartDate(>=)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserArenaLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_LT) => [
                    'label' => 'EndDate(<)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_USER_BADGE_LOG] = [
            'modelClass' => $this->instanceFactory->userBadgeLogModel(),
            'bUseStringResourceId' => false,
            'jsClassName' => 'RpgAUserBadgeLogLookupTable',
            'bUseDownloadExcel' => true,
            'orderFieldInfo' => [
                'defaultTitle' => '순서',
                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => [
                UserBadgeLog::FIELD_FUUID => 'fUUID',
                UserBadgeLog::FIELD_FBADGEUID => 'fBadgeUID',
                UserBadgeLog::FIELD_FLOGKIND => 'fLogKind',
                UserBadgeLog::FIELD_FKINDSTR => 'fKindStr',
                UserBadgeLog::FIELD_FTIME => 'fTime',
            ],
            'valueCategoryInfos' => [
                UserBadgeLog::FIELD_FLOGKIND => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                UserBadgeLog::FIELD_FTIME => ValueCommentator::CATEGORY_DATE_TIME_STR,
            ],
            'sortingFiledNames' => [
                UserBadgeLog::FIELD_FUUID,
                UserBadgeLog::FIELD_FLOGKIND,
                UserBadgeLog::FIELD_FTIME,
            ],
            'filterComment' => self::getFilterWarningMessage(),
            'filterCategoryInfos' => [
                UserBadgeLog::FIELD_FUUID => [
                    'label' => 'UUID',
                    //'category' => ValueCommentator::CATEGORY_,
                ],
                UserBadgeLog::FIELD_FLOGKIND => [
                    'label' => 'LogKind',
                    'category' => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserBadgeLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_GE) => [
                    'label' => 'StartDate(>=)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserBadgeLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_LT) => [
                    'label' => 'EndDate(<)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_USER_CARD_LOG] = [
            'modelClass' => $this->instanceFactory->userCardLogModel(),
            'bUseStringResourceId' => false,
            'jsClassName' => 'RpgAUserCardLogLookupTable',
            'bUseDownloadExcel' => true,
            'orderFieldInfo' => [
                'defaultTitle' => '순서',
                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => [
                UserCardLog::FIELD_FUUID => 'fUUID',
                UserCardLog::FIELD_FCARDUID => 'fCardUID',
                UserCardLog::FIELD_FLOGKIND => 'fLogKind',
                UserCardLog::FIELD_FKINDSTR => 'fKindStr',
                UserCardLog::FIELD_FTIME => 'fTime',
            ],
            'valueCategoryInfos' => [
                UserCardLog::FIELD_FLOGKIND => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                UserCardLog::FIELD_FTIME => ValueCommentator::CATEGORY_DATE_TIME_STR,
            ],
            'sortingFiledNames' => [
                UserCardLog::FIELD_FUUID,
                UserCardLog::FIELD_FLOGKIND,
                UserCardLog::FIELD_FTIME,
            ],
            'filterComment' => self::getFilterWarningMessage(),
            'filterCategoryInfos' => [
                UserCardLog::FIELD_FUUID => [
                    'label' => 'UUID',
                    //'category' => ValueCommentator::CATEGORY_,
                ],
                UserCardLog::FIELD_FLOGKIND => [
                    'label' => 'LogKind',
                    'category' => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserCardLog::FIELD_FKINDSTR, GameLogModel::OPERATOR_TEXT_LIKE) => [
                    'label' => 'KindStr',
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserCardLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_GE) => [
                    'label' => 'StartDate(>=)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserCardLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_LT) => [
                    'label' => 'EndDate(<)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_USER_CARD_BOOK_LOG] = [
            'modelClass' => $this->instanceFactory->userCardBookLogModel(),
            'bUseStringResourceId' => false,
            'jsClassName' => 'RpgAUserCardBookLogLookupTable',
            'bUseDownloadExcel' => true,
            'orderFieldInfo' => [
                'defaultTitle' => '순서',
                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => [
                UserCardBookLog::FIELD_FUUID => 'fUUID',
                UserCardBookLog::FIELD_FTABLEID => 'fTableID',
                UserCardBookLog::FIELD_FLOGKIND => 'fLogKind',
                UserCardBookLog::FIELD_FKINDSTR => 'fKindStr',
                UserCardBookLog::FIELD_FTIME => 'fTime',
            ],
            'valueCategoryInfos' => [
                UserCardBookLog::FIELD_FLOGKIND => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                UserCardBookLog::FIELD_FTIME => ValueCommentator::CATEGORY_DATE_TIME_STR,
                UserCardBookLog::FIELD_FTABLEID => ValueCommentator::CATEGORY_CARD_ID,
            ],
            'sortingFiledNames' => [
                UserCardBookLog::FIELD_FUUID,
                UserCardBookLog::FIELD_FLOGKIND,
                UserCardBookLog::FIELD_FTIME,
            ],
            'filterComment' => self::getFilterWarningMessage(),
            'filterCategoryInfos' => [
                UserCardBookLog::FIELD_FUUID => [
                    'label' => 'UUID',
                    //'category' => ValueCommentator::CATEGORY_,
                ],
                UserCardBookLog::FIELD_FLOGKIND => [
                    'label' => 'LogKind',
                    'category' => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserCardBookLog::FIELD_FKINDSTR, GameLogModel::OPERATOR_TEXT_LIKE) => [
                    'label' => 'KindStr',
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserCardBookLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_GE) => [
                    'label' => 'StartDate(>=)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserCardBookLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_LT) => [
                    'label' => 'EndDate(<)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_USER_COSTUME_LOG] = [
            'modelClass' => $this->instanceFactory->userCostumeLogModel(),
            'bUseStringResourceId' => false,
            'jsClassName' => 'RpgAUserCostumeLogLookupTable',
            'bUseDownloadExcel' => true,
            'orderFieldInfo' => [
                'defaultTitle' => '순서',
                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => [
                UserCostumeLog::FIELD_FUUID => 'fUUID',
                UserCostumeLog::FIELD_FTABLEID => 'fTableID',
                UserCostumeLog::FIELD_FLOGKIND => 'fLogKind',
                UserCostumeLog::FIELD_FTIME => 'fTime',
            ],
            'valueCategoryInfos' => [
                UserCostumeLog::FIELD_FLOGKIND => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                UserCostumeLog::FIELD_FTIME => ValueCommentator::CATEGORY_DATE_TIME_STR,
                UserCostumeLog::FIELD_FTABLEID => ValueCommentator::CATEGORY_COSTUME_ID,
            ],
            'sortingFiledNames' => [
                UserCostumeLog::FIELD_FUUID,
                UserCostumeLog::FIELD_FLOGKIND,
                UserCostumeLog::FIELD_FTIME,
            ],
            'filterComment' => self::getFilterWarningMessage(),
            'filterCategoryInfos' => [
                UserCostumeLog::FIELD_FUUID => [
                    'label' => 'UUID',
                    //'category' => ValueCommentator::CATEGORY_,
                ],
                UserCostumeLog::FIELD_FLOGKIND => [
                    'label' => 'LogKind',
                    'category' => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserCostumeLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_GE) => [
                    'label' => 'StartDate(>=)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserCostumeLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_LT) => [
                    'label' => 'EndDate(<)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_USER_DATA_LOG] = [
            'modelClass' => $this->instanceFactory->userDataLogModel(),
            'bUseStringResourceId' => false,
            'jsClassName' => 'RpgAUserDataLogLookupTable',
            'bUseDownloadExcel' => true,
            'orderFieldInfo' => [
                'defaultTitle' => '순서',
                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => [
                UserDataLog::FIELD_FUUID => 'fUUID',
                UserDataLog::FIELD_FLOGKIND => 'fLogKind',
                UserDataLog::FIELD_FKINDSTR => 'fKindStr',
                UserDataLog::FIELD_FTIME => 'fTime',
            ],
            'valueCategoryInfos' => [
                UserDataLog::FIELD_FLOGKIND => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                UserDataLog::FIELD_FTIME => ValueCommentator::CATEGORY_DATE_TIME_STR,
            ],
            'sortingFiledNames' => [
                UserDataLog::FIELD_FUUID,
                UserDataLog::FIELD_FLOGKIND,
                UserDataLog::FIELD_FTIME,
            ],
            'filterComment' => self::getFilterWarningMessage(),
            'filterCategoryInfos' => [
                UserDataLog::FIELD_FUUID => [
                    'label' => 'UUID',
                    //'category' => ValueCommentator::CATEGORY_,
                ],
                UserDataLog::FIELD_FLOGKIND => [
                    'label' => 'LogKind',
                    'category' => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserDataLog::FIELD_FKINDSTR, GameLogModel::OPERATOR_TEXT_LIKE) => [
                    'label' => 'KindStr',
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserDataLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_GE) => [
                    'label' => 'StartDate(>=)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserDataLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_LT) => [
                    'label' => 'EndDate(<)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_USER_EVENT_REWARD_LOG] = [
            'modelClass' => $this->instanceFactory->userEventRewardLogModel(),
            'bUseStringResourceId' => false,
            'jsClassName' => 'RpgAUserEventRewardLogLookupTable',
            'bUseDownloadExcel' => true,
            'orderFieldInfo' => [
                'defaultTitle' => '순서',
                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => [
                UserEventRewardLog::FIELD_FUUID => 'fUUID',
                UserEventRewardLog::FIELD_FEVENTID => 'fEventID',
                UserEventRewardLog::FIELD_FSTEP => 'fStep',
                UserEventRewardLog::FIELD_FTIME => 'fTime',
                UserEventRewardLog::FIELD_FLOGKIND => 'fLogKind',
                UserEventRewardLog::FIELD_FKINDSTR => 'fKindStr',
            ],
            'valueCategoryInfos' => [
                UserEventRewardLog::FIELD_FLOGKIND => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                UserEventRewardLog::FIELD_FTIME => ValueCommentator::CATEGORY_DATE_TIME_STR,
                UserEventRewardLog::FIELD_FEVENTID => ValueCommentator::CATEGORY_EVENT_SET_ID,
            ],
            'sortingFiledNames' => [
                UserEventRewardLog::FIELD_FUUID,
                UserEventRewardLog::FIELD_FLOGKIND,
                UserEventRewardLog::FIELD_FTIME,
            ],
            'filterComment' => self::getFilterWarningMessage(),
            'filterCategoryInfos' => [
                UserEventRewardLog::FIELD_FUUID => [
                    'label' => 'UUID',
                    //'category' => ValueCommentator::CATEGORY_,
                ],
                UserEventRewardLog::FIELD_FLOGKIND => [
                    'label' => 'LogKind',
                    'category' => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserEventRewardLog::FIELD_FKINDSTR, GameLogModel::OPERATOR_TEXT_LIKE) => [
                    'label' => 'KindStr',
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserEventRewardLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_GE) => [
                    'label' => 'StartDate(>=)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserEventRewardLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_LT) => [
                    'label' => 'EndDate(<)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_USER_FACILITY_LOG] = [
            'modelClass' => $this->instanceFactory->userFacilityLogModel(),
            'bUseStringResourceId' => false,
            'jsClassName' => 'RpgAUserFacilityLogLookupTable',
            'bUseDownloadExcel' => true,
            'orderFieldInfo' => [
                'defaultTitle' => '순서',
                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => [
                UserFacilityLog::FIELD_FUUID => 'fUUID',
                UserFacilityLog::FIELD_FTABLEID => 'fTableID',
                UserFacilityLog::FIELD_FLOGKIND => 'fLogKind',
                UserFacilityLog::FIELD_FKINDSTR => 'fKindStr',
                UserFacilityLog::FIELD_FTIME => 'fTime',
            ],
            'valueCategoryInfos' => [
                UserFacilityLog::FIELD_FLOGKIND => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                UserFacilityLog::FIELD_FTIME => ValueCommentator::CATEGORY_DATE_TIME_STR,
                UserFacilityLog::FIELD_FTABLEID => ValueCommentator::CATEGORY_FACILITY_ID,
            ],
            'sortingFiledNames' => [
                UserFacilityLog::FIELD_FUUID,
                UserFacilityLog::FIELD_FLOGKIND,
                UserFacilityLog::FIELD_FTIME,
            ],
            'filterComment' => self::getFilterWarningMessage(),
            'filterCategoryInfos' => [
                UserFacilityLog::FIELD_FUUID => [
                    'label' => 'UUID',
                    //'category' => ValueCommentator::CATEGORY_,
                ],
                UserFacilityLog::FIELD_FLOGKIND => [
                    'label' => 'LogKind',
                    'category' => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserFacilityLog::FIELD_FKINDSTR, GameLogModel::OPERATOR_TEXT_LIKE) => [
                    'label' => 'KindStr',
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserFacilityLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_GE) => [
                    'label' => 'StartDate(>=)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserFacilityLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_LT) => [
                    'label' => 'EndDate(<)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_USER_GEM_LOG] = [
            'modelClass' => $this->instanceFactory->userGemLogModel(),
            'bUseStringResourceId' => false,
            'jsClassName' => 'RpgAUserGemLogLookupTable',
            'bUseDownloadExcel' => true,
            'orderFieldInfo' => [
                'defaultTitle' => '순서',
                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => [
                UserGemLog::FIELD_FUUID => 'fUUID',
                UserGemLog::FIELD_FGEMUID => 'fGemUID',
                UserGemLog::FIELD_FLOGKIND => 'fLogKind',
                UserGemLog::FIELD_FKINDSTR => 'fKindStr',
                UserGemLog::FIELD_FTIME => 'fTime',
            ],
            'valueCategoryInfos' => [
                UserGemLog::FIELD_FLOGKIND => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                UserGemLog::FIELD_FTIME => ValueCommentator::CATEGORY_DATE_TIME_STR,
            ],
            'sortingFiledNames' => [
                UserGemLog::FIELD_FUUID,
                UserGemLog::FIELD_FLOGKIND,
                UserGemLog::FIELD_FTIME,
            ],
            'filterComment' => self::getFilterWarningMessage(),
            'filterCategoryInfos' => [
                UserGemLog::FIELD_FUUID => [
                    'label' => 'UUID',
                    //'category' => ValueCommentator::CATEGORY_,
                ],
                UserGemLog::FIELD_FLOGKIND => [
                    'label' => 'LogKind',
                    'category' => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserGemLog::FIELD_FKINDSTR, GameLogModel::OPERATOR_TEXT_LIKE) => [
                    'label' => 'KindStr',
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserGemLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_GE) => [
                    'label' => 'StartDate(>=)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserGemLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_LT) => [
                    'label' => 'EndDate(<)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_USER_GOODS_LOG] = [
            'modelClass' => $this->instanceFactory->userGoodsLogModel(),
            'bUseStringResourceId' => false,
            'jsClassName' => 'RpgAUserGoodsLogLookupTable',
            'bUseDownloadExcel' => true,
            'orderFieldInfo' => [
                'defaultTitle' => '순서',
                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => [
                UserGoodsLog::FIELD_FUUID => 'fUUID',
                UserGoodsLog::FIELD_FLOGKIND => 'fLogKind',
                UserGoodsLog::FIELD_FGOODSKIND => 'fGoodsKind',
                UserGoodsLog::FIELD_FGOODSVALUE => 'fGoodsValue',
                UserGoodsLog::FIELD_FKINDVALUE => 'fKindValue',
                UserGoodsLog::FIELD_FTIME => 'fTime',
            ],
            'valueCategoryInfos' => [
                UserGoodsLog::FIELD_FLOGKIND => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                UserGoodsLog::FIELD_FTIME => ValueCommentator::CATEGORY_DATE_TIME_STR,
                UserGoodsLog::FIELD_FGOODSKIND => ValueCommentator::CATEGORY_GOODS_TYPE,
            ],
            'sortingFiledNames' => [
                UserGoodsLog::FIELD_FUUID,
                UserGoodsLog::FIELD_FLOGKIND,
                UserGoodsLog::FIELD_FTIME,
            ],
            'filterComment' => self::getFilterWarningMessage(),
            'filterCategoryInfos' => [
                UserGoodsLog::FIELD_FUUID => [
                    'label' => 'UUID',
                    //'category' => ValueCommentator::CATEGORY_,
                ],
                UserGoodsLog::FIELD_FLOGKIND => [
                    'label' => 'LogKind',
                    'category' => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserGoodsLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_GE) => [
                    'label' => 'StartDate(>=)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserGoodsLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_LT) => [
                    'label' => 'EndDate(<)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_USER_GUERRILLA_MISSION_LOG] = [
            'modelClass' => $this->instanceFactory->userGuerrillaMissionLogModel(),
            'bUseStringResourceId' => false,
            'jsClassName' => 'RpgAUserGuerrillaMissionLogLookupTable',
            'orderFieldInfo' => [
                'defaultTitle' => '순서',
                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => [
                UserGuerrillaMissionLog::FIELD_FUUID => 'fUUID',
                UserGuerrillaMissionLog::FIELD_FTIME => 'fTime',
                UserGuerrillaMissionLog::FIELD_FLOGKIND => 'fLogKind',
                UserGuerrillaMissionLog::FIELD_FGROUPID => 'fGroupID',
                UserGuerrillaMissionLog::FIELD_FCOUNT => 'fCount',
                UserGuerrillaMissionLog::FIELD_FNOWSTEP => 'fNowStep',
            ],
            'valueCategoryInfos' => [
                // UserGuerrillaMissionLog::FIELD_FLOGKIND => ValueCommentator::CATEGORY_,
                UserGuerrillaMissionLog::FIELD_FTIME => ValueCommentator::CATEGORY_DATE_TIME_STR,
            ],
            'sortingFiledNames' => [
                UserGuerrillaMissionLog::FIELD_FUUID,
                UserGuerrillaMissionLog::FIELD_FLOGKIND,
                UserGuerrillaMissionLog::FIELD_FTIME,
            ],
            'filterComment' => self::getFilterWarningMessage(),
            'filterCategoryInfos' => [
                UserGuerrillaMissionLog::FIELD_FUUID => [
                    'label' => 'UUID',
                    //'category' => ValueCommentator::CATEGORY_,
                ],
                UserGuerrillaMissionLog::FIELD_FLOGKIND => [
                    'label' => 'LogKind',
                    'category' => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserGuerrillaMissionLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_GE) => [
                    'label' => 'StartDate(>=)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserGuerrillaMissionLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_LT) => [
                    'label' => 'EndDate(<)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_USER_PASS_MISSION_LOG] = [
            'modelClass' => $this->instanceFactory->userPassMissionLogModel(),
            'bUseStringResourceId' => false,
            'jsClassName' => 'RpgAUserPassMissionLogLookupTable',
            'orderFieldInfo' => [
                'defaultTitle' => '순서',
                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => [
                UserPassMissionLog::FIELD_FUUID => 'fUUID',
                UserPassMissionLog::FIELD_FMISSIONID => 'fMissionID',
                UserPassMissionLog::FIELD_FTIME => 'fTime',
                UserPassMissionLog::FIELD_FLOGKIND => 'fLogKind',
                UserPassMissionLog::FIELD_FKINDSTR => 'fKindStr',
            ],
            'valueCategoryInfos' => [
                // UserGuerrillaMissionLog::FIELD_FLOGKIND => ValueCommentator::CATEGORY_,
                UserPassMissionLog::FIELD_FTIME => ValueCommentator::CATEGORY_DATE_TIME_STR,
            ],
            'sortingFiledNames' => [
                UserPassMissionLog::FIELD_FUUID,
                UserPassMissionLog::FIELD_FLOGKIND,
                UserPassMissionLog::FIELD_FTIME,
            ],
            'filterComment' => self::getFilterWarningMessage(),
            'filterCategoryInfos' => [
                UserPassMissionLog::FIELD_FUUID => [
                    'label' => 'UUID',
                    //'category' => ValueCommentator::CATEGORY_,
                ],
                UserPassMissionLog::FIELD_FLOGKIND => [
                    'label' => 'LogKind',
                    'category' => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserPassMissionLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_GE) => [
                    'label' => 'StartDate(>=)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserPassMissionLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_LT) => [
                    'label' => 'EndDate(<)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_USER_ITEM_LOG] = [
            'modelClass' => $this->instanceFactory->userItemLogModel(),
            'bUseStringResourceId' => false,
            'jsClassName' => 'RpgAUserItemLogLookupTable',
            'bUseDownloadExcel' => true,
            'orderFieldInfo' => [
                'defaultTitle' => '순서',
                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => [
                UserItemLog::FIELD_FUUID => 'fUUID',
                UserItemLog::FIELD_FITEMUID => 'fItemUID',
                UserItemLog::FIELD_FLOGKIND => 'fLogKind',
                UserItemLog::FIELD_FITEMCNT => 'fItemCnt',
                UserItemLog::FIELD_FKINDSTR => 'fKindStr',
                UserItemLog::FIELD_FTIME => 'fTime',
            ],
            'valueCategoryInfos' => [
                UserItemLog::FIELD_FLOGKIND => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                UserItemLog::FIELD_FTIME => ValueCommentator::CATEGORY_DATE_TIME_STR,
            ],
            'sortingFiledNames' => [
                UserItemLog::FIELD_FUUID,
                UserItemLog::FIELD_FLOGKIND,
                UserItemLog::FIELD_FTIME,
            ],
            'filterComment' => self::getFilterWarningMessage(),
            'filterCategoryInfos' => [
                UserItemLog::FIELD_FUUID => [
                    'label' => 'UUID',
                    //'category' => ValueCommentator::CATEGORY_,
                ],
                UserItemLog::FIELD_FLOGKIND => [
                    'label' => 'LogKind',
                    'category' => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserItemLog::FIELD_FKINDSTR, GameLogModel::OPERATOR_TEXT_LIKE) => [
                    'label' => 'KindStr',
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserItemLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_GE) => [
                    'label' => 'StartDate(>=)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserItemLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_LT) => [
                    'label' => 'EndDate(<)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_USER_MAIL_LOG] = [
            'modelClass' => $this->instanceFactory->userMailLogModel(),
            'bUseStringResourceId' => false,
            'jsClassName' => 'RpgAUserMailLogLookupTable',
            'bUseDownloadExcel' => true,
            'orderFieldInfo' => [
                'defaultTitle' => '순서',
                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => [
                UserMailLog::FIELD_FUUID => 'fUUID',
                UserMailLog::FIELD_FMAILUID => 'fMailUID',
                UserMailLog::FIELD_FTIME => 'fTime',
                UserMailLog::FIELD_FLOGKIND => 'fLogKind',
                UserMailLog::FIELD_FKINDSTR => 'fKindStr',
            ],
            'valueCategoryInfos' => [
                UserMailLog::FIELD_FLOGKIND => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                UserMailLog::FIELD_FTIME => ValueCommentator::CATEGORY_DATE_TIME_STR,
            ],
            'sortingFiledNames' => [
                UserMailLog::FIELD_FUUID,
                UserMailLog::FIELD_FLOGKIND,
                UserMailLog::FIELD_FTIME,
            ],
            'filterComment' => self::getFilterWarningMessage(),
            'filterCategoryInfos' => [
                UserMailLog::FIELD_FUUID => [
                    'label' => 'UUID',
                    //'category' => ValueCommentator::CATEGORY_,
                ],
                UserMailLog::FIELD_FLOGKIND => [
                    'label' => 'LogKind',
                    'category' => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserMailLog::FIELD_FKINDSTR, GameLogModel::OPERATOR_TEXT_LIKE) => [
                    'label' => 'KindStr',
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserMailLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_GE) => [
                    'label' => 'StartDate(>=)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserMailLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_LT) => [
                    'label' => 'EndDate(<)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_USER_MARK_LOG] = [
            'modelClass' => $this->instanceFactory->userMarkLogModel(),
            'bUseStringResourceId' => false,
            'jsClassName' => 'RpgAUserMarkLogLookupTable',
            'bUseDownloadExcel' => true,
            'orderFieldInfo' => [
                'defaultTitle' => '순서',
                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => [
                UserMarkLog::FIELD_FUUID => 'fUUID',
                UserMarkLog::FIELD_FTIME => 'fTime',
                UserMarkLog::FIELD_FLOGKIND => 'fLogKind',
                UserMarkLog::FIELD_FTABLEID => 'fTableID',
            ],
            'valueCategoryInfos' => [
                UserMarkLog::FIELD_FLOGKIND => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                UserMarkLog::FIELD_FTIME => ValueCommentator::CATEGORY_DATE_TIME_STR,
            ],
            'sortingFiledNames' => [
                UserMarkLog::FIELD_FUUID,
                UserMarkLog::FIELD_FLOGKIND,
                UserMarkLog::FIELD_FTIME,
            ],
            'filterComment' => self::getFilterWarningMessage(),
            'filterCategoryInfos' => [
                UserMarkLog::FIELD_FUUID => [
                    'label' => 'UUID',
                    //'category' => ValueCommentator::CATEGORY_,
                ],
                UserMarkLog::FIELD_FLOGKIND => [
                    'label' => 'LogKind',
                    'category' => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserMarkLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_GE) => [
                    'label' => 'StartDate(>=)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserMarkLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_LT) => [
                    'label' => 'EndDate(<)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_USER_MONSTER_BOOK_LOG] = [
            'modelClass' => $this->instanceFactory->userMonsterBookLogModel(),
            'bUseStringResourceId' => false,
            'jsClassName' => 'RpgAUserMonsterBookLogLookupTable',
            'bUseDownloadExcel' => true,
            'orderFieldInfo' => [
                'defaultTitle' => '순서',
                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => [
                UserMonsterBookLog::FIELD_FUUID => 'fUUID',
                UserMonsterBookLog::FIELD_FTABLEID => 'fTableID',
                UserMonsterBookLog::FIELD_FLOGKIND => 'fLogKind',
                UserMonsterBookLog::FIELD_FKINDVAL_0 => 'fKindVal_0',
                UserMonsterBookLog::FIELD_FTIME => 'fTime',
            ],
            'valueCategoryInfos' => [
                UserMonsterBookLog::FIELD_FLOGKIND => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                UserMonsterBookLog::FIELD_FTIME => ValueCommentator::CATEGORY_DATE_TIME_STR,
            ],
            'sortingFiledNames' => [
                UserMonsterBookLog::FIELD_FUUID,
                UserMonsterBookLog::FIELD_FLOGKIND,
                UserMonsterBookLog::FIELD_FTIME,
            ],
            'filterComment' => self::getFilterWarningMessage(),
            'filterCategoryInfos' => [
                UserMonsterBookLog::FIELD_FUUID => [
                    'label' => 'UUID',
                    //'category' => ValueCommentator::CATEGORY_,
                ],
                UserMonsterBookLog::FIELD_FLOGKIND => [
                    'label' => 'LogKind',
                    'category' => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserMonsterBookLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_GE) => [
                    'label' => 'StartDate(>=)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserMonsterBookLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_LT) => [
                    'label' => 'EndDate(<)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_USER_ROOM_FIGURE_SLOT_LOG] = [
            'modelClass' => $this->instanceFactory->userRoomFigureSlotLogModel(),
            'bUseStringResourceId' => false,
            'jsClassName' => 'RpgAUserRoomFigureSlotLogLookupTable',
            'bUseDownloadExcel' => true,
            'orderFieldInfo' => [
                'defaultTitle' => '순서',
                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => [
                UserRoomFigureSlotLog::FIELD_FUUID => 'fUUID',
                UserRoomFigureSlotLog::FIELD_FFIGURESLOTNUM => 'fFigureSlotNum',
                UserRoomFigureSlotLog::FIELD_FTIME => 'fTime',
                UserRoomFigureSlotLog::FIELD_FLOGKIND => 'fLogKind',
                UserRoomFigureSlotLog::FIELD_FKINDSTR => 'fKindStr',
            ],
            'valueCategoryInfos' => [
                UserRoomFigureSlotLog::FIELD_FLOGKIND => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                UserRoomFigureSlotLog::FIELD_FTIME => ValueCommentator::CATEGORY_DATE_TIME_STR,
            ],
            'sortingFiledNames' => [
                UserRoomFigureSlotLog::FIELD_FUUID,
                UserRoomFigureSlotLog::FIELD_FLOGKIND,
                UserRoomFigureSlotLog::FIELD_FTIME,
            ],
            'filterComment' => self::getFilterWarningMessage(),
            'filterCategoryInfos' => [
                UserRoomFigureSlotLog::FIELD_FUUID => [
                    'label' => 'UUID',
                    //'category' => ValueCommentator::CATEGORY_,
                ],
                UserRoomFigureSlotLog::FIELD_FLOGKIND => [
                    'label' => 'LogKind',
                    'category' => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserRoomFigureSlotLog::FIELD_FKINDSTR, GameLogModel::OPERATOR_TEXT_LIKE) => [
                    'label' => 'KindStr',
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserRoomFigureSlotLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_GE) => [
                    'label' => 'StartDate(>=)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserRoomFigureSlotLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_LT) => [
                    'label' => 'EndDate(<)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_USER_ROOM_PURCHASE_LOG] = [
            'modelClass' => $this->instanceFactory->userRoomPurchaseLogModel(),
            'bUseStringResourceId' => false,
            'jsClassName' => 'RpgAUserRoomPurchaseLogLookupTable',
            'bUseDownloadExcel' => true,
            'orderFieldInfo' => [
                'defaultTitle' => '순서',
                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => [
                UserRoomPurchaseLog::FIELD_FUUID => 'fUUID',
                UserRoomPurchaseLog::FIELD_FTABLEID => 'fTableID',
                UserRoomPurchaseLog::FIELD_FKINDSTR => 'fKindStr',
                UserRoomPurchaseLog::FIELD_FTIME => 'fTime',
                UserRoomPurchaseLog::FIELD_FLOGKIND => 'fLogKind',
            ],
            'valueCategoryInfos' => [
                UserRoomPurchaseLog::FIELD_FLOGKIND => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                UserRoomPurchaseLog::FIELD_FTIME => ValueCommentator::CATEGORY_DATE_TIME_STR,
            ],
            'sortingFiledNames' => [
                UserRoomPurchaseLog::FIELD_FUUID,
                UserRoomPurchaseLog::FIELD_FLOGKIND,
                UserRoomPurchaseLog::FIELD_FTIME,
            ],
            'filterComment' => self::getFilterWarningMessage(),
            'filterCategoryInfos' => [
                UserRoomPurchaseLog::FIELD_FUUID => [
                    'label' => 'UUID',
                    //'category' => ValueCommentator::CATEGORY_,
                ],
                UserRoomPurchaseLog::FIELD_FLOGKIND => [
                    'label' => 'LogKind',
                    'category' => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserRoomPurchaseLog::FIELD_FKINDSTR, GameLogModel::OPERATOR_TEXT_LIKE) => [
                    'label' => 'KindStr',
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserRoomPurchaseLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_GE) => [
                    'label' => 'StartDate(>=)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserRoomPurchaseLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_LT) => [
                    'label' => 'EndDate(<)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_USER_ROOM_THEME_SLOT_LOG] = [
            'modelClass' => $this->instanceFactory->userRoomThemeSlotLogModel(),
            'bUseStringResourceId' => false,
            'jsClassName' => 'RpgAUserRoomThemeSlotLogLookupTable',
            'bUseDownloadExcel' => true,
            'orderFieldInfo' => [
                'defaultTitle' => '순서',
                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => [
                UserRoomThemeSlotLog::FIELD_FUUID => 'fUUID',
                UserRoomThemeSlotLog::FIELD_FTHEMESLOTNUM => 'fThemeSlotNum',
                UserRoomThemeSlotLog::FIELD_FTIME => 'fTime',
                UserRoomThemeSlotLog::FIELD_FLOGKIND => 'fLogKind',
                UserRoomThemeSlotLog::FIELD_FKINDSTR => 'fKindStr',
            ],
            'valueCategoryInfos' => [
                UserRoomThemeSlotLog::FIELD_FLOGKIND => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                UserRoomThemeSlotLog::FIELD_FTIME => ValueCommentator::CATEGORY_DATE_TIME_STR,
            ],
            'sortingFiledNames' => [
                UserRoomThemeSlotLog::FIELD_FUUID,
                UserRoomThemeSlotLog::FIELD_FLOGKIND,
                UserRoomThemeSlotLog::FIELD_FTIME,
            ],
            'filterComment' => self::getFilterWarningMessage(),
            'filterCategoryInfos' => [
                UserRoomThemeSlotLog::FIELD_FUUID => [
                    'label' => 'UUID',
                    //'category' => ValueCommentator::CATEGORY_,
                ],
                UserRoomThemeSlotLog::FIELD_FLOGKIND => [
                    'label' => 'LogKind',
                    'category' => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserRoomThemeSlotLog::FIELD_FKINDSTR, GameLogModel::OPERATOR_TEXT_LIKE) => [
                    'label' => 'KindStr',
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserRoomThemeSlotLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_GE) => [
                    'label' => 'StartDate(>=)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserRoomThemeSlotLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_LT) => [
                    'label' => 'EndDate(<)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_USER_STAGE_LOG] = [
            'modelClass' => $this->instanceFactory->userStageLogModel(),
            'bUseStringResourceId' => false,
            'jsClassName' => 'RpgAUserStageLogLookupTable',
            'bUseDownloadExcel' => true,
            'orderFieldInfo' => [
                'defaultTitle' => '순서',
                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => [
                UserStageLog::FIELD_FUUID => 'fUUID',
                UserStageLog::FIELD_FSTAGEID => 'fStageID',
                UserStageLog::FIELD_FLOGKIND => 'fLogKind',
                UserStageLog::FIELD_FKINDSTR => 'fKindStr',
                UserStageLog::FIELD_FTIME => 'fTime',
            ],
            'valueCategoryInfos' => [
                UserStageLog::FIELD_FLOGKIND => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                UserStageLog::FIELD_FTIME => ValueCommentator::CATEGORY_DATE_TIME_STR,
                UserStageLog::FIELD_FSTAGEID => ValueCommentator::CATEGORY_STAGE_ID,
            ],
            'sortingFiledNames' => [
                UserStageLog::FIELD_FUUID,
                UserStageLog::FIELD_FLOGKIND,
                UserStageLog::FIELD_FTIME,
            ],
            'filterComment' => self::getFilterWarningMessage(),
            'filterCategoryInfos' => [
                UserStageLog::FIELD_FUUID => [
                    'label' => 'UUID',
                    //'category' => ValueCommentator::CATEGORY_,
                ],
                UserStageLog::FIELD_FLOGKIND => [
                    'label' => 'LogKind',
                    'category' => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserStageLog::FIELD_FKINDSTR, GameLogModel::OPERATOR_TEXT_LIKE) => [
                    'label' => 'KindStr',
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserStageLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_GE) => [
                    'label' => 'StartDate(>=)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserStageLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_LT) => [
                    'label' => 'EndDate(<)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_USER_STAGE_CLEAR_LOG] = [
            'modelClass' => $this->instanceFactory->userStageClearLogModel(),
            'bUseStringResourceId' => false,
            'jsClassName' => 'RpgAUserStageClearLogLookupTable',
            'bUseDownloadExcel' => true,
            'orderFieldInfo' => [
                'defaultTitle' => '순서',
                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => [
                UserStageClearLog::FIELD_FUUID => 'fUUID',
                UserStageClearLog::FIELD_FTABLEID => 'fTableID',
                UserStageClearLog::FIELD_FLOGKIND => 'fLogKind',
                UserStageClearLog::FIELD_FKINDSTR => 'fKindStr',
                UserStageClearLog::FIELD_FTIME => 'fTime',
            ],
            'valueCategoryInfos' => [
                UserStageClearLog::FIELD_FLOGKIND => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                UserStageClearLog::FIELD_FTIME => ValueCommentator::CATEGORY_DATE_TIME_STR,
                UserStageClearLog::FIELD_FTABLEID => ValueCommentator::CATEGORY_STAGE_ID,
            ],
            'sortingFiledNames' => [
                UserStageClearLog::FIELD_FUUID,
                UserStageClearLog::FIELD_FLOGKIND,
                UserStageClearLog::FIELD_FTIME,
            ],
            'filterComment' => self::getFilterWarningMessage(),
            'filterCategoryInfos' => [
                UserStageClearLog::FIELD_FUUID => [
                    'label' => 'UUID',
                    //'category' => ValueCommentator::CATEGORY_,
                ],
                UserStageClearLog::FIELD_FLOGKIND => [
                    'label' => 'LogKind',
                    'category' => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserStageClearLog::FIELD_FKINDSTR, GameLogModel::OPERATOR_TEXT_LIKE) => [
                    'label' => 'KindStr',
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserStageClearLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_GE) => [
                    'label' => 'StartDate(>=)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserStageClearLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_LT) => [
                    'label' => 'EndDate(<)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_USER_STORE_SALE_LOG] = [
            'modelClass' => $this->instanceFactory->userStoreSaleLogModel(),
            'bUseStringResourceId' => false,
            'jsClassName' => 'RpgAUserStoreSaleLogLookupTable',
            'bUseDownloadExcel' => true,
            'orderFieldInfo' => [
                'defaultTitle' => '순서',
                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => [
                UserStoreSaleLog::FIELD_FUUID => 'fUUID',
                UserStoreSaleLog::FIELD_FTABLEID => 'fTableID',
                UserStoreSaleLog::FIELD_FLOGKIND => 'fLogKind',
                UserStoreSaleLog::FIELD_FKINDVAL_0 => 'fKindVal_0',
                UserStoreSaleLog::FIELD_FTIME => 'fTime',
            ],
            'valueCategoryInfos' => [
                UserStoreSaleLog::FIELD_FLOGKIND => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                UserStoreSaleLog::FIELD_FTIME => ValueCommentator::CATEGORY_DATE_TIME_STR,
            ],
            'sortingFiledNames' => [
                UserStoreSaleLog::FIELD_FUUID,
                UserStoreSaleLog::FIELD_FLOGKIND,
                UserStoreSaleLog::FIELD_FTIME,
            ],
            'filterComment' => self::getFilterWarningMessage(),
            'filterCategoryInfos' => [
                UserStoreSaleLog::FIELD_FUUID => [
                    'label' => 'UUID',
                    //'category' => ValueCommentator::CATEGORY_,
                ],
                UserStoreSaleLog::FIELD_FLOGKIND => [
                    'label' => 'LogKind',
                    'category' => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserStoreSaleLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_GE) => [
                    'label' => 'StartDate(>=)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserStoreSaleLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_LT) => [
                    'label' => 'EndDate(<)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_USER_TIME_ATK_STAGE_REC_LOG] = [
            'modelClass' => $this->instanceFactory->userTimeAtkStageRecLogModel(),
            'bUseStringResourceId' => false,
            'jsClassName' => 'RpgAUserTimeAtkStageRecLogLookupTable',
            'bUseDownloadExcel' => true,
            'orderFieldInfo' => [
                'defaultTitle' => '순서',
                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => [
                UserTimeAtkStageRecLog::FIELD_FUUID => 'fUUID',
                UserTimeAtkStageRecLog::FIELD_FSTAGEID => 'fStageID',
                UserTimeAtkStageRecLog::FIELD_FLOGKIND => 'fLogKind',
                UserTimeAtkStageRecLog::FIELD_FKINDSTR => 'fKindStr',
                UserTimeAtkStageRecLog::FIELD_FTIME => 'fTime',
            ],
            'valueCategoryInfos' => [
                UserTimeAtkStageRecLog::FIELD_FLOGKIND => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                UserTimeAtkStageRecLog::FIELD_FTIME => ValueCommentator::CATEGORY_DATE_TIME_STR,
                UserTimeAtkStageRecLog::FIELD_FSTAGEID => ValueCommentator::CATEGORY_STAGE_ID,
            ],
            'sortingFiledNames' => [
                UserTimeAtkStageRecLog::FIELD_FUUID,
                UserTimeAtkStageRecLog::FIELD_FLOGKIND,
                UserTimeAtkStageRecLog::FIELD_FTIME,
            ],
            'filterComment' => self::getFilterWarningMessage(),
            'filterCategoryInfos' => [
                UserTimeAtkStageRecLog::FIELD_FUUID => [
                    'label' => 'UUID',
                    //'category' => ValueCommentator::CATEGORY_,
                ],
                UserTimeAtkStageRecLog::FIELD_FLOGKIND => [
                    'label' => 'LogKind',
                    'category' => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserTimeAtkStageRecLog::FIELD_FKINDSTR, GameLogModel::OPERATOR_TEXT_LIKE) => [
                    'label' => 'KindStr',
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserTimeAtkStageRecLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_GE) => [
                    'label' => 'StartDate(>=)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserTimeAtkStageRecLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_LT) => [
                    'label' => 'EndDate(<)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_USER_WEAPON_LOG] = [
            'modelClass' => $this->instanceFactory->userWeaponLogModel(),
            'bUseStringResourceId' => false,
            'jsClassName' => 'RpgAUserWeaponLogLookupTable',
            'bUseDownloadExcel' => true,
            'orderFieldInfo' => [
                'defaultTitle' => '순서',
                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => [
                UserWeaponLog::FIELD_FUUID => 'fUUID',
                UserWeaponLog::FIELD_FWEAPONUID => 'fWeaponUID',
                UserWeaponLog::FIELD_FLOGKIND => 'fLogKind',
                UserWeaponLog::FIELD_FKINDSTR => 'fKindStr',
                UserWeaponLog::FIELD_FTIME => 'fTime',
            ],
            'valueCategoryInfos' => [
                UserWeaponLog::FIELD_FLOGKIND => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                UserWeaponLog::FIELD_FTIME => ValueCommentator::CATEGORY_DATE_TIME_STR,
            ],
            'sortingFiledNames' => [
                UserWeaponLog::FIELD_FUUID,
                UserWeaponLog::FIELD_FLOGKIND,
                UserWeaponLog::FIELD_FTIME,
            ],
            'filterComment' => self::getFilterWarningMessage(),
            'filterCategoryInfos' => [
                UserWeaponLog::FIELD_FUUID => [
                    'label' => 'UUID',
                    //'category' => ValueCommentator::CATEGORY_,
                ],
                UserWeaponLog::FIELD_FLOGKIND => [
                    'label' => 'LogKind',
                    'category' => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserWeaponLog::FIELD_FKINDSTR, GameLogModel::OPERATOR_TEXT_LIKE) => [
                    'label' => 'KindStr',
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserWeaponLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_GE) => [
                    'label' => 'StartDate(>=)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserWeaponLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_LT) => [
                    'label' => 'EndDate(<)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_USER_WEAPON_BOOK_LOG] = [
            'modelClass' => $this->instanceFactory->userWeaponBookLogModel(),
            'bUseStringResourceId' => false,
            'jsClassName' => 'RpgAUserWeaponBookLogLookupTable',
            'bUseDownloadExcel' => true,
            'orderFieldInfo' => [
                'defaultTitle' => '순서',
                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => [
                UserWeaponBookLog::FIELD_FUUID => 'fUUID',
                UserWeaponBookLog::FIELD_FTABLEID => 'fTableID',
                UserWeaponBookLog::FIELD_FLOGKIND => 'fLogKind',
                UserWeaponBookLog::FIELD_FKINDVAL_0 => 'fKindVal_0',
                UserWeaponBookLog::FIELD_FTIME => 'fTime',
            ],
            'valueCategoryInfos' => [
                UserWeaponBookLog::FIELD_FLOGKIND => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                UserWeaponBookLog::FIELD_FTIME => ValueCommentator::CATEGORY_DATE_TIME_STR,
                UserWeaponBookLog::FIELD_FTABLEID => ValueCommentator::CATEGORY_WEAPON_ID,
            ],
            'sortingFiledNames' => [
                UserWeaponBookLog::FIELD_FUUID,
                UserWeaponBookLog::FIELD_FLOGKIND,
                UserWeaponBookLog::FIELD_FTIME,
            ],
            'filterComment' => self::getFilterWarningMessage(),
            'filterCategoryInfos' => [
                UserWeaponBookLog::FIELD_FUUID => [
                    'label' => 'UUID',
                    //'category' => ValueCommentator::CATEGORY_,
                ],
                UserWeaponBookLog::FIELD_FLOGKIND => [
                    'label' => 'LogKind',
                    'category' => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserWeaponBookLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_GE) => [
                    'label' => 'StartDate(>=)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserWeaponBookLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_LT) => [
                    'label' => 'EndDate(<)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_USER_WEEK_MISSION_LOG] = [
            'modelClass' => $this->instanceFactory->userWeekMissionLogModel(),
            'bUseStringResourceId' => false,
            'jsClassName' => 'RpgAUserWeekMissionLogLookupTable',
            'bUseDownloadExcel' => true,
            'orderFieldInfo' => [
                'defaultTitle' => '순서',
                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => [
                UserWeekMissionLog::FIELD_FUUID => 'fUUID',
                UserWeekMissionLog::FIELD_FTIME => 'fTime',
                UserWeekMissionLog::FIELD_FLOGKIND => 'fLogKind',
                UserWeekMissionLog::FIELD_FKINDSTR => 'fKindStr',
            ],
            'valueCategoryInfos' => [
                UserWeekMissionLog::FIELD_FLOGKIND => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                UserWeekMissionLog::FIELD_FTIME => ValueCommentator::CATEGORY_DATE_TIME_STR,
            ],
            'sortingFiledNames' => [
                UserWeekMissionLog::FIELD_FUUID,
                UserWeekMissionLog::FIELD_FLOGKIND,
                UserWeekMissionLog::FIELD_FTIME,
            ],
            'filterComment' => self::getFilterWarningMessage(),
            'filterCategoryInfos' => [
                UserWeekMissionLog::FIELD_FUUID => [
                    'label' => 'UUID',
                    //'category' => ValueCommentator::CATEGORY_,
                ],
                UserWeekMissionLog::FIELD_FLOGKIND => [
                    'label' => 'LogKind',
                    'category' => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserWeekMissionLog::FIELD_FKINDSTR, GameLogModel::OPERATOR_TEXT_LIKE) => [
                    'label' => 'KindStr',
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserWeekMissionLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_GE) => [
                    'label' => 'StartDate(>=)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserWeekMissionLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_LT) => [
                    'label' => 'EndDate(<)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_USER_LOBBY_THEME_LOG] = [
            'modelClass' => $this->instanceFactory->userLobbyThemeLogModel(),
            'bUseStringResourceId' => false,
            'jsClassName' => 'RpgAUserLobbyThemeLogLookupTable',
            'bUseDownloadExcel' => true,
            'orderFieldInfo' => [
                'defaultTitle' => '순서',
                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => [
                UserLobbyThemeLog::FIELD_FUUID => 'fUUID',
                UserLobbyThemeLog::FIELD_FTIME => 'fTime',
                UserLobbyThemeLog::FIELD_FLOGKIND => 'fLogKind',
                UserLobbyThemeLog::FIELD_FTABLEID => 'fTableID',
            ],
            'valueCategoryInfos' => [
                UserLobbyThemeLog::FIELD_FLOGKIND => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                UserLobbyThemeLog::FIELD_FTIME => ValueCommentator::CATEGORY_DATE_TIME_STR,
            ],
            'sortingFiledNames' => [
                UserLobbyThemeLog::FIELD_FUUID,
                UserLobbyThemeLog::FIELD_FLOGKIND,
                UserLobbyThemeLog::FIELD_FTIME,
            ],
            'filterComment' => self::getFilterWarningMessage(),
            'filterCategoryInfos' => [
                UserLobbyThemeLog::FIELD_FUUID => [
                    'label' => 'UUID',
                    //'category' => ValueCommentator::CATEGORY_,
                ],
                UserLobbyThemeLog::FIELD_FLOGKIND => [
                    'label' => 'LogKind',
                    'category' => ValueCommentator::CATEGORY_GAME_LOG_KIND,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserMarkLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_GE) => [
                    'label' => 'StartDate(>=)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserMarkLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_LT) => [
                    'label' => 'EndDate(<)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
            ],
        ];

        // game log table 설정. 종료



        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_CCU] = [
            'modelClass' => $this->instanceFactory->infoUserCountLogModel(),
            'bUseStringResourceId' => true,
            'jsClassName' => 'RpgAInfoUserCountLogLookupTable',
            'bUseDownloadExcel' => true,
            'orderFieldInfo' => [
                'defaultTitle' => '순서',
                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => [
                InfoUserCountLog::FIELD_FTOTAL => '동시 접속 유저 수',
                InfoUserCountLog::FIELD_FTIME => '측정 일시',
                InfoUserCountLog::FIELD_FANDROID => 'Adroid',
                InfoUserCountLog::FIELD_FIOS => 'IOS',
            ],
            'valueCategoryInfos' => [
                InfoUserCountLog::FIELD_FTOTAL => ValueCommentator::CATEGORY_COUNT_HUMAN,
                InfoUserCountLog::FIELD_FTIME => ValueCommentator::CATEGORY_DATE_TIME_STR,
            ],
            'sortingFiledNames' => [
                InfoUserCountLog::FIELD_FTIME,
            ],
            'filterCategoryInfos' => [
                GameLogModel::createFilterKeyNameWithOperator(InfoUserCountLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_GE) => [
                    'label' => 'StartDate(>=)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
                GameLogModel::createFilterKeyNameWithOperator(InfoUserCountLog::FIELD_FTIME, GameLogModel::OPERATOR_TEXT_LT) => [
                    'label' => 'EndDate(<)',
                    'category' => ValueCommentator::CATEGORY_DATE_TIME_STR_SIMPLE,
                ],
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_HOUR_PAYMENT] = [
            'modelClass' => $this->instanceFactory->hourPaymentModel(),
            'bUseStringResourceId' => true,
            'jsClassName' => 'RpgAHourPaymentLookupTable',
            'bUseDownloadExcel' => true,
            'bAsc' => false,
            'initDisplayLength' => 25,
            'orderFieldInfo' => [
                'defaultTitle' => 'Order',
//                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => [
                HourPayment::FIELD_DATE_HOUR => 'Date Hour',
                HourPayment::FIELD_PAYMENT => 'Payment',
            ],
            'valueCategoryInfos' => [
                HourPayment::FIELD_PAYMENT => ValueCommentator::CATEGORY_CURRENCY_JPY,
            ],
            'sortingFiledNames' => [
                HourPayment::FIELD_DATE_HOUR
            ],
            'filterCategoryInfos' => [
                'yyyymmddStart' => [
                    'label' => 'StartDate(>=)',
                    'category' => ValueCommentator::CATEGORY_YYYYMMDD_DATE_SIMPLE,
//                    'defaultValue' => TimeUtil::getYmdFromTime(max(strtotime('-9 day', NOW), $this->instanceFactory->kpiFactory()->getFirstTime())),
                ],
                'yyyymmddEnd' => [
                    'label' => 'EndDate(<=)',
                    'category' => ValueCommentator::CATEGORY_YYYYMMDD_DATE_SIMPLE,
//                    'defaultValue' => TimeUtil::getYmdFromTime(NOW),
                ],
            ],
        ];

        $rankDauInfo = [
            'fieldDefaultTitles' => [
                RankDau::FIELD_DATE => 'Date',
                RankDau::FIELD_TOTAL => 'Total',
            ],
            'valueCategoryInfos' => [
                RankDau::FIELD_TOTAL => ValueCommentator::CATEGORY_COUNT_HUMAN,
            ],
        ];
//        $rankMax = $this->instanceFactory->rankDauModel()->getRankMax();
//        for ($rank = 1; $rank <= $rankMax; $rank++) {
//            $rankDauInfo['fieldDefaultTitles'][RankDau::getCountFieldName($rank)] = "{$rank}";
//            $rankDauInfo['fieldDefaultTitles'][RankDau::getRateFieldName($rank)] = "{$rank}";
//
//            $rankDauInfo['valueCategoryInfos'][RankDau::getCountFieldName($rank)] = ValueCommentator::CATEGORY_COUNT_HUMAN;
//            $rankDauInfo['valueCategoryInfos'][RankDau::getRateFieldName($rank)] = ValueCommentator::CATEGORY_PERCENT_RATE;
//        }
        $rankGroupMax = $this->instanceFactory->rankDauModel()->getRankGroupMax();
        for ($rankGroup = 1; $rankGroup <= $rankGroupMax; $rankGroup++) {
            $rankStart = ($rankGroup - 1) * 5 + 1;
            $rankEnd = $rankStart + 4;
            $rankDauInfo['fieldDefaultTitles'][RankDau::getRankGroupRateFieldName($rankGroup)] = "Rank {$rankStart}~{$rankEnd}";

            $rankDauInfo['valueCategoryInfos'][RankDau::getRankGroupRateFieldName($rankGroup)] = ValueCommentator::CATEGORY_PERCENT_RATE;
        }
        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_RAMK_DAU] = [
            'modelClass' => $this->instanceFactory->rankDauModel(),
            'bUseStringResourceId' => false,
            'jsClassName' => 'RpgARankDauLookupTable',
            'bUseDownloadExcel' => true,
            'bAsc' => false,
            'initDisplayLength' => 10,
            'orderFieldInfo' => [
                'defaultTitle' => 'Order',
//                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => $rankDauInfo['fieldDefaultTitles'],
            'valueCategoryInfos' => $rankDauInfo['valueCategoryInfos'],
            'sortingFiledNames' => [
                RankDau::FIELD_DATE
            ],
            'filterCategoryInfos' => [
                'yyyymmddStart' => [
                    'label' => 'StartDate(>=)',
                    'category' => ValueCommentator::CATEGORY_YYYYMMDD_DATE_SIMPLE,
//                    'defaultValue' => TimeUtil::getYmdFromTime(max(strtotime('-9 day', NOW), $this->instanceFactory->kpiFactory()->getFirstTime())),
                ],
                'yyyymmddEnd' => [
                    'label' => 'EndDate(<=)',
                    'category' => ValueCommentator::CATEGORY_YYYYMMDD_DATE_SIMPLE,
//                    'defaultValue' => TimeUtil::getYmdFromTime(NOW),
                ],
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_DAILY_KPI] = [
            'modelClass' => $this->instanceFactory->dailyKpiModel(),
            'bUseStringResourceId' => true,
            'jsClassName' => 'RpgADailyKpiLookupTable',
            'bUseDownloadExcel' => true,
            'bAsc' => false,
            'orderFieldInfo' => [
                'defaultTitle' => 'Order',
//                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => [
                DailyKpi::FIELD_DATE => 'Date',
                DailyKpi::FIELD_PAYMENT => 'Payment',

                DailyKpi::FIELD_NRU => '[Account] NRU',
                DailyKpi::FIELD_DAU => '[Account] DAU',
                DailyKpi::FIELD_ARPDAU => '[Account] ARPDAU',
                DailyKpi::FIELD_DPU => '[Account] DPU',
                DailyKpi::FIELD_ARPDPU => '[Account] ARPDPU',

                DailyKpi::FIELD_DRPU_PAYMENT => '[Account] DRPU payment',
                DailyKpi::FIELD_DRPU => '[Account] DRPU',
                DailyKpi::FIELD_ARPDRPU => '[Account] ARPDRPU',

                DailyKpi::FIELD_NRU_BY_UUID => '[UUID] NRU',
                DailyKpi::FIELD_DAU_BY_UUID => '[UUID] DAU',
                DailyKpi::FIELD_ARPDAU_BY_UUID => '[UUID] ARPDAU',
                DailyKpi::FIELD_DPU_BY_UUID => '[UUID] DPU',
                DailyKpi::FIELD_ARPDPU_BY_UUID => '[UUID] ARPDPU',

                DailyKpi::FIELD_DRPU_PAYMENT_BY_UUID => '[UUID] DRPU payment',
                DailyKpi::FIELD_DRPU_BY_UUID => '[UUID] DRPU',
                DailyKpi::FIELD_ARPDRPU_BY_UUID => '[UUID] ARPDRPU',

            ],
            'valueCategoryInfos' => [
                DailyKpi::FIELD_PAYMENT => ValueCommentator::CATEGORY_CURRENCY_JPY,
                DailyKpi::FIELD_DRPU_PAYMENT => ValueCommentator::CATEGORY_CURRENCY_JPY,
                DailyKpi::FIELD_DRPU_PAYMENT_BY_UUID => ValueCommentator::CATEGORY_CURRENCY_JPY,
                DailyKpi::FIELD_ARPDAU => ValueCommentator::CATEGORY_CURRENCY_JPY,
                DailyKpi::FIELD_ARPDPU => ValueCommentator::CATEGORY_CURRENCY_JPY,
                DailyKpi::FIELD_ARPDRPU => ValueCommentator::CATEGORY_CURRENCY_JPY,
                DailyKpi::FIELD_ARPDAU_BY_UUID => ValueCommentator::CATEGORY_CURRENCY_JPY,
                DailyKpi::FIELD_ARPDPU_BY_UUID => ValueCommentator::CATEGORY_CURRENCY_JPY,
                DailyKpi::FIELD_ARPDRPU_BY_UUID => ValueCommentator::CATEGORY_CURRENCY_JPY,

                DailyKpi::FIELD_NRU => ValueCommentator::CATEGORY_COUNT_HUMAN,
                DailyKpi::FIELD_DRPU => ValueCommentator::CATEGORY_COUNT_HUMAN,
                DailyKpi::FIELD_DAU => ValueCommentator::CATEGORY_COUNT_HUMAN,
                DailyKpi::FIELD_DPU => ValueCommentator::CATEGORY_COUNT_HUMAN,
                DailyKpi::FIELD_NRU_BY_UUID => ValueCommentator::CATEGORY_COUNT_HUMAN,
                DailyKpi::FIELD_DRPU_BY_UUID => ValueCommentator::CATEGORY_COUNT_HUMAN,
                DailyKpi::FIELD_DAU_BY_UUID => ValueCommentator::CATEGORY_COUNT_HUMAN,
                DailyKpi::FIELD_DPU_BY_UUID => ValueCommentator::CATEGORY_COUNT_HUMAN,
            ],
            'sortingFiledNames' => [
                DailyKpi::FIELD_DATE
            ],
            'filterCategoryInfos' => [
                'yyyymmddStart' => [
                    'label' => 'StartDate(>=)',
                    'category' => ValueCommentator::CATEGORY_YYYYMMDD_DATE_SIMPLE,
//                    'defaultValue' => TimeUtil::getYmdFromTime(max(strtotime('-9 day', NOW), $this->instanceFactory->kpiFactory()->getFirstTime())),
                ],
                'yyyymmddEnd' => [
                    'label' => 'EndDate(<=)',
                    'category' => ValueCommentator::CATEGORY_YYYYMMDD_DATE_SIMPLE,
//                    'defaultValue' => TimeUtil::getYmdFromTime(NOW),
                ],
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_WEEKLY_KPI] = [
            'modelClass' => $this->instanceFactory->weeklyKpiModel(),
            'bUseStringResourceId' => true,
            'jsClassName' => 'RpgAWeeklyKpiLookupTable',
            'bUseDownloadExcel' => true,
            'bAsc' => false,
            'orderFieldInfo' => [
                'defaultTitle' => 'Order',
//                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => [
                WeeklyKpi::FIELD_DATE => 'Date',
                WeeklyKpi::FIELD_PAYMENT => 'Payment',

                WeeklyKpi::FIELD_NRU => '[Account] NRU',
                WeeklyKpi::FIELD_WAU => '[Account] WAU',
                WeeklyKpi::FIELD_ARPWAU => '[Account] ARPWAU',
                WeeklyKpi::FIELD_WPU => '[Account] WPU',
                WeeklyKpi::FIELD_ARPWPU => '[Account] ARPWPU',

                WeeklyKpi::FIELD_NRU_BY_UUID => '[UUID] NRU',
                WeeklyKpi::FIELD_WAU_BY_UUID => '[UUID] WAU',
                WeeklyKpi::FIELD_ARPWAU_BY_UUID => '[UUID] ARPWAU',
                WeeklyKpi::FIELD_WPU_BY_UUID => '[UUID] WPU',
                WeeklyKpi::FIELD_ARPWPU_BY_UUID => '[UUID] ARPWPU',
            ],
            'valueCategoryInfos' => [
                WeeklyKpi::FIELD_PAYMENT => ValueCommentator::CATEGORY_CURRENCY_JPY,
                WeeklyKpi::FIELD_ARPWAU => ValueCommentator::CATEGORY_CURRENCY_JPY,
                WeeklyKpi::FIELD_ARPWPU => ValueCommentator::CATEGORY_CURRENCY_JPY,
                WeeklyKpi::FIELD_ARPWAU_BY_UUID => ValueCommentator::CATEGORY_CURRENCY_JPY,
                WeeklyKpi::FIELD_ARPWPU_BY_UUID => ValueCommentator::CATEGORY_CURRENCY_JPY,

                WeeklyKpi::FIELD_NRU => ValueCommentator::CATEGORY_COUNT_HUMAN,
                WeeklyKpi::FIELD_WAU => ValueCommentator::CATEGORY_COUNT_HUMAN,
                WeeklyKpi::FIELD_WPU => ValueCommentator::CATEGORY_COUNT_HUMAN,
                WeeklyKpi::FIELD_NRU_BY_UUID => ValueCommentator::CATEGORY_COUNT_HUMAN,
                WeeklyKpi::FIELD_WAU_BY_UUID => ValueCommentator::CATEGORY_COUNT_HUMAN,
                WeeklyKpi::FIELD_WPU_BY_UUID => ValueCommentator::CATEGORY_COUNT_HUMAN,
            ],
            'sortingFiledNames' => [
                WeeklyKpi::FIELD_DATE
            ],
            'filterCategoryInfos' => [
                'yyyymmddStart' => [
                    'label' => 'StartDate(>=)',
                    'category' => ValueCommentator::CATEGORY_YYYYMMDD_DATE_SIMPLE,
//                    'defaultValue' => TimeUtil::getYmdFromTime(max(strtotime('-9 week', NOW), $this->instanceFactory->kpiFactory()->getFirstTime())),
                ],
                'yyyymmddEnd' => [
                    'label' => 'EndDate(<=)',
                    'category' => ValueCommentator::CATEGORY_YYYYMMDD_DATE_SIMPLE,
//                    'defaultValue' => TimeUtil::getYmdFromTime(NOW),
                ],
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_MONTHLY_KPI] = [
            'modelClass' => $this->instanceFactory->monthlyKpiModel(),
            'bUseStringResourceId' => true,
            'jsClassName' => 'RpgAMonthlyKpiLookupTable',
            'bUseDownloadExcel' => true,
            'bAsc' => false,
            'orderFieldInfo' => [
                'defaultTitle' => 'Order',
//                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => [
                MonthlyKpi::FIELD_DATE => 'Date',
                MonthlyKpi::FIELD_PAYMENT => 'Payment',

                MonthlyKpi::FIELD_NRU => '[Account] NRU',
                MonthlyKpi::FIELD_MAU => '[Account] MAU',
                MonthlyKpi::FIELD_ARPMAU => '[Account] ARPMAU',
                MonthlyKpi::FIELD_MPU => '[Account] MPU',
                MonthlyKpi::FIELD_ARPMPU => '[Account] ARPMPU',

                MonthlyKpi::FIELD_NRU_BY_UUID => '[UUID] NRU',
                MonthlyKpi::FIELD_MAU_BY_UUID => '[UUID] MAU',
                MonthlyKpi::FIELD_ARPMAU_BY_UUID => '[UUID] ARPMAU',
                MonthlyKpi::FIELD_MPU_BY_UUID => '[UUID] MPU',
                MonthlyKpi::FIELD_ARPMPU_BY_UUID => '[UUID] ARPMPU',
            ],
            'valueCategoryInfos' => [
                MonthlyKpi::FIELD_PAYMENT => ValueCommentator::CATEGORY_CURRENCY_JPY,
                MonthlyKpi::FIELD_ARPMAU => ValueCommentator::CATEGORY_CURRENCY_JPY,
                MonthlyKpi::FIELD_ARPMPU => ValueCommentator::CATEGORY_CURRENCY_JPY,
                MonthlyKpi::FIELD_ARPMAU_BY_UUID => ValueCommentator::CATEGORY_CURRENCY_JPY,
                MonthlyKpi::FIELD_ARPMPU_BY_UUID => ValueCommentator::CATEGORY_CURRENCY_JPY,

                MonthlyKpi::FIELD_NRU => ValueCommentator::CATEGORY_COUNT_HUMAN,
                MonthlyKpi::FIELD_MAU => ValueCommentator::CATEGORY_COUNT_HUMAN,
                MonthlyKpi::FIELD_MPU => ValueCommentator::CATEGORY_COUNT_HUMAN,
                MonthlyKpi::FIELD_NRU_BY_UUID => ValueCommentator::CATEGORY_COUNT_HUMAN,
                MonthlyKpi::FIELD_MAU_BY_UUID => ValueCommentator::CATEGORY_COUNT_HUMAN,
                MonthlyKpi::FIELD_MPU_BY_UUID => ValueCommentator::CATEGORY_COUNT_HUMAN,
            ],
            'sortingFiledNames' => [
                MonthlyKpi::FIELD_DATE
            ],
            'filterCategoryInfos' => [
                'yyyymmddStart' => [
                    'label' => 'StartDate(>=)',
                    'category' => ValueCommentator::CATEGORY_YYYYMMDD_DATE_SIMPLE,
//                    'defaultValue' => TimeUtil::getYmdFromTime(max(strtotime('-9 month', NOW), $this->instanceFactory->kpiFactory()->getFirstTime())),
                ],
                'yyyymmddEnd' => [
                    'label' => 'EndDate(<=)',
                    'category' => ValueCommentator::CATEGORY_YYYYMMDD_DATE_SIMPLE,
//                    'defaultValue' => TimeUtil::getYmdFromTime(NOW),
                ],
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_DAILY_RETENTION_7] = [
            'modelClass' => $this->instanceFactory->dailyRetention7Model(),
            'bUseStringResourceId' => false,
            'jsClassName' => 'RpgADailyRetention7LookupTable',
            'bUseDownloadExcel' => true,
            'bAsc' => true,
            'initDisplayLength' => 10,
            'orderFieldInfo' => [
                'defaultTitle' => 'Order',
//                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => [
                DailyRetentionN::FIELD_DATE => 'Date',
                DailyRetentionN::FIELD_NRU => 'NRU',
                DailyRetentionN::getDayOtherFieldName(1) => '+1 Day',
                DailyRetentionN::getDayOtherFieldName(2) => '+2 Day',
                DailyRetentionN::getDayOtherFieldName(3) => '+3 Day',
                DailyRetentionN::getDayOtherFieldName(4) => '+4 Day',
                DailyRetentionN::getDayOtherFieldName(5) => '+5 Day',
                DailyRetentionN::getDayOtherFieldName(6) => '+6 Day',
                DailyRetentionN::getDayOtherFieldName(7) => '+7 Day',
            ],
            'valueCategoryInfos' => [
            ],
            'sortingFiledNames' => [
                DailyRetentionN::FIELD_DATE
            ],
            'filterCategoryInfos' => [
                'yyyymmddStart' => [
                    'label' => 'StartDate(>=)',
                    'category' => ValueCommentator::CATEGORY_YYYYMMDD_DATE_SIMPLE,
                    'defaultValue' => TimeUtil::getYmdFromTime(strtotime('-10 day', NOW)),
                ],
                'yyyymmddEnd' => [
                    'label' => 'EndDate(<=)',
                    'category' => ValueCommentator::CATEGORY_YYYYMMDD_DATE_SIMPLE,
                    'defaultValue' => TimeUtil::getYmdFromTime(strtotime('-1 day', NOW)),
                ],
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_DAILY_RETENTION_15] = [
            'modelClass' => $this->instanceFactory->dailyRetention15Model(),
            'bUseStringResourceId' => false,
            'jsClassName' => 'RpgADailyRetention15LookupTable',
            'bUseDownloadExcel' => true,
            'bAsc' => true,
            'initDisplayLength' => 25,
            'orderFieldInfo' => [
                'defaultTitle' => 'Order',
//                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => [
                DailyRetentionN::FIELD_DATE => 'Date',
                DailyRetentionN::FIELD_NRU => 'NRU',
                DailyRetentionN::getDayOtherFieldName(1) => '+1 Day',
                DailyRetentionN::getDayOtherFieldName(2) => '+2 Day',
                DailyRetentionN::getDayOtherFieldName(3) => '+3 Day',
                DailyRetentionN::getDayOtherFieldName(4) => '+4 Day',
                DailyRetentionN::getDayOtherFieldName(5) => '+5 Day',
                DailyRetentionN::getDayOtherFieldName(6) => '+6 Day',
                DailyRetentionN::getDayOtherFieldName(7) => '+7 Day',
                DailyRetentionN::getDayOtherFieldName(8) => '+8 Day',
                DailyRetentionN::getDayOtherFieldName(9) => '+9 Day',
                DailyRetentionN::getDayOtherFieldName(10) => '+10 Day',
                DailyRetentionN::getDayOtherFieldName(11) => '+11 Day',
                DailyRetentionN::getDayOtherFieldName(12) => '+12 Day',
                DailyRetentionN::getDayOtherFieldName(13) => '+13 Day',
                DailyRetentionN::getDayOtherFieldName(14) => '+14 Day',
                DailyRetentionN::getDayOtherFieldName(15) => '+15 Day',
            ],
            'valueCategoryInfos' => [
            ],
            'sortingFiledNames' => [
                DailyRetentionN::FIELD_DATE
            ],
            'filterCategoryInfos' => [
                'yyyymmddStart' => [
                    'label' => 'StartDate(>=)',
                    'category' => ValueCommentator::CATEGORY_YYYYMMDD_DATE_SIMPLE,
                    'defaultValue' => TimeUtil::getYmdFromTime(strtotime('-16 day', NOW)),
                ],
                'yyyymmddEnd' => [
                    'label' => 'EndDate(<=)',
                    'category' => ValueCommentator::CATEGORY_YYYYMMDD_DATE_SIMPLE,
                    'defaultValue' => TimeUtil::getYmdFromTime(strtotime('-1 day', NOW)),
                ],
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_DAILY_RETENTION_30] = [
            'modelClass' => $this->instanceFactory->dailyRetention30Model(),
            'bUseStringResourceId' => false,
            'jsClassName' => 'RpgADailyRetention30LookupTable',
            'bUseDownloadExcel' => true,
            'bAsc' => true,
            'initDisplayLength' => 50,
            'orderFieldInfo' => [
                'defaultTitle' => 'Order',
//                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => [
                DailyRetentionN::FIELD_DATE => 'Date',
                DailyRetentionN::FIELD_NRU => 'NRU',
                DailyRetentionN::getDayOtherFieldName(1) => '+1 Day',
                DailyRetentionN::getDayOtherFieldName(2) => '+2 Day',
                DailyRetentionN::getDayOtherFieldName(3) => '+3 Day',
                DailyRetentionN::getDayOtherFieldName(4) => '+4 Day',
                DailyRetentionN::getDayOtherFieldName(5) => '+5 Day',
                DailyRetentionN::getDayOtherFieldName(6) => '+6 Day',
                DailyRetentionN::getDayOtherFieldName(7) => '+7 Day',
                DailyRetentionN::getDayOtherFieldName(8) => '+8 Day',
                DailyRetentionN::getDayOtherFieldName(9) => '+9 Day',
                DailyRetentionN::getDayOtherFieldName(10) => '+10 Day',
                DailyRetentionN::getDayOtherFieldName(11) => '+11 Day',
                DailyRetentionN::getDayOtherFieldName(12) => '+12 Day',
                DailyRetentionN::getDayOtherFieldName(13) => '+13 Day',
                DailyRetentionN::getDayOtherFieldName(14) => '+14 Day',
                DailyRetentionN::getDayOtherFieldName(15) => '+15 Day',
                DailyRetentionN::getDayOtherFieldName(16) => '+16 Day',
                DailyRetentionN::getDayOtherFieldName(17) => '+17 Day',
                DailyRetentionN::getDayOtherFieldName(18) => '+18 Day',
                DailyRetentionN::getDayOtherFieldName(19) => '+19 Day',
                DailyRetentionN::getDayOtherFieldName(20) => '+20 Day',
                DailyRetentionN::getDayOtherFieldName(21) => '+21 Day',
                DailyRetentionN::getDayOtherFieldName(22) => '+22 Day',
                DailyRetentionN::getDayOtherFieldName(23) => '+23 Day',
                DailyRetentionN::getDayOtherFieldName(24) => '+24 Day',
                DailyRetentionN::getDayOtherFieldName(25) => '+25 Day',
                DailyRetentionN::getDayOtherFieldName(26) => '+26 Day',
                DailyRetentionN::getDayOtherFieldName(27) => '+27 Day',
                DailyRetentionN::getDayOtherFieldName(28) => '+28 Day',
                DailyRetentionN::getDayOtherFieldName(29) => '+29 Day',
                DailyRetentionN::getDayOtherFieldName(30) => '+30 Day',
            ],
            'valueCategoryInfos' => [
            ],
            'sortingFiledNames' => [
                DailyRetentionN::FIELD_DATE
            ],
            'filterCategoryInfos' => [
                'yyyymmddStart' => [
                    'label' => 'StartDate(>=)',
                    'category' => ValueCommentator::CATEGORY_YYYYMMDD_DATE_SIMPLE,
                    'defaultValue' => TimeUtil::getYmdFromTime(strtotime('-31 day', NOW)),
                ],
                'yyyymmddEnd' => [
                    'label' => 'EndDate(<=)',
                    'category' => ValueCommentator::CATEGORY_YYYYMMDD_DATE_SIMPLE,
                    'defaultValue' => TimeUtil::getYmdFromTime(strtotime('-1 day', NOW)),
                ],
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_DAILY_RETENTION_BY_UUID_7] = [
            'modelClass' => $this->instanceFactory->dailyRetentionByUuid7Model(),
            'bUseStringResourceId' => false,
            'jsClassName' => 'RpgADailyRetentionByUuid7LookupTable',
            'bUseDownloadExcel' => true,
            'bAsc' => true,
            'initDisplayLength' => 10,
            'orderFieldInfo' => [
                'defaultTitle' => 'Order',
//                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => [
                DailyRetentionN::FIELD_DATE => 'Date',
                DailyRetentionN::FIELD_NRU => 'NRU',
                DailyRetentionN::getDayOtherFieldName(1) => '+1 Day',
                DailyRetentionN::getDayOtherFieldName(2) => '+2 Day',
                DailyRetentionN::getDayOtherFieldName(3) => '+3 Day',
                DailyRetentionN::getDayOtherFieldName(4) => '+4 Day',
                DailyRetentionN::getDayOtherFieldName(5) => '+5 Day',
                DailyRetentionN::getDayOtherFieldName(6) => '+6 Day',
                DailyRetentionN::getDayOtherFieldName(7) => '+7 Day',
            ],
            'valueCategoryInfos' => [
            ],
            'sortingFiledNames' => [
                DailyRetentionN::FIELD_DATE
            ],
            'filterCategoryInfos' => [
                'yyyymmddStart' => [
                    'label' => 'StartDate(>=)',
                    'category' => ValueCommentator::CATEGORY_YYYYMMDD_DATE_SIMPLE,
                    'defaultValue' => TimeUtil::getYmdFromTime(strtotime('-8 day', NOW)),
                ],
                'yyyymmddEnd' => [
                    'label' => 'EndDate(<=)',
                    'category' => ValueCommentator::CATEGORY_YYYYMMDD_DATE_SIMPLE,
                    'defaultValue' => TimeUtil::getYmdFromTime(strtotime('-1 day', NOW)),
                ],
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_DAILY_RETENTION_BY_UUID_15] = [
            'modelClass' => $this->instanceFactory->dailyRetentionByUuid15Model(),
            'bUseStringResourceId' => false,
            'jsClassName' => 'RpgADailyRetentionByUuid15LookupTable',
            'bUseDownloadExcel' => true,
            'bAsc' => true,
            'initDisplayLength' => 25,
            'orderFieldInfo' => [
                'defaultTitle' => 'Order',
//                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => [
                DailyRetentionN::FIELD_DATE => 'Date',
                DailyRetentionN::FIELD_NRU => 'NRU',
                DailyRetentionN::getDayOtherFieldName(1) => '+1 Day',
                DailyRetentionN::getDayOtherFieldName(2) => '+2 Day',
                DailyRetentionN::getDayOtherFieldName(3) => '+3 Day',
                DailyRetentionN::getDayOtherFieldName(4) => '+4 Day',
                DailyRetentionN::getDayOtherFieldName(5) => '+5 Day',
                DailyRetentionN::getDayOtherFieldName(6) => '+6 Day',
                DailyRetentionN::getDayOtherFieldName(7) => '+7 Day',
                DailyRetentionN::getDayOtherFieldName(8) => '+8 Day',
                DailyRetentionN::getDayOtherFieldName(9) => '+9 Day',
                DailyRetentionN::getDayOtherFieldName(10) => '+10 Day',
                DailyRetentionN::getDayOtherFieldName(11) => '+11 Day',
                DailyRetentionN::getDayOtherFieldName(12) => '+12 Day',
                DailyRetentionN::getDayOtherFieldName(13) => '+13 Day',
                DailyRetentionN::getDayOtherFieldName(14) => '+14 Day',
                DailyRetentionN::getDayOtherFieldName(15) => '+15 Day',
            ],
            'valueCategoryInfos' => [
            ],
            'sortingFiledNames' => [
                DailyRetentionN::FIELD_DATE
            ],
            'filterCategoryInfos' => [
                'yyyymmddStart' => [
                    'label' => 'StartDate(>=)',
                    'category' => ValueCommentator::CATEGORY_YYYYMMDD_DATE_SIMPLE,
                    'defaultValue' => TimeUtil::getYmdFromTime(strtotime('-16 day', NOW)),
                ],
                'yyyymmddEnd' => [
                    'label' => 'EndDate(<=)',
                    'category' => ValueCommentator::CATEGORY_YYYYMMDD_DATE_SIMPLE,
                    'defaultValue' => TimeUtil::getYmdFromTime(strtotime('-1 day', NOW)),
                ],
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_DAILY_RETENTION_BY_UUID_30] = [
            'modelClass' => $this->instanceFactory->dailyRetentionByUuid30Model(),
            'bUseStringResourceId' => false,
            'jsClassName' => 'RpgADailyRetentionByUuid30LookupTable',
            'bUseDownloadExcel' => true,
            'bAsc' => true,
            'initDisplayLength' => 50,
            'orderFieldInfo' => [
                'defaultTitle' => 'Order',
//                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => [
                DailyRetentionN::FIELD_DATE => 'Date',
                DailyRetentionN::FIELD_NRU => 'NRU',
                DailyRetentionN::getDayOtherFieldName(1) => '+1 Day',
                DailyRetentionN::getDayOtherFieldName(2) => '+2 Day',
                DailyRetentionN::getDayOtherFieldName(3) => '+3 Day',
                DailyRetentionN::getDayOtherFieldName(4) => '+4 Day',
                DailyRetentionN::getDayOtherFieldName(5) => '+5 Day',
                DailyRetentionN::getDayOtherFieldName(6) => '+6 Day',
                DailyRetentionN::getDayOtherFieldName(7) => '+7 Day',
                DailyRetentionN::getDayOtherFieldName(8) => '+8 Day',
                DailyRetentionN::getDayOtherFieldName(9) => '+9 Day',
                DailyRetentionN::getDayOtherFieldName(10) => '+10 Day',
                DailyRetentionN::getDayOtherFieldName(11) => '+11 Day',
                DailyRetentionN::getDayOtherFieldName(12) => '+12 Day',
                DailyRetentionN::getDayOtherFieldName(13) => '+13 Day',
                DailyRetentionN::getDayOtherFieldName(14) => '+14 Day',
                DailyRetentionN::getDayOtherFieldName(15) => '+15 Day',
                DailyRetentionN::getDayOtherFieldName(16) => '+16 Day',
                DailyRetentionN::getDayOtherFieldName(17) => '+17 Day',
                DailyRetentionN::getDayOtherFieldName(18) => '+18 Day',
                DailyRetentionN::getDayOtherFieldName(19) => '+19 Day',
                DailyRetentionN::getDayOtherFieldName(20) => '+20 Day',
                DailyRetentionN::getDayOtherFieldName(21) => '+21 Day',
                DailyRetentionN::getDayOtherFieldName(22) => '+22 Day',
                DailyRetentionN::getDayOtherFieldName(23) => '+23 Day',
                DailyRetentionN::getDayOtherFieldName(24) => '+24 Day',
                DailyRetentionN::getDayOtherFieldName(25) => '+25 Day',
                DailyRetentionN::getDayOtherFieldName(26) => '+26 Day',
                DailyRetentionN::getDayOtherFieldName(27) => '+27 Day',
                DailyRetentionN::getDayOtherFieldName(28) => '+28 Day',
                DailyRetentionN::getDayOtherFieldName(29) => '+29 Day',
                DailyRetentionN::getDayOtherFieldName(30) => '+30 Day',
            ],
            'valueCategoryInfos' => [
            ],
            'sortingFiledNames' => [
                DailyRetentionN::FIELD_DATE
            ],
            'filterCategoryInfos' => [
                'yyyymmddStart' => [
                    'label' => 'StartDate(>=)',
                    'category' => ValueCommentator::CATEGORY_YYYYMMDD_DATE_SIMPLE,
                    'defaultValue' => TimeUtil::getYmdFromTime(strtotime('-31 day', NOW)),
                ],
                'yyyymmddEnd' => [
                    'label' => 'EndDate(<=)',
                    'category' => ValueCommentator::CATEGORY_YYYYMMDD_DATE_SIMPLE,
                    'defaultValue' => TimeUtil::getYmdFromTime(strtotime('-1 day', NOW)),
                ],
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_USER_PAYMENT_RANKING] = [
            'modelClass' => $this->instanceFactory->userPaymentRankingModel(),
            'bUseStringResourceId' => false,
            'jsClassName' => 'RpgAUserPaymentRankingLookupTable',
            'bUseDownloadExcel' => true,
            'bAsc' => true,
            'initDisplayLength' => 50,
//            'orderFieldInfo' => [
//                'defaultTitle' => 'Ranking',
//                'bUseSortField' => true,
//            ],
            'fieldDefaultTitles' => [
                UserPaymentRanking::FIELD_RANKING => 'Ranking',
                UserPaymentRanking::FIELD_PAYMENT => 'Payment',
                UserPaymentRanking::FIELD_ACCOUNT_ID => 'AccountId',
            ],
            'valueCategoryInfos' => [
                UserPaymentRanking::FIELD_PAYMENT => ValueCommentator::CATEGORY_CURRENCY_JPY,
            ],
            'sortingFiledNames' => [
                UserPaymentRanking::FIELD_RANKING
            ],
            'filterCategoryInfos' => [
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_USER_PAYMENT_RANKING_BY_UUID] = [
            'modelClass' => $this->instanceFactory->userPaymentRankingByUuidModel(),
            'bUseStringResourceId' => false,
            'jsClassName' => 'RpgAUserPaymentRankingByUuidLookupTable',
            'bUseDownloadExcel' => true,
            'bAsc' => true,
            'initDisplayLength' => 50,
//            'orderFieldInfo' => [
//                'defaultTitle' => 'Ranking',
//                'bUseSortField' => true,
//            ],
            'fieldDefaultTitles' => [
                UserPaymentRankingByUuid::FIELD_RANKING => 'Ranking',
                UserPaymentRankingByUuid::FIELD_PAYMENT => 'Payment',
                UserPaymentRankingByUuid::FIELD_UUID => 'UUID',
            ],
            'valueCategoryInfos' => [
                UserPaymentRankingByUuid::FIELD_PAYMENT => ValueCommentator::CATEGORY_CURRENCY_JPY,
            ],
            'sortingFiledNames' => [
                UserPaymentRankingByUuid::FIELD_RANKING
            ],
            'filterCategoryInfos' => [
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_DAILY_USER_GOODS] = [
            'modelClass' => $this->instanceFactory->dailyUserGoodsModel(),
            'bUseStringResourceId' => true,
            'jsClassName' => 'RpgADailyUserGoodsLookupTable',
            'bUseDownloadExcel' => true,
//            'bAsc' => true,
            'orderFieldInfo' => [
                'defaultTitle' => 'Order',
//                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => [
                DailyUserGoods::FIELD_DATE => 'Date',

                DailyUserGoods::FIELD_DAU_BY_UUID => 'DAU',

                DailyUserGoods::FIELD_CASH_INCOME => '대마석 획득',
                DailyUserGoods::FIELD_CASH_OUTGO => '대마석 소비',
                DailyUserGoods::FIELD_CASH_REMAIN => '대마석 잔여',
                DailyUserGoods::FIELD_CASH_INCOME_PER_UUID => 'DAU 당 대마석 획득',
                DailyUserGoods::FIELD_CASH_OUTGO_PER_UUID => 'DAU 당 대마석 소비',
                DailyUserGoods::FIELD_CASH_REMAIN_PER_UUID => 'DAU 당 대마석 잔여',

                DailyUserGoods::FIELD_GOLD_INCOME => '골드 획득',
                DailyUserGoods::FIELD_GOLD_OUTGO => '골드 소비',
                DailyUserGoods::FIELD_GOLD_REMAIN => '골드 잔여',
                DailyUserGoods::FIELD_GOLD_INCOME_PER_UUID => 'DAU 당 골드 획득',
                DailyUserGoods::FIELD_GOLD_OUTGO_PER_UUID => 'DAU 당 골드 소비',
                DailyUserGoods::FIELD_GOLD_REMAIN_PER_UUID => 'DAU 당 골드 잔여',
            ],
            'valueCategoryInfos' => [
                DailyUserGoods::FIELD_DAU_BY_UUID => ValueCommentator::CATEGORY_COUNT_HUMAN,
                DailyUserGoods::FIELD_CASH_INCOME => ValueCommentator::CATEGORY_COUNT_OBJECT,
                DailyUserGoods::FIELD_CASH_OUTGO => ValueCommentator::CATEGORY_COUNT_OBJECT,
                DailyUserGoods::FIELD_CASH_REMAIN => ValueCommentator::CATEGORY_COUNT_OBJECT,
                DailyUserGoods::FIELD_CASH_INCOME_PER_UUID => ValueCommentator::CATEGORY_COUNT_OBJECT_DECIMALS,
                DailyUserGoods::FIELD_CASH_OUTGO_PER_UUID => ValueCommentator::CATEGORY_COUNT_OBJECT_DECIMALS,
                DailyUserGoods::FIELD_CASH_REMAIN_PER_UUID => ValueCommentator::CATEGORY_COUNT_OBJECT_DECIMALS,

                DailyUserGoods::FIELD_GOLD_INCOME => ValueCommentator::CATEGORY_COUNT_GOLD,
                DailyUserGoods::FIELD_GOLD_OUTGO => ValueCommentator::CATEGORY_COUNT_GOLD,
                DailyUserGoods::FIELD_GOLD_REMAIN => ValueCommentator::CATEGORY_COUNT_GOLD,
                DailyUserGoods::FIELD_GOLD_INCOME_PER_UUID => ValueCommentator::CATEGORY_COUNT_GOLD,
                DailyUserGoods::FIELD_GOLD_OUTGO_PER_UUID => ValueCommentator::CATEGORY_COUNT_GOLD,
                DailyUserGoods::FIELD_GOLD_REMAIN_PER_UUID => ValueCommentator::CATEGORY_COUNT_GOLD,
            ],
            'sortingFiledNames' => [
                DailyUserGoods::FIELD_DATE
            ],
            'filterCategoryInfos' => [
//                'yyyymmddStart' => [
//                    'label' => 'StartDate(>=)',
//                    'category' => ValueCommentator::CATEGORY_YYYYMMDD_DATE_SIMPLE,
//                    'defaultValue' => TimeUtil::getYmdFromTime(max(strtotime('-9 day', NOW), $this->instanceFactory->kpiFactory()->getFirstTime())),
//                ],
//                'yyyymmddEnd' => [
//                    'label' => 'EndDate(<=)',
//                    'category' => ValueCommentator::CATEGORY_YYYYMMDD_DATE_SIMPLE,
//                    'defaultValue' => TimeUtil::getYmdFromTime(NOW),
//                ],
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_DAILY_USER_GOODS23] = [
            'modelClass' => $this->instanceFactory->dailyUserGoods23Model(),
            'bUseStringResourceId' => true,
            'jsClassName' => 'RpgADailyUserGoods23LookupTable',
            'bUseDownloadExcel' => true,
//            'bAsc' => true,
            'orderFieldInfo' => [
                'defaultTitle' => 'Order',
//                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => [
                DailyUserGoods23::FIELD_DATE => 'Date',

                DailyUserGoods23::FIELD_DAU_BY_UUID => 'DAU',

                DailyUserGoods23::FIELD_SUPPORTER_POINT_INCOME => '마일리지 포인트 획득',
                DailyUserGoods23::FIELD_SUPPORTER_POINT_OUTGO => '마일리지 포인트 소비',
                DailyUserGoods23::FIELD_SUPPORTER_POINT_REMAIN => '마일리지 포인트 잔여',
                DailyUserGoods23::FIELD_SUPPORTER_POINT_INCOME_PER_UUID => 'DAU 당 마일리지 포인트 획득',
                DailyUserGoods23::FIELD_SUPPORTER_POINT_OUTGO_PER_UUID => 'DAU 당 마일리지 포인트 소비',
                DailyUserGoods23::FIELD_SUPPORTER_POINT_REMAIN_PER_UUID => 'DAU 당 마일리지 포인트 잔여',

                DailyUserGoods23::FIELD_ROOM_POINT_INCOME => '룸 포인트 획득',
                DailyUserGoods23::FIELD_ROOM_POINT_OUTGO => '룸 포인트 소비',
                DailyUserGoods23::FIELD_ROOM_POINT_REMAIN => '룸 포인트 잔여',
                DailyUserGoods23::FIELD_ROOM_POINT_INCOME_PER_UUID => 'DAU 당 룸 포인트 획득',
                DailyUserGoods23::FIELD_ROOM_POINT_OUTGO_PER_UUID => 'DAU 당 룸 포인트 소비',
                DailyUserGoods23::FIELD_ROOM_POINT_REMAIN_PER_UUID => 'DAU 당 룸 포인트 잔여',
            ],
            'valueCategoryInfos' => [
                DailyUserGoods23::FIELD_DAU_BY_UUID => ValueCommentator::CATEGORY_COUNT_HUMAN,
                DailyUserGoods23::FIELD_SUPPORTER_POINT_INCOME => ValueCommentator::CATEGORY_COUNT_NUMBER,
                DailyUserGoods23::FIELD_SUPPORTER_POINT_OUTGO => ValueCommentator::CATEGORY_COUNT_NUMBER,
                DailyUserGoods23::FIELD_SUPPORTER_POINT_REMAIN => ValueCommentator::CATEGORY_COUNT_NUMBER,
                DailyUserGoods23::FIELD_SUPPORTER_POINT_INCOME_PER_UUID => ValueCommentator::CATEGORY_COUNT_NUMBER_DECIMALS,
                DailyUserGoods23::FIELD_SUPPORTER_POINT_OUTGO_PER_UUID => ValueCommentator::CATEGORY_COUNT_NUMBER_DECIMALS,
                DailyUserGoods23::FIELD_SUPPORTER_POINT_REMAIN_PER_UUID => ValueCommentator::CATEGORY_COUNT_NUMBER_DECIMALS,

                DailyUserGoods23::FIELD_ROOM_POINT_INCOME => ValueCommentator::CATEGORY_COUNT_NUMBER,
                DailyUserGoods23::FIELD_ROOM_POINT_OUTGO => ValueCommentator::CATEGORY_COUNT_NUMBER,
                DailyUserGoods23::FIELD_ROOM_POINT_REMAIN => ValueCommentator::CATEGORY_COUNT_NUMBER,
                DailyUserGoods23::FIELD_ROOM_POINT_INCOME_PER_UUID => ValueCommentator::CATEGORY_COUNT_NUMBER_DECIMALS,
                DailyUserGoods23::FIELD_ROOM_POINT_OUTGO_PER_UUID => ValueCommentator::CATEGORY_COUNT_NUMBER_DECIMALS,
                DailyUserGoods23::FIELD_ROOM_POINT_REMAIN_PER_UUID => ValueCommentator::CATEGORY_COUNT_NUMBER_DECIMALS,
            ],
            'sortingFiledNames' => [
                DailyUserGoods23::FIELD_DATE
            ],
            'filterCategoryInfos' => [
//                'yyyymmddStart' => [
//                    'label' => 'StartDate(>=)',
//                    'category' => ValueCommentator::CATEGORY_YYYYMMDD_DATE_SIMPLE,
//                    'defaultValue' => TimeUtil::getYmdFromTime(max(strtotime('-9 day', NOW), $this->instanceFactory->kpiFactory()->getFirstTime())),
//                ],
//                'yyyymmddEnd' => [
//                    'label' => 'EndDate(<=)',
//                    'category' => ValueCommentator::CATEGORY_YYYYMMDD_DATE_SIMPLE,
//                    'defaultValue' => TimeUtil::getYmdFromTime(NOW),
//                ],
            ],
        ];


        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_DAILY_USER_GOODS45] = [
            'modelClass' => $this->instanceFactory->dailyUserGoods45Model(),
            'bUseStringResourceId' => true,
            'jsClassName' => 'RpgADailyUserGoods45LookupTable',
            'bUseDownloadExcel' => true,
//            'bAsc' => true,
            'orderFieldInfo' => [
                'defaultTitle' => 'Order',
//                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => [
                DailyUserGoods45::FIELD_DATE => 'Date',

                DailyUserGoods45::FIELD_DAU_BY_UUID => 'DAU',

                DailyUserGoods45::FIELD_AP_INCOME => 'AP 획득',
                DailyUserGoods45::FIELD_AP_OUTGO => 'AP 소비',
                DailyUserGoods45::FIELD_AP_REMAIN => 'AP 잔여',
                DailyUserGoods45::FIELD_AP_INCOME_PER_UUID => 'DAU 당 AP 획득',
                DailyUserGoods45::FIELD_AP_OUTGO_PER_UUID => 'DAU 당 AP 소비',
                DailyUserGoods45::FIELD_AP_REMAIN_PER_UUID => 'DAU 당 AP 잔여',

                DailyUserGoods45::FIELD_BP_INCOME => 'BP 획득',
                DailyUserGoods45::FIELD_BP_OUTGO => 'BP 소비',
                DailyUserGoods45::FIELD_BP_REMAIN => 'BP 잔여',
                DailyUserGoods45::FIELD_BP_INCOME_PER_UUID => 'DAU 당 BP 획득',
                DailyUserGoods45::FIELD_BP_OUTGO_PER_UUID => 'DAU 당 BP 소비',
                DailyUserGoods45::FIELD_BP_REMAIN_PER_UUID => 'DAU 당 BP 잔여',
            ],
            'valueCategoryInfos' => [
                DailyUserGoods45::FIELD_DAU_BY_UUID => ValueCommentator::CATEGORY_COUNT_HUMAN,
                DailyUserGoods45::FIELD_AP_INCOME => ValueCommentator::CATEGORY_COUNT_NUMBER,
                DailyUserGoods45::FIELD_AP_OUTGO => ValueCommentator::CATEGORY_COUNT_NUMBER,
                DailyUserGoods45::FIELD_AP_REMAIN => ValueCommentator::CATEGORY_COUNT_NUMBER,
                DailyUserGoods45::FIELD_AP_INCOME_PER_UUID => ValueCommentator::CATEGORY_COUNT_NUMBER_DECIMALS,
                DailyUserGoods45::FIELD_AP_OUTGO_PER_UUID => ValueCommentator::CATEGORY_COUNT_NUMBER_DECIMALS,
                DailyUserGoods45::FIELD_AP_REMAIN_PER_UUID => ValueCommentator::CATEGORY_COUNT_NUMBER_DECIMALS,

                DailyUserGoods45::FIELD_BP_INCOME => ValueCommentator::CATEGORY_COUNT_NUMBER,
                DailyUserGoods45::FIELD_BP_OUTGO => ValueCommentator::CATEGORY_COUNT_NUMBER,
                DailyUserGoods45::FIELD_BP_REMAIN => ValueCommentator::CATEGORY_COUNT_NUMBER,
                DailyUserGoods45::FIELD_BP_INCOME_PER_UUID => ValueCommentator::CATEGORY_COUNT_NUMBER_DECIMALS,
                DailyUserGoods45::FIELD_BP_OUTGO_PER_UUID => ValueCommentator::CATEGORY_COUNT_NUMBER_DECIMALS,
                DailyUserGoods45::FIELD_BP_REMAIN_PER_UUID => ValueCommentator::CATEGORY_COUNT_NUMBER_DECIMALS,
            ],
            'sortingFiledNames' => [
                DailyUserGoods45::FIELD_DATE
            ],
            'filterCategoryInfos' => [
//                'yyyymmddStart' => [
//                    'label' => 'StartDate(>=)',
//                    'category' => ValueCommentator::CATEGORY_YYYYMMDD_DATE_SIMPLE,
//                    'defaultValue' => TimeUtil::getYmdFromTime(max(strtotime('-9 day', NOW), $this->instanceFactory->kpiFactory()->getFirstTime())),
//                ],
//                'yyyymmddEnd' => [
//                    'label' => 'EndDate(<=)',
//                    'category' => ValueCommentator::CATEGORY_YYYYMMDD_DATE_SIMPLE,
//                    'defaultValue' => TimeUtil::getYmdFromTime(NOW),
//                ],
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_DAILY_USER_GOODS6] = [
            'modelClass' => $this->instanceFactory->dailyUserGoods6Model(),
            'bUseStringResourceId' => true,
            'jsClassName' => 'RpgADailyUserGoods6LookupTable',
            'bUseDownloadExcel' => true,
//            'bAsc' => true,
            'orderFieldInfo' => [
                'defaultTitle' => 'Order',
//                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => [
                DailyUserGoods6::FIELD_DATE => 'Date',

                DailyUserGoods6::FIELD_DAU_BY_UUID => 'DAU',

                DailyUserGoods6::FIELD_BATTLE_COIN_INCOME => '배틀 코인 획득',
                DailyUserGoods6::FIELD_BATTLE_COIN_OUTGO => '배틀 코인 소비',
                DailyUserGoods6::FIELD_BATTLE_COIN_REMAIN => '배틀 코인 잔여',
                DailyUserGoods6::FIELD_BATTLE_COIN_INCOME_PER_UUID => 'DAU 당 배틀 코인 획득',
                DailyUserGoods6::FIELD_BATTLE_COIN_OUTGO_PER_UUID => 'DAU 당 배틀 코인 소비',
                DailyUserGoods6::FIELD_BATTLE_COIN_REMAIN_PER_UUID => 'DAU 당 배틀 코인 잔여',
            ],
            'valueCategoryInfos' => [
                DailyUserGoods6::FIELD_DAU_BY_UUID => ValueCommentator::CATEGORY_COUNT_HUMAN,
                DailyUserGoods6::FIELD_BATTLE_COIN_INCOME => ValueCommentator::CATEGORY_COUNT_NUMBER,
                DailyUserGoods6::FIELD_BATTLE_COIN_OUTGO => ValueCommentator::CATEGORY_COUNT_NUMBER,
                DailyUserGoods6::FIELD_BATTLE_COIN_REMAIN => ValueCommentator::CATEGORY_COUNT_NUMBER,
                DailyUserGoods6::FIELD_BATTLE_COIN_INCOME_PER_UUID => ValueCommentator::CATEGORY_COUNT_NUMBER_DECIMALS,
                DailyUserGoods6::FIELD_BATTLE_COIN_OUTGO_PER_UUID => ValueCommentator::CATEGORY_COUNT_NUMBER_DECIMALS,
                DailyUserGoods6::FIELD_BATTLE_COIN_REMAIN_PER_UUID => ValueCommentator::CATEGORY_COUNT_NUMBER_DECIMALS,
            ],
            'sortingFiledNames' => [
                DailyUserGoods6::FIELD_DATE
            ],
            'filterCategoryInfos' => [
//                'yyyymmddStart' => [
//                    'label' => 'StartDate(>=)',
//                    'category' => ValueCommentator::CATEGORY_YYYYMMDD_DATE_SIMPLE,
//                    'defaultValue' => TimeUtil::getYmdFromTime(max(strtotime('-9 day', NOW), $this->instanceFactory->kpiFactory()->getFirstTime())),
//                ],
//                'yyyymmddEnd' => [
//                    'label' => 'EndDate(<=)',
//                    'category' => ValueCommentator::CATEGORY_YYYYMMDD_DATE_SIMPLE,
//                    'defaultValue' => TimeUtil::getYmdFromTime(NOW),
//                ],
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_USER_CASH_RANKING] = [
            'modelClass' => $this->instanceFactory->userCashRankingModel(),
            'bUseStringResourceId' => true,
            'jsClassName' => 'RpgAUserCashRankingLookupTable',
            'bUseDownloadExcel' => true,
            'bAsc' => true,
            'fieldDefaultTitles' => [
                UserCashRanking::FIELD_RANKING => '순위',
                UserCashRanking::FIELD_CASH => '대마석 보유량',
                UserCashRanking::FIELD_UUID => '랭커 UUID',
            ],
            'valueCategoryInfos' => [
                UserCashRanking::FIELD_CASH => ValueCommentator::CATEGORY_COUNT_OBJECT,
            ],
            'sortingFiledNames' => [
                UserCashRanking::FIELD_RANKING
            ],
            'filterCategoryInfos' => [
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_USER_GOLD_RANKING] = [
            'modelClass' => $this->instanceFactory->userGoldRankingModel(),
            'bUseStringResourceId' => true,
            'jsClassName' => 'RpgAUserGoldRankingLookupTable',
            'bUseDownloadExcel' => true,
            'bAsc' => true,
            'fieldDefaultTitles' => [
                UserGoldRanking::FIELD_RANKING => '순위',
                UserGoldRanking::FIELD_GOLD => '골드 보유량',
                UserGoldRanking::FIELD_UUID => '랭커 UUID',
            ],
            'valueCategoryInfos' => [
                UserGoldRanking::FIELD_GOLD => ValueCommentator::CATEGORY_COUNT_GOLD,
            ],
            'sortingFiledNames' => [
                UserGoldRanking::FIELD_RANKING
            ],
            'filterCategoryInfos' => [
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_USER_SUPPORTER_POINT_RANKING] = [
            'modelClass' => $this->instanceFactory->userSupporterPointRankingModel(),
            'bUseStringResourceId' => true,
            'jsClassName' => 'RpgAUserSupporterPointRankingLookupTable',
            'bUseDownloadExcel' => true,
            'bAsc' => true,
            'fieldDefaultTitles' => [
                UserSupporterPointRanking::FIELD_RANKING => '순위',
                UserSupporterPointRanking::FIELD_SUPPORTER_POINT => '마일리지 포인트 보유량',
                UserSupporterPointRanking::FIELD_UUID => '랭커 UUID',
            ],
            'valueCategoryInfos' => [
                UserSupporterPointRanking::FIELD_SUPPORTER_POINT => ValueCommentator::CATEGORY_COUNT_NUMBER,
            ],
            'sortingFiledNames' => [
                UserSupporterPointRanking::FIELD_RANKING
            ],
            'filterCategoryInfos' => [
            ],
        ];

        $this->configArr[SmartLookupTableConfig::CONFIG_NAME_ATSPDAU] = [
            'modelClass' => $this->instanceFactory->atspdauModel(),
            'bUseStringResourceId' => false,
            'jsClassName' => 'RpgAUserAtspdauLookupTable',
            'bUseDownloadExcel' => true,
            'bAsc' => false,
            'initDisplayLength' => 10,
//            'orderFieldInfo' => [
//                'defaultTitle' => 'Ranking',
//                'bUseSortField' => true,
//            ],
            'fieldDefaultTitles' => [
                Atspdau::FIELD_DATE => '일자',
                Atspdau::FIELD_DRV_BY_UUID => '[UUID] 재방문자 수',
                Atspdau::FIELD_ATSPDRV_BY_UUID => '[UUID] 재방문자 평균 플레이 시간',
                Atspdau::FIELD_DAU_BY_UUID => '[UUID] DAU',
                Atspdau::FIELD_ATSPDAU_BY_UUID => '[UUID] DAU 평균 플레이 시간',
                Atspdau::FIELD_DRV => '[Account ID] 재방문자 수',
                Atspdau::FIELD_ATSPDRV => '[Account ID] 재방문자 평균 플레이 시간',
                Atspdau::FIELD_DAU => '[Account ID] DAU',
                Atspdau::FIELD_ATSPDAU => '[Account ID] DAU 평균 플레이 시간',
            ],
            'valueCategoryInfos' => [
                Atspdau::FIELD_DRV_BY_UUID => ValueCommentator::CATEGORY_COUNT_HUMAN,
                Atspdau::FIELD_DAU_BY_UUID => ValueCommentator::CATEGORY_COUNT_HUMAN,
                Atspdau::FIELD_DRV => ValueCommentator::CATEGORY_COUNT_HUMAN,
                Atspdau::FIELD_DAU => ValueCommentator::CATEGORY_COUNT_HUMAN,

                Atspdau::FIELD_ATSPDRV_BY_UUID => ValueCommentator::CATEGORY_PERIODSECONDS,
                Atspdau::FIELD_ATSPDAU_BY_UUID => ValueCommentator::CATEGORY_PERIODSECONDS,
                Atspdau::FIELD_ATSPDRV => ValueCommentator::CATEGORY_PERIODSECONDS,
                Atspdau::FIELD_ATSPDAU => ValueCommentator::CATEGORY_PERIODSECONDS,
            ],
            'sortingFiledNames' => [
                Atspdau::FIELD_DATE
            ],
            'filterCategoryInfos' => [
                'yyyymmddStart' => [
                    'label' => 'StartDate(>=)',
                    'category' => ValueCommentator::CATEGORY_YYYYMMDD_DATE_SIMPLE,
                ],
                'yyyymmddEnd' => [
                    'label' => 'EndDate(<=)',
                    'category' => ValueCommentator::CATEGORY_YYYYMMDD_DATE_SIMPLE,
                    'defaultValue' => TimeUtil::getYmdFromTime(strtotime('-1 day', NOW)),
                ],
            ],
        ];

        $this->configArr[self::CONFIG_NAME_RANK_PLAY_SECONDS] = [
            'modelClass' => $this->instanceFactory->rankPlaySecondsModel(),
            'bUseStringResourceId' => true,
            'jsClassName' => 'RpgARankPlaySeconds',
            'bUseDownloadExcel' => true,
            'bAsc' => false,
            'initDisplayLength' => 10,
            'fieldDefaultTitles' => [
                RankPlaySeconds::FIELD_RANK => '지휘관 랭크',
                RankPlaySeconds::FIELD_COUNT => 'UUID 수',
                RankPlaySeconds::FIELD_PLAY_SECONDS_AVG => '평균 누적 플레이 시간',
                RankPlaySeconds::FIELD_PLAY_SECONDS => '누적 플레이 시간',
            ],
            'valueCategoryInfos' => [
                RankPlaySeconds::FIELD_COUNT => ValueCommentator::CATEGORY_COUNT_HUMAN,
                RankPlaySeconds::FIELD_PLAY_SECONDS => ValueCommentator::CATEGORY_PERIODSECONDS,
                RankPlaySeconds::FIELD_PLAY_SECONDS_AVG => ValueCommentator::CATEGORY_PERIODSECONDS,
            ],
            'sortingFiledNames' => [
                RankPlaySeconds::FIELD_PLAY_SECONDS_AVG
            ],
            'filterCategoryInfos' => [
                'yyyymmddStart' => [
                    'label' => 'StartDate(>=)',
                    'category' => ValueCommentator::CATEGORY_YYYYMMDD_DATE_SIMPLE,
                ],
                'yyyymmddEnd' => [
                    'label' => 'EndDate(<=)',
                    'category' => ValueCommentator::CATEGORY_YYYYMMDD_DATE_SIMPLE,
                    'defaultValue' => TimeUtil::getYmdFromTime(strtotime('-1 day', NOW)),
                ],
            ],

        ];


        $this->configArr[self::CONFIG_NAME_TOTAL_USER_DATA_LOOKUP] = [
            'modelClass' => $this->instanceFactory->accountAuthDBModel(),
            'bUseStringResourceId' => true,
            'jsClassName' => 'RpgATotalUserDataLookupTable',
            'orderFieldInfo' => [            // orderFieldInfo 속성이 있을 경우 자동으로 맨 앞에 순서 필드가 포함된다
                'defaultTitle' => '순서',
                'bUseSortField' => true,
            ],
            'fieldDefaultTitles' => [
                AccountAuthDB::FIELD_FUUID => 'UUID(정렬 부하 있음)',
                AccountAuthDB::FIELD_FACCOUNTID => 'Account ID',
                AccountAuthDB::FIELD_FGAMEDBID => 'Game DB ID(정렬 부하 있음)',
                AccountAuthDB::FIELD_FDUMMYFLAG => '더미 유저 플래그',
                AccountAuthDB::FIELD_FKEEPONID => '이어하기 코드',
                AccountAuthDB::FIELD_FKEEPONPW => '이어하기 비번 Hash',
                AccountAuthDB::FIELD_FINSERTTIME => '생성 일시(정렬 부하 있음)',
            ],
            'valueCategoryInfos' => [
                AccountAuthDB::FIELD_FINSERTTIME => ValueCommentator::CATEGORY_DATE_TIME_STR,
                AccountAuthDB::FIELD_FKEEPONPW => ValueCommentator::CATEGORY_MD5,
            ],
            'sortingFiledNames' => [        // 정렬용으로 사용 가능한 필드명 배열
                AccountAuthDB::FIELD_FUUID,
                AccountAuthDB::FIELD_FGAMEDBID ,
                AccountAuthDB::FIELD_FINSERTTIME,
            ],
            'filterCategoryInfos' => [
                AccountAuthDB::FIELD_FUUID => [
                    'label' => 'UUID',
                    //'category' => ValueCommentator::CATEGORY_,
                ],
                AccountAuthDB::FIELD_FKEEPONID => [
                    'label' => 'Keep on ID',
                    //'category' => ValueCommentator::CATEGORY_,
                ],
            ],
        ];

        $this->configArr[self::CONFIG_NAME_USER_DATA_LOOKUP] = [
            'modelClass' => $this->instanceFactory->userDataModel(),
            'bUseStringResourceId' => true,
            'jsClassName' => 'RpgAUserDataLookupTable',
            'basketInfo' => [
                'optionValues' => UserDbNodeIndex::getSelectOptions(),
            ],
            'orderFieldInfo' => array(            // orderFieldInfo 속성이 있을 경우 자동으로 맨 앞에 순서 필드가 포함된다
                'defaultTitle' => '순서',
            ),
            'fieldDefaultTitles' => array(
                UserData::FIELD_FUUID => 'UUID(정렬 부하 없음)',
                UserData::FIELD_FNICKNAME => '닉네임(정렬시 부하 있음)',
                UserData::FIELD_FLV => '레벨(정렬 부하)',
                UserData::FIELD_FEXP => '경험치(정렬 부하)',
                UserData::FIELD_FDELETETIME => '탈퇴 시각(정렬 부하)',
                UserData::FIELD_FMAKETIME => '생성 일시(정렬 부하)',
            ),
            'valueCategoryInfos' => [
                UserData::FIELD_FDELETETIME => ValueCommentator::CATEGORY_DATE_TIME_STR,
                UserData::FIELD_FMAKETIME => ValueCommentator::CATEGORY_DATE_TIME_STR,
            ],
            'sortingFiledNames' => [        // 정렬용으로 사용 가능한 필드명 배열
                UserData::FIELD_FUUID,
                UserData::FIELD_FNICKNAME,
                UserData::FIELD_FLV,
                UserData::FIELD_FEXP,
                UserData::FIELD_FDELETETIME,
                UserData::FIELD_FMAKETIME,
            ],
        ];

        $this->configArr[self::CONFIG_NAME_USER_IN_APP_PAYMENT_LOOKUP] = [
            'modelClass' => $this->instanceFactory->userInAppPaymentModel(),
            'bUseStringResourceId' => true,
            'bUseDownloadExcel' => true,
            'jsClassName' => 'RpgAUserInAppPaymentLookupTable',
            'basketInfo' => [
                'optionValues' => UserDbNodeIndex::getSelectOptions(),
            ],
            'orderFieldInfo' => array(            // orderFieldInfo 속성이 있을 경우 자동으로 맨 앞에 순서 필드가 포함된다
//                'defaultTitle' => '순서',
            ),
            'fieldDefaultTitles' => array(
                UserInAppPayment::FIELD_ID => 'ID',
                UserInAppPayment::FIELD_FUUID => 'fUUID',
                UserInAppPayment::FIELD_FACCOUNTID => 'fAccountID',
                UserInAppPayment::FIELD_FSTOREID => 'fStoreID',
                UserInAppPayment::FIELD_FPAY => 'fPay',
                UserInAppPayment::FIELD_FINAPPKIND => 'fInAppKind',
                UserInAppPayment::FIELD_FCHECKID => 'fCheckID',
                UserInAppPayment::FIELD_FPURCHASETOKEN => 'fPurchaseToken',
                UserInAppPayment::FIELD_FREFUND => 'fRefund',
                UserInAppPayment::FIELD_FINSERTTIME => 'fInsertTime',
                UserInAppPayment::OTHER_FIELD_REFUND_DATE => 'Refund date',
            ),
            'valueCategoryInfos' => [
                UserInAppPayment::FIELD_FPAY => ValueCommentator::CATEGORY_CURRENCY_JPY,
                UserInAppPayment::FIELD_FINAPPKIND => ValueCommentator::CATEGORY_IN_APP_KIND,
                UserInAppPayment::FIELD_FSTOREID => ValueCommentator::CATEGORY_STORE_ID_ONLY_MONEY,
                UserInAppPayment::FIELD_FINSERTTIME => ValueCommentator::CATEGORY_DATE_TIME_STR,
                UserInAppPayment::FIELD_FPURCHASETOKEN => ValueCommentator::CATEGORY_GOOGLE_PURCHASE_TOKEN,
                UserInAppPayment::FIELD_FREFUND => ValueCommentator::CATEGORY_REFUND_FLAG,
            ],
            'sortingFiledNames' => [        // 정렬용으로 사용 가능한 필드명 배열
                UserInAppPayment::FIELD_ID,
            ],
            'filterCategoryInfos' => [
                UserInAppPayment::FIELD_FUUID => [
                    'label' => 'UUID',
                    //'category' => ValueCommentator::CATEGORY_,
                ],
                UserInAppPayment::FIELD_FACCOUNTID => [
                    'label' => 'Account ID',
                    //'category' => ValueCommentator::CATEGORY_,
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserInAppPayment::FIELD_FCHECKID, GameLogModel::OPERATOR_TEXT_LIKE) => [
                    'label' => '영수증(領収書) ID',
                ],
                GameLogModel::createFilterKeyNameWithOperator(UserInAppPayment::FIELD_FREFUND, GameLogModel::OPERATOR_TEXT_GE) => [
                    'label' => '환불 상태',
                    'category' => ValueCommentator::CATEGORY_BOOLEAN_USER_IN_APP_PAYMENT_REFUNDED,
                ],
                UserInAppPayment::FIELD_FREFUND => [
                    'label' => '환불 상세',
                    'category' => ValueCommentator::CATEGORY_BOOLEAN_USER_IN_APP_PAYMENT_REFUND_FLAG,
                ],
                UserInAppPayment::FIELD_FINAPPKIND => [
                    'label' => 'Market',
                    'category' => ValueCommentator::CATEGORY_IN_APP_KIND_ALL,
                ],
            ],
        ];

        $this->configArr[self::CONFIG_NAME_USER_PUSH_NOTI_LOOKUP] = [
            'modelClass' => $this->instanceFactory->userPushNotiModel(),
            'bUseStringResourceId' => false,
            'jsClassName' => 'RpgAUserPushNotiLookupTable',
            'basketInfo' => [
                'optionValues' => UserDbNodeIndex::getSelectOptions(),
            ],
            'orderFieldInfo' => array(            // orderFieldInfo 속성이 있을 경우 자동으로 맨 앞에 순서 필드가 포함된다
                'defaultTitle' => 'Order',
            ),
            'fieldDefaultTitles' => array(
                UserPushNoti::FIELD_FUUID => 'UUID',
                'userSummary' => 'User Summary',
                UserPushNoti::FIELD_FFCMTOKEN => 'FCM Token',
            ),
            'valueCategoryInfos' => [
            ],
            'sortingFiledNames' => [        // 정렬용으로 사용 가능한 필드명 배열
                UserPushNoti::FIELD_FUUID,
            ],
            'filterCategoryInfos' => [
                'token' => [
                    'label' => 'Token',
                    'category' => ValueCommentator::CATEGORY_BOOLEAN_EXIST_ALL,
                ],
            ],
        ];

        $this->configArr[self::CONFIG_NAME_USER_MAIL_LOOKUP] = [
            'modelClass' => $this->instanceFactory->userMailModel(),
            'bUseStringResourceId' => true,
            'jsClassName' => 'RpgAUserMailLookupTable',
            'basketInfo' => [
                'optionValues' => UserDbNodeIndex::getSelectOptions(),
            ],
            'orderFieldInfo' => array(            // orderFieldInfo 속성이 있을 경우 자동으로 맨 앞에 순서 필드가 포함된다
                'defaultTitle' => '순서',
                'bUseSortField' => true,
            ),
            'fieldDefaultTitles' => array(
                UserMail::FIELD_FUUID => 'UUID (정렬 부하 있음)',
                UserMail::FIELD_FMAILUID => 'MailUID (정렬 부하 있음)',
                UserMail::FIELD_FENDTIME => '유효 기간(정렬 부하 있음)',
                UserMail::FIELD_FPRODUCTTYPE => UserMail::FIELD_FPRODUCTTYPE,
                UserMail::FIELD_FPRODUCTINDEX => UserMail::FIELD_FPRODUCTINDEX,
                UserMail::FIELD_FPRODUCTVALUE => UserMail::FIELD_FPRODUCTVALUE,
                UserMail::FIELD_FMAILTYPEID => UserMail::FIELD_FMAILTYPEID,
                UserMail::FIELD_FMAILTYPEVALUE => UserMail::FIELD_FMAILTYPEVALUE,
                UserMail::FIELD_FDELFLAG => UserMail::FIELD_FDELFLAG,
                UserMail::FIELD_FDELTIME => UserMail::FIELD_FDELTIME,
            ),
            'valueCategoryInfos' => [
                UserMail::FIELD_FENDTIME => ValueCommentator::CATEGORY_BIG_INT_DATE,
                UserMail::FIELD_FPRODUCTTYPE => ValueCommentator::CATEGORY_REWARD_TYPE,
                UserMail::FIELD_FDELFLAG => ValueCommentator::CATEGORY_BOOLEAN_DELETED,
                UserMail::FIELD_FDELTIME => ValueCommentator::CATEGORY_DATE_TIME_STR,
                UserMail::FIELD_FMAILTYPEID => ValueCommentator::CATEGORY_MAIL_TYPE,
            ],
            'sortingFiledNames' => [        // 정렬용으로 사용 가능한 필드명 배열
                UserMail::FIELD_FUUID,
                UserMail::FIELD_FMAILUID,
                UserMail::FIELD_FENDTIME,
            ],
            'dependCategoryInfos' => RewardType::getDependCategoryInfos(UserMail::FIELD_FPRODUCTTYPE, UserMail::FIELD_FPRODUCTINDEX),
            'filterCategoryInfos' => [
                UserMail::FIELD_FPRODUCTTYPE => [
                    'label' => 'ProductType',
                    'category' => ValueCommentator::CATEGORY_REWARD_TYPE_ALL,
                ],
                UserMail::FIELD_FMAILTYPEID => [
                    'label' => 'MailType',
                    'category' => ValueCommentator::CATEGORY_MAIL_TYPE_ALL,
                ],
                UserMail::FIELD_FDELFLAG => [
                    'label' => 'DelFlag',
                    'category' => ValueCommentator::CATEGORY_BOOLEAN_DELETED_ALL,
                ],
            ],
        ];

        // 자동으로 처리해주는 속성 관련 처리. 시작
        foreach ($this->configArr as $smartLookupTableConfigName => &$sltConfig) {

            $modelClass = $sltConfig['modelClass'];
            $bUseStringResourceId = $sltConfig['bUseStringResourceId'] ?? false;


            // 모델 명 구하기
            $sltConfig['modelName'] = $modelClass->modelName();


            // MsSqlModel 인 경우 환경별 DB 스키마 차이로 포함되지 않은 필드 삭제 처리
            if ($modelClass instanceof MsSqlModel) {
                foreach ($sltConfig['fieldDefaultTitles'] as $fieldName => $fieldDefaultTitle) {
                    if (array_search($fieldName, $modelClass->getAllFieldNames()) === false) {
                        unset($sltConfig['fieldDefaultTitles'][$fieldName]);
                    }
                }
            }

            // smartLookupTableConfigName 속성 추가
            $sltConfig['smartLookupTableConfigName'] = $smartLookupTableConfigName;

            if (is_array($sltConfig['valueCategoryInfos'])) {
                foreach ($sltConfig['valueCategoryInfos'] as $fieldName => $category) {
                    $this->smartLookupTableConfigNamesChainedWithCategory[$category][$smartLookupTableConfigName] = true;
                }
            }


            // fieldInfos 생성. 시작
            $fieldInfos = [];

            // 실제 필드들에 대한 정보 추가
            foreach ($sltConfig['fieldDefaultTitles'] as $fieldName => $fieldDefaultTitle) {

                $fieldInfos[$fieldName] = [
                    'key' => $fieldName,
                    'tableColName' => $fieldDefaultTitle,
                ];

                if ($bUseStringResourceId) {
                    $stringResourceId = StringReference::getSmartLookupTableFieldStringId($smartLookupTableConfigName, $fieldName);
                    $fieldInfos[$fieldName]['tableColSrid'] = $stringResourceId;
                }
            }

            $sltConfig['fieldInfos'] = $fieldInfos;
            // fieldInfos 생성. 마침


            // 정렬용 필드 인덱스 정보 배열 구하기. 시작
            $idx = 0;
            $sortingFieldIndexInfos = [];
            $orderFieldInfo = $sltConfig['orderFieldInfo'] ?? null;
            $sortingFiledNames = $sltConfig['sortingFiledNames'] ?? [];
            if ($orderFieldInfo) {
                // 순서는 필드명이 없으므로 여기서 가상의 필드명을 넣어준다
                $sltConfig['orderFieldInfo']['fieldName'] = SmartLookupInterface::ORDER_FIELD_NAME;
                if ($orderFieldInfo['bUseSortField'] ?? false) {
                    $sortingFieldIndexInfos[$idx++] = SmartLookupInterface::ORDER_FIELD_NAME ?? null;
                } else {
                    $sortingFieldIndexInfos[$idx++] = null;
                }
            }
            foreach ($fieldInfos as $fieldName => $filedInfo) {
                if (array_search($fieldName, $sortingFiledNames) === false) {
                    // 테이블 정렬용 필드가 아닌 경우
                    $sortingFieldIndexInfos[$idx++] = null;
                } else {
                    // 테이블 정렬용 필드인 경우
                    $sortingFieldIndexInfos[$idx++] = $fieldName;
                }

            }
            $sltConfig['sortingFieldIndexInfos'] = $sortingFieldIndexInfos;
            // 정렬용 필드 인덱스 정보 배열 구하기. 마침


        }
        // 자동으로 처리해주는 속성 관련 처리. 마침
    }


    public function getSmartLookupTableConfigNamesChainedWithCategory($category)
    {
        return array_keys($this->smartLookupTableConfigNamesChainedWithCategory[$category]);
    }


    public function getConfig($smartLookupTableConfigName)
    {
        return $this->configArr[$smartLookupTableConfigName];
    }

    public function getConfigArr()
    {
        return $this->configArr;
    }

    public function mapConfigArr($callback)
    {
        array_map($callback, $this->configArr);
    }

    public function isExistConfig($smartLookupTableConfigName)
    {
        return !is_null($this->configArr[$smartLookupTableConfigName]);
    }

    public function getLinkedModelClass($smartLookupTableConfigName)
    {
        return $this->configArr[$smartLookupTableConfigName]['modelClass'];
    }

    /**
     * @param string $smartLookupTableConfigName
     * @param int $userDbNodeIndex
     * @return UserDataModel|UserSingleDataModel
     */
    public function getLinkedUserDbModelClass($smartLookupTableConfigName, $userDbNodeIndex)
    {
        $config = $this->getConfig($smartLookupTableConfigName);
        $modelName = $config['modelName'] ?? '';
        $configModel = $this->instanceFactory->userDataModelInstance($modelName, $userDbNodeIndex);

        return $configModel;
    }

}
