package server.stresser;

import org.apache.log4j.Logger;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.json.simple.parser.JSONParser;

import java.io.File;
import java.io.FileReader;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ConcurrentHashMap;
import java.text.SimpleDateFormat;
import java.text.DateFormat;

import server.StresserConfig;
import server.stresser.StresserSession.State;

public class ScenarioManager {

    private static final Logger logger = Logger.getLogger(ScenarioManager.class);
    private long processTime = 0L;
    private DateFormat df = new SimpleDateFormat("ss.SSSS");

    public class StressTest implements Cloneable {
        public String name;
        public List<Scenario> scenarios = new ArrayList<>();

        public StressTest(String name) {
            this.name = name;
        }

        public void addScenario(Scenario scenario) {
            this.scenarios.add(scenario);
        }

        public StressTest clone() throws CloneNotSupportedException {
            StressTest clone = (StressTest) super.clone();
            List<Scenario> scenarios = new ArrayList<>();
            for (Scenario scenario : clone.scenarios) {
                scenarios.add(scenario.clone());
            }
            clone.scenarios = scenarios;
            return clone;
        }
    }

    public class Scenario implements Cloneable {
        public int id;
        public String name;
        public int number = 0;
        public List<Content> contents = new ArrayList<>();

        public Scenario(String name) {
            this.name = name;
        }

        public void addContent(Content content) {
            this.contents.add(content);
        }

        public Content findNotFinishedContent(State state) {
            // 아직 완료되지 않은 컨텐츠 중 특정 state의 컨텐츠를 찾음

            for (Content content : contents) {
                if (content.state.value == state.value && !content.finished) {
                    return content;
                }
            }
            return null;
        }

        public Scenario clone() throws CloneNotSupportedException {
            Scenario clone = (Scenario) super.clone();
            List<Content> contents = new ArrayList<>();
            for (Content content : clone.contents) {
                contents.add(content.clone());
            }
            clone.contents = contents;
            return clone;
        }
    }

    public static class Content implements Cloneable {
        public State state = State.IDLE;
        public int number = 0; // player number
        public boolean repeat = false;
        public double percent = 0;
        public boolean wait = true;
        public ConcurrentHashMap<String, Object> data = new ConcurrentHashMap<>();
        public boolean finished = false; // 해당 컨텐츠가 모든 더미스트레서의 모든 세션에서 끝났는지를 알려줌
        public JSONObject jsonData = null;

        public synchronized void finished(int number) {
            // 각 더미스트레서에서 컨텐츠를 완료한 세션의 개수를 이곳(메인스트레서의 컨텐츠)에서 관리
            // number은 모든 더미스트레서에서 총 수행되어야 할 세션수로 처음 설정되는데,
            // number 가 0 이 되면 모든 더미 스트레서의 모든 세션들에서 컨텐츠가 완료되었단 뜻이므로 finished를 true로 변경

            this.number -= number;
            if (this.number == 0) {
                this.finished = true;
            }
        }

        public synchronized void setNumber(int number) {
            this.number -= number;
        }

        public Content clone() throws CloneNotSupportedException {
            return (Content) super.clone();
        }
    }

    private static final ScenarioManager singleton = new ScenarioManager();
    public static ScenarioManager get() { return singleton; }

    private List<StressTest> stressTestsInfo = new ArrayList<>(); // 스트레스 테스트들의 정보
    private List<Scenario> scenariosInfo = new ArrayList<>(); // 시나리오들의 정보
    private StressTest currentStressTest; // 현재 실행중인 스트레스 테스트
    private Scenario currentScenario; // 현재 실행중인 시나리오

    public void init() {
        try {
            /*
               scenarios 에 있는 스트레스 테스트와 시나리오, 컨텐츠들의 정보를 로드한다.
               스트레스 테스트는 scenarios에 있는 정보만을 토대로 진행된다.

               scenarios 에는 모든 시나리오들이 담겨있는데, 각 시나리오는 다음과 같이 구성된다.

               id : 시나리오의 id
               name : 시나리오의 이름
               number : 해당 시나리오를 실행할 세션 수
               contents : 시나리오를 구성하는 컨텐츠들

               contents 에는 해당 시나리오의 모든 컨텐츠들이 담겨있는데, 각 컨텐츠는 다음과 같이 구성된다.

               state : 컨텐츠의 종류
               repeat : 반복해서 실행되는 컨텐츠인가
               wait : 해당 컨텐츠가 모든 세션에서 완료될 때 까지 기다려야 하는가 (true라면 완료되기 전까지는 다음 컨텐츠를 실행하지 못함)
               percent : 세션 내의 스레드에서 매번 반복될 때 해당 컨텐츠가 실행될 확률, 0~1 사이의 소수점으로 구성
               (UNIT_MOVE의 경우 캐릭터는 계속 움직이니 1.0인 반면 USE_SKILL의 경우 예를 들어 매 스레드의 반복마다 10%의 확률로 실행되게끔 하고 싶으면 0.1)
               data_Type : 해당 컨텐츠 실행에 필요한 데이터 종류, 이곳에 필요한 데이터를 넣고 그 뒤에 본인이 데이터를 직접 추가하면 된다.

                stress_tests 에는 스트레스 테스트들이 담겨있는데, 각 스트레스 테스트는 다음과 같이 구성된다.

                name : 스트레스 테스트의 이름
                scenario_ids : 실행할 시나리오의 종류, 시나리오 단위로 순서대로 실행된다.
            */

            File file = new File("scenarios");
            if (!file.exists()) {
                System.out.println("Scenarios file does not exist!");
                return;
            }

            JSONParser parser = new JSONParser();
            Object obj = parser.parse(new FileReader(file));
            org.json.simple.JSONObject jsonObject = (org.json.simple.JSONObject) obj;

            JSONArray jsonScenarios = new JSONObject(jsonObject.toJSONString()).getJSONArray("scenarios");

            for (int i = 0; i < jsonScenarios.length(); i++) {
                JSONObject jsonScenario = jsonScenarios.getJSONObject(i);
                Scenario scenario = new Scenario(jsonScenario.getString("name"));
                scenario.id = jsonScenario.getInt("id");
                int number = StresserConfig.playerNum;
                scenario.number = number;
                JSONArray contents = jsonScenario.getJSONArray("contents");
                for (int j = 0; j < contents.length(); j++) {
                    JSONObject jsonContent = contents.getJSONObject(j);
                    Content content = new Content();
                    content.state = State.valueOf(jsonContent.getString("state"));
                    content.number = number;
                    content.wait = jsonContent.getBoolean("wait");
                    content.repeat = jsonContent.getBoolean("repeat");
                    if (content.repeat) content.percent = jsonContent.getDouble("percent");

                    try {
                        JSONArray dataType = jsonContent.getJSONArray("data_type");

                        for (int k = 0; k < dataType.length(); k++) {
                            String key = (String) dataType.get(k);
                            content.data.put(key, jsonContent.get(key));
                        }
                    } catch (JSONException e) {
                    }

                    content.jsonData = jsonContent;

                    scenario.addContent(content);
                }

                scenariosInfo.add(scenario);
            }

            JSONArray jsonStressTests = new JSONObject(jsonObject.toJSONString()).getJSONArray("stress_tests");
            for (int i = 0; i < jsonStressTests.length(); i++) {
                JSONObject jsonStressTest = jsonStressTests.getJSONObject(i);
                StressTest stressTest = new StressTest(jsonStressTest.getString("name"));
                JSONArray scenarioIds = jsonStressTest.getJSONArray("scenario_ids");
                for (int j = 0; j < scenarioIds.length(); j++) {

                    int scenarioId = scenarioIds.getInt(j);

                    for (Scenario item : scenariosInfo) {
                        if (item.id == scenarioId) {
                            stressTest.addScenario(item);
                            break;
                        }
                    }
                }

                stressTestsInfo.add(stressTest);
            }

        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void runStressTest(int index) throws Exception {
        currentStressTest = stressTestsInfo.get(index).clone();
        List<Scenario> scenarios = currentStressTest.scenarios;

        // 스트레스 테스트 안에 있는 시나리오들을 차례대로 실행
        for (Scenario scenario : scenarios) {
            runScenario(scenario);
        }

    }

    public void runScenario(int index) throws Exception {

        runScenario(scenariosInfo.get(index).clone());
    }

    public void runScenario(Scenario scenario) throws Exception {
        currentScenario = scenario;

        List<Content> contents = currentScenario.contents;

        // 재실행일때... 현재 캐릭터들 생성및 로그인이 되어있으니 캐릭생성로긴부분은 패스.. 튜토리얼도 재실행시에는 패스
        if ((StresserManager.get().getRepeat() && scenario.id == 1) || (StresserManager.get().getRepeat() && scenario.id == 2))
            return;


        logger.info("[Scenario Start : " + scenario.name + "]");
        System.out.print("[Scenario Start : " + scenario.name + "]\n");
        processTime = System.currentTimeMillis();
        // 시나리오 내에 있는 컨텐츠 중 기다리지 않아도 되는 컨텐츠 체크
        for (int i = 0; i < contents.size(); i++) {
            Content content = contents.get(i);
            if (!content.wait)
                // 아래 checkScenarioFinished 메소드에서 content의 wait flag를 체크해서 현재 시나리오의 모든 컨텐츠들이 실행 완료되었는지 체크하기 때문에
                // 기다리지 않아도 되는 컨텐츠는 이곳에서 미리 끝났다는 표시를 해둔다.
                content.finished = true;
        }

        // 더미 스트레서에게 시나리오를 실행하라는 이벤트 전달
        StresserManager.get().handler.handleScenario(scenario.number, scenario);

        while (!checkScenarioFinished()) {
            // 현재 시나리오의 모든 컨텐츠가 실행 완료될 때 까지 대기
            Thread.sleep(100);
        }
        processTime = (System.currentTimeMillis() - processTime);
        logger.info("[Scenario End : " + scenario.name + "] - " + df.format(processTime));
        System.out.print("[Scenario End : " + scenario.name + "] - " + df.format(processTime) +"\n");

        // 현재 시나리오의 실행을 모두 마쳤으므로 현재 시나리오를 null로 지정
        currentScenario = null;
    }

    public boolean checkScenarioFinished() {
        // 현재 실행중인 시나리오의 모든 컨텐츠가 실행 완료되었는지 체크한다
        // 하나의 컨텐츠라도 실행이 완료되지 않았다면 (content.finished 가 false라면) false를 리턴한다.
        // 컨텐츠가 완료되어 true 되는 시점은 Content 클래스의 finished 메소드를 보면 이해할 수 있다

        if (currentScenario == null) return true;

        for (Content content : currentScenario.contents) {
            if (!content.finished) {
                return false;
            }
        }
        return true;
    }

    public List<Scenario> getScenariosInfo() {
        return this.scenariosInfo;
    }

    public Scenario getScenario() {
        return this.currentScenario;
    }

    public List<StressTest> getStressTestsInfo() {
        return this.stressTestsInfo;
    }

    public Scenario getScenarioByID(int id) {
        for (Scenario scenario : scenariosInfo) {
            if (scenario.id == id) return scenario;
        }
        return null;
    }
}
