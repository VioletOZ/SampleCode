1. 프로젝트 개요
 - 게임서버 프로젝트
   간략한 TCP 기반 멀티스레드 서버
   서버는 Gateway, GameServer 로 구성
2. 서버 아키텍처
  a. Gateway
   - 클라이언트에서 처음 연결하여 인증 및 패킷 라우팅하는서버
  b. GameServer
   - 게임 로직을 담당하는서버
     Core
	 Network
	 Game
	 Server
	 
3. 핵심 설계 결정
   - 서버는 클라이언트에서 온 입력만 처리
   - 게임상태 일관성을 유지하기위해 tick 기반 single thread 로 처리
   - 전달받은 패킷은 JobQueue 기반 메시징처리 
   - 객체 생성 비용을 줄이기위해 size class 기반 memory pool생성
4. 멀티스레드 구조 
   - MPSC 처리로 게임상태 접근에 대한 처리가 단순하게 함
   - IO Thread 에서 받은 패킷을 Queue 로 보내고 이걸 worker thread 에서 처리
     I/OThread 1 
	 I/OThread 2    -> JobQueue -> GameThread
	 I/OThread 3
   - 락을 최소화 하여 동기화를 최소화하기 윟서 채택한 방식
   - JobQueue 는 데이터 담당만하고 직접적인 접근은 하지않음
5. 메모리 관리 전략
   - 메모리 풀을 사용해서 처리
   - 생성비용과 단편화 및 캐시접근속도를 높이는데 중점을 둠
   - class pool 과 packet pool, job queue 를 이용
6. 네트워크 처리 흐름
   - Client -> Gateway -> Deserialize -> JobQueue -> GameLoop(SingleTHread) -> Serialize
   - packet header ( size + type)
   - sequence number 로 패킷 구분ㄴ
7. 장애/운영 시나리오
   - GameThread 가 느리면 QUeue 가 쌓임
   - 배칭처리나 특정 작업은 드랍하는방향으로 설정
8. 개선 가능 포인트
   - Queue 에 작업이 몰렸을때 처리
   - Job 분류하여 중요도에 따라 처리 방식
   - job 에 데이터 넣을때 I/OThread에서 모아서 한번에 처리하기
