<?php
namespace RpgA\GmSite;

use EToyLab\Utils\Util;
use RpgA\InstanceFactory;
use RpgA\Models\FlexibleModel;
use RpgA\Models\PublicSingleDataModel;
use RpgA\Models\UserSingleDataModel;

class SmartDataPageConfig
{
    const CONFIG_NAME_SVR_CONTROL = 'SvrControl';

    const CONFIG_NAME_GAME_CONFIG_BALANCE = 'GameConfigBalance';

    const CONFIG_NAME_GAME_CONFIG_INIT = 'GameConfigInit';

    const CONFIG_NAME_USER_DATA = 'UserData';

    const CONFIG_NAME_USER_GOODS = 'UserGoods';

    const CONFIG_NAME_USER_WEEK_MISSION = 'UserWeekMission';

    const CONFIG_NAME_USER_MARK = 'UserMark';

    const CONFIG_NAME_USER_PUSH_NOTI = 'UserPushNoti';

    const CONFIG_NAME_USER_STAGE = 'UserStage';

    const CONFIG_NAME_USER_INIT_ITEM = 'UserInitItem';

    const CONFIG_NAME_USER_LOBBY_THEME = 'UserLobbyThemeLog';

    /**
     * @var InstanceFactory
     */
    protected $instanceFactory;

    /**
     * @var mixed[]
     */
    protected $configArr;

    protected $smartDataPageConfigNamesChainedWithCategory = array();    // 한 카테고리를 참조하는 smartDataPageConfigName 들의 목록

    /**
     * @param bool $bEditAble
     * @param bool $bDeleteAble
     * @param bool $bInsertAble
     * @param bool $bModifyAble
     * @param bool $bExcelControl
     * @return array
     */
    public static function createJsClassConfigObj($bEditAble = false, $bDeleteAble = false, $bInsertAble = false, $bModifyAble = false, $bExcelControl = false)
    {
        return [
            'bDbEditable' => $bEditAble,                    // 데이터 추가, 수정, 삭제 기능 사용 여부
            "bDbDeleteDisabled" => !$bDeleteAble,             // bDbEditable 가 true 일 때, 데이터 기능을 막을지 여부.
            "bInsertDataDisabled" => !$bInsertAble,           // bDbEditable 가 true 일 때, "신규 입력(직접)" 기능을 막을지 여부
            "bDbModifyDisabled" => !$bModifyAble,             // bDbEditable 가 true 일 때, 데이터 수정 기능을 막을지 여부.

            "bRefreshFromDbDisabled" => false, 			    // "DB 에서 새로읽기" 기능을 막을지 여부

            "bUpdateDbFromXlsDisabled" => !$bExcelControl,  // "액셀(.xls)파일에서 읽어 DB 덮어쓰기" 기능을 막을지 여부
            "bSaveOnlyFormatXlsDisabled" => !$bExcelControl,// 데이터 의 포맷 만 액셀(.xls)파일로 저장 하는 기능을 막을지 여부
            "bSaveDataXlsDisabled" => !$bExcelControl,  // 데이터 전체를 액셀(.xls)파일로 저장하는 기능을 막을지 여부

//            "bSaveBdjAbled" => Boolean,                       // 데이터 하나를 *.bdj 파일로 저장 하는 기능 사용 여부
//            "bInsertDataFromBdjAbled" => Boolean,             // bDbEditable 가 true 일 때, "신규 입력(from *.bdj)" 기능을 허용할지 여부
//            "bAppendDatasFromBdjAbled" => Boolean,            // bDbEditable 가 true 일 때, "신규 등록(from *.bdj)" 기능을 허용할지 여부
//            "bSaveDatasBdjAbled" => Boolean,                  // 체크된 데이터 들을 *.bdj 파일로 저장 하는 기능 사용 여부
        ];
    }

    public static function createJsClassConfigObjForUser($bEditAble = true, $bDeleteAble = false, $bModifyAble = true, $bInsertAble = false)
    {
        return [
            'bDbEditable' => $bEditAble,                    // 데이터 추가, 수정, 삭제 기능 사용 여부
            "bDbDeleteDisabled" => !$bDeleteAble,             // bDbEditable 가 true 일 때, 데이터 기능을 막을지 여부.
            "bInsertDataDisabled" => !$bInsertAble,           // bDbEditable 가 true 일 때, "신규 입력(직접)" 기능을 막을지 여부
            "bDbModifyDisabled" => !$bModifyAble,             // bDbEditable 가 true 일 때, 데이터 수정 기능을 막을지 여부.

            "bRefreshFromDbDisabled" => true, 			    // "DB 에서 새로읽기" 기능을 막을지 여부

            "bUpdateDbFromXlsDisabled" => true,  // "액셀(.xls)파일에서 읽어 DB 덮어쓰기" 기능을 막을지 여부
            "bSaveOnlyFormatXlsDisabled" => true,// 데이터 의 포맷 만 액셀(.xls)파일로 저장 하는 기능을 막을지 여부
            "bSaveDataXlsDisabled" => true,  // 데이터 전체를 액셀(.xls)파일로 저장하는 기능을 막을지 여부

//            "bSaveBdjAbled" => Boolean,                       // 데이터 하나를 *.bdj 파일로 저장 하는 기능 사용 여부
//            "bInsertDataFromBdjAbled" => Boolean,             // bDbEditable 가 true 일 때, "신규 입력(from *.bdj)" 기능을 허용할지 여부
//            "bAppendDatasFromBdjAbled" => Boolean,            // bDbEditable 가 true 일 때, "신규 등록(from *.bdj)" 기능을 허용할지 여부
//            "bSaveDatasBdjAbled" => Boolean,                  // 체크된 데이터 들을 *.bdj 파일로 저장 하는 기능 사용 여부
        ];
    }

    /**
     * 필드 속성 정보를 만들어준다
     * @param bool $pageShow 테이블에서 이 필드가 보이는지 여부
     * @param null|string $title UI 에서 보이는 이 필드의 이름. null 이면 자동 결정
     * @param bool $nullEnabled null 값이 허용되는 필드인지 여부
     * @param null|bool $insertDisabled 신규 입력시 이 필드 편집 가능 여부. null 이면 자동 결정
     * @param null|bool $updateDisabled 수정시 이 필드 편집 가능 여부. null 이면 자동 결정
     * @param null|string $valueCommentatorCategory 입력 UI 카테고리. null 이면 사용 안함
     * * @param number|string|null $defaultValue 기본 값
     * @return array
     */
    public static function createFieldProp($pageShow = true, $title = null, $nullEnabled = false, $insertDisabled = null, $updateDisabled = null, $valueCommentatorCategory = null, $defaultValue = '')
    {
        return [
            'pageView' => $pageShow,
            'title' => $title,
            'nullEnabled' => $nullEnabled,
            'insertDisabled' => $insertDisabled,
            'updateDisabled' => $updateDisabled,
            'valueCommentatorCategory' => $valueCommentatorCategory,
            'defaultValue' => $defaultValue,
        ];
    }

    public static function createVirtualFieldProp($pageShow = true, $title = null, $valueCommentatorCategory = null)
    {
        $nullEnabled = true;
        $insertDisabled = true;
        $updateDisabled = true;

        return self::createFieldProp($pageShow, $title, $nullEnabled, $insertDisabled, $updateDisabled, $valueCommentatorCategory);
    }

    protected static function createFieldUiInfo($smartDataPageConfigName, $bUseStringResourceId, $fieldName, $fieldDataType, $fieldTitle = null, $bPrimaryNoField = false, $bNullEnable = false, $insertDisabledField = false, $updateDisabledField = false, $bPageShow = true, $defaultValue = '')
    {
        if (is_null($fieldTitle)) {
            $fieldTitle = '[' . $fieldName . ']';
        }

        $stringResourceId = null;
        if ($bUseStringResourceId) {
            $stringResourceId = StringReference::getSmartDataPageFieldStringId($smartDataPageConfigName, $fieldName);
        }

        // GM site UI 속성 결정
        {
            // 페이지 노출 여부
            $uiPageShow = $bPageShow;

            // 신규 입력 창 - 노출 여부
            $uiInsertShow = !$insertDisabledField;

            // 신규 입력 창 - 편집 가능 여부
            $uiInsertEditAble = !$insertDisabledField;

            // 신규 입력 창 - API 인자에 포함 여부
            $uiInsertApiIncluded = !$insertDisabledField;

            // 수정 창 - 노출 여부. 일단은 무조건 노출
            $uiUpdateShow = true;

            // 수정 창 - 편집 가능 여부
            $uiUpdateEditAble = !$updateDisabledField;

            // 수정 창 - API 인자에 포함 여부
            $uiUpdateApiIncluded = (!$updateDisabledField || $bPrimaryNoField);
        }

        return [
            'key' => $fieldName,
            'dataType' => $fieldDataType,
            'bNullEnable' => $bNullEnable,
            'default' => $defaultValue,
//            'tableCol' => (($bPageShow)?'show':'hide'),
            'fieldLabelValue' => $fieldTitle,
            'fieldLabelSrid' => $stringResourceId,
//            'fieldIdentity' => $fieldIdentity,

            'bPrimaryNoField' => $bPrimaryNoField,

            'uiPageShow' => $uiPageShow,
            'uiInsertShow' => $uiInsertShow,
            'uiInsertEditAble' => $uiInsertEditAble,
            'uiInsertApiIncluded' => $uiInsertApiIncluded,
            'uiUpdateShow' => $uiUpdateShow,
            'uiUpdateEditAble' => $uiUpdateEditAble,
            'uiUpdateApiIncluded' => $uiUpdateApiIncluded,
        ];
    }

    protected static function createPrimaryNoFieldUiInfo($smartDataPageConfigName, $bUseStringResourceId, $fieldName, $fieldDataType, $fieldTitle = null, $insertDisabledField = true, $bPageShow = true)
    {
        $bPrimaryNoField = true;
        $updateDisabledField = true;
        $bNullEnable = false;

        return self::createFieldUiInfo($smartDataPageConfigName, $bUseStringResourceId, $fieldName, $fieldDataType, $fieldTitle, $bPrimaryNoField, $bNullEnable, $insertDisabledField, $updateDisabledField, $bPageShow);
    }

    protected static function createNormalFieldUiInfo($smartDataPageConfigName, $bUseStringResourceId, $fieldName, $fieldDataType, $fieldTitle = null, $bNullEnable = false, $insertDisabledField = false, $updateDisabledField = false, $defaultValue = '', $bPageShow = true)
    {
        $bPrimaryNoField = false;

        return self::createFieldUiInfo($smartDataPageConfigName, $bUseStringResourceId, $fieldName, $fieldDataType,  $fieldTitle, $bPrimaryNoField, $bNullEnable, $insertDisabledField, $updateDisabledField, $bPageShow, $defaultValue);
    }

    /**
     *
     * @param InstanceFactory $instanceFactory
     */
    public function __construct($instanceFactory)
    {
        $this->instanceFactory = $instanceFactory;

        $this->configArr = [];

        // 페이지 설정 배열 채우기
        $includeDir = __DIR__ . '/SmartDataPageConfigs';
        $handle = opendir($includeDir);
        while (false !== $fileName = readdir($handle)) {
            $filePath = $includeDir . '/' . $fileName;
            if (is_file($filePath)) {
                require $filePath;
            }
        }
        closedir($handle);


        // 자동으로 처리해주는 속성 관련 처리. 시작
        foreach ($this->configArr as $smartDataPageConfigName => &$sdpConfig) {

            $modelClass = $sdpConfig['modelClass'];
            if (!($modelClass instanceof FlexibleModel)) {
                throw new \Exception("No modelClass. smartDataPageConfigName:{$smartDataPageConfigName}");
            }

            $bUseStringResourceId = $sdpConfig['bUseStringResourceId'] ?? false;

            // smartDataPageConfigName 속성 추가
            $sdpConfig['smartDataPageConfigName'] = $smartDataPageConfigName;


            // 모델 명 구하기
            $sdpConfig['modelName'] = $modelClass->modelName();

//            // 게임 데이터 이름 속성 추가. 게임 데이터가 아닌 경우 null 이 들어간다
//            $sdpConfig['gameDataName'] = $instanceFactory->gameDataFactory()->getGameDataName($modelClass);
//
//            // 통합 게임 데이터에 해당하는지 여부
//            $sdpConfig['bTotalGameData'] =  ($sdpConfig['gameDataName'])?true:false;

            // sheetTitle 기본값 적용
            if (!Util::getArrayValue($sdpConfig, 'sheetTitle')) {
                $sdpConfig['sheetTitle'] = $smartDataPageConfigName;
            }

//            if (isset($sdpConfig['valueCategoryInfos']) && is_array($sdpConfig['valueCategoryInfos'])) {
//                foreach ($sdpConfig['valueCategoryInfos'] as $fieldName => $category) {
//                    $this->smartDataPageConfigNamesChainedWithCategory[$category][$smartDataPageConfigName] = true;
//                }
//            } else {
//                $sdpConfig['valueCategoryInfos'] = [];
//            }

//            $sdpConfig['fieldLabelStringResourceInfos'] = array();
//            foreach ($modelClass->getDataFieldNames() as $fieldName) {
//
//                $fieldLabel = $sdpConfig['fieldDefaultLabels'][$fieldName] ?? null;
//                if (is_null($fieldLabel)) {
//                    $fieldLabel = '[' . $fieldName . ']';
//                }
//
//                $stringResourceId = "smartDataPage.$smartDataPageConfigName.col_name.$fieldName";
//                $sdpConfig['fieldLabelStringResourceInfos'][$fieldName] = array(
//                    'srid' => $stringResourceId,
//                    'default' => $fieldLabel,
//                );
//            }

            // value category info 배열 초기화
            $valueCategoryInfos = [];

            // fieldInfos 생성. 시작
            $fieldInfos = [];

            // 고유 번호 필드명
            $primaryNoFieldName = $modelClass->getPrimaryNoFieldName();

            // 실제 필드들의 이름 배열
            $dataFieldFieldNameArr = $modelClass->getDataRealFieldNames();

            // 모든 필드들의 이름 배열
            $allFieldNameArr = $modelClass->getAllFieldNames();

            // 필드 속성 배열
            $fieldProps = $sdpConfig['fieldProps'] ?? [];

//            // 고유 번호 필드가 실제 필드에 포함되지 않은 경우, 정보 추가
//            if (array_search($primaryNoFieldName, $dataFieldFieldNameArr) === false) {
//                $fieldInfos[$primaryNoFieldName] = self::createPrimaryNoFieldUiInfo($smartDataPageConfigName, $primaryNoFieldName, SmartData::VALUE_TYPE_STRING, '(가상)고유 번호');
//            }

            // 필드들에 대한 정보 추가
            foreach ($allFieldNameArr as $fieldName) {
                $fieldDataType = $modelClass->getDataFieldTypeArr()[$fieldName];

                $fieldProp = $fieldProps[$fieldName] ?? null;
                if (!$fieldProp) {
                    $fieldProp = self::createFieldProp();
                }

                $bPageShow = $fieldProp['pageShow'] ?? false;
                $fieldTitle = $fieldProp['title'] ?? null;
                $bNullEnable = $fieldProp['nullEnabled'] ?? false;
                $insertDisabled = $fieldProp['insertDisabled'] ?? null;
                $updateDisabled = $fieldProp['updateDisabled'] ?? null;
                $valueCommentatorCategory = $fieldProp['valueCommentatorCategory'] ?? null;

                // defaultValue 값이 null 일수도 있으므로, ?? 연산자를 쓰면 안된다
                if (array_key_exists('defaultValue', $fieldProp)) {
                    $defaultValue = $fieldProp['defaultValue'];
                } else {
                    $defaultValue = '';
                }

                if ($valueCommentatorCategory) {
                    $valueCategoryInfos[$fieldName] = $valueCommentatorCategory;
                }

                if ($primaryNoFieldName == $fieldName) {
                    // 고유 번호 필드인 경우 (이면서 고유 번호 필드가 실제 필드에 포함된 경우)

                    // 신규 입력시 해당 필드 편집 불가 여부 결정. 기본 값은 편집 불가(즉, true) 이다
                    if ($insertDisabled === false) {
                        $insertDisabledField = false;
                    } else {
                        $insertDisabledField = true;
                    }

                    $fieldInfos[$fieldName] = self::createPrimaryNoFieldUiInfo($smartDataPageConfigName, $bUseStringResourceId, $fieldName, $fieldDataType, $fieldTitle, $insertDisabledField, $bPageShow);

                } else {
                    // 고유 번호 필드가 아닌 경우.

                    // 신규 입력시 해당 필드 편집 불가 여부 결정. 기본 값은 편집 가능(즉, false) 이다
                    if (is_null($insertDisabled) || $insertDisabled === false) {
                        $insertDisabledField = false;
                    } else {
                        $insertDisabledField = true;
                    }

                    // 수정시 해당 필드 편집 불가 여부 결정.
                    if (is_null($updateDisabled)) {
                        // 기본 값은 고유키 구성 필드인지에 따라서 다르다. 고유키 구성 필드이면 편집 불가(즉, true), 고유키 구성 필드가 아니면 편집 가능(즉, false)
                        $updateDisabledField = ($modelClass->isPrimaryKey($fieldName));
                    } elseif ($updateDisabled === false) {
                        $updateDisabledField = false;
                    } else {
                        $updateDisabledField = true;
                    }

                    $fieldInfos[$fieldName] = self::createNormalFieldUiInfo($smartDataPageConfigName, $bUseStringResourceId, $fieldName, $fieldDataType, $fieldTitle, $bNullEnable, $insertDisabledField, $updateDisabledField, $defaultValue, $bPageShow);
                }

            }

            $sdpConfig['fieldInfos'] = $fieldInfos;
            // fieldInfos 생성. 마침


            // value category info 배열 설정
            $sdpConfig['valueCategoryInfos'] = $valueCategoryInfos;
        }
        // 자동으로 처리해주는 속성 관련 처리. 마침
    }


    public function getsmartDataPageConfigNamesChainedWithCategory($category)
    {
        return array_keys($this->smartDataPageConfigNamesChainedWithCategory[$category]);
    }


    public function getConfig($smartDataPageConfigName)
    {
        return $this->configArr[$smartDataPageConfigName];
    }

    public function getConfigArr()
    {
        return $this->configArr;
    }

    public function mapConfigArr($callback)
    {
        array_map($callback, $this->configArr);
    }

    public function isExistConfig($smartDataPageConfigName)
    {
        return !is_null($this->configArr[$smartDataPageConfigName]);
    }

    /**
     * @param string $smartDataPageConfigName
     * @return UserSingleDataModel|PublicSingleDataModel
     */
    public function getLinkedModelClass($smartDataPageConfigName)
    {
        return $this->configArr[$smartDataPageConfigName]['modelClass'];
    }

    /**
     * @param string $smartDataPageConfigName
     * @param int $uuid 유저 고유키
     * @return UserSingleDataModel
     */
    public function getLinkedUserDataModelClass($smartDataPageConfigName, $uuid)
    {
        $nodeIndex = getUserDbNodeIndexFromUuid($uuid);

        $config = $this->getConfig($smartDataPageConfigName);
        $modelName = $config['modelName'] ?? '';
        $configModel = $this->instanceFactory->userDataModelInstance($modelName, $nodeIndex);

        return $configModel;
    }

    public function getJsClassName($smartDataPageConfigName)
    {
        return $this->configArr[$smartDataPageConfigName]['jsClassName'];
    }

    public function getVerifyDataFunc($smartDataPageConfigName)
    {
        $config = $this->getConfig($smartDataPageConfigName);

        return $config['verifyDataFunc'] ?? null;
    }

    public function getExcelSheetTitle($smartDataPageConfigName)
    {
        return $this->configArr[$smartDataPageConfigName]['sheetTitle'];
    }

    /**
     * 외부에 보이면 안되는 데이터인지 여부 체크
     * @param string $smartDataPageConfigName
     * @return boolean
     */
    public function checkHideOnOutside($smartDataPageConfigName)
    {
        return $this->configArr[$smartDataPageConfigName]['bHideOnOutside'];
    }

    /**
     * 외부에 데이터 버전을 보이면 안되는지 여부 체크
     * @param string $smartDataPageConfigName
     * @return boolean true 면 외부에 보이면 안됨
     */
    public function checkHideVersionNoOnOutside($smartDataPageConfigName)
    {
        return ($this->checkHideOnOutside($smartDataPageConfigName) || $this->configArr[$smartDataPageConfigName]['bHideVersionNoOnOutside']);
    }

    public function getSmartDataPageConfigNameFromSheetTitle($sheetTitle)
    {

        if ($sheetTitle == "") {
            return null;
        }

        foreach ($this->configArr as $key => $data) {
            if ($data['sheetTitle'] == $sheetTitle) {
                return $key;
            }
        }

        return null;
    }


}
