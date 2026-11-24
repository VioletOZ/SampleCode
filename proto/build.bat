@echo off
rem *****************************************************
rem .proto -> C++/C# 코드 생성 + 서버/클라이언트 proto 폴더로 복사
rem protoc는 서버 vcpkg manifest의 것(런타임과 같은 29.3)만 사용한다.
rem (없으면 OZServer.sln을 한 번 빌드해서 vcpkg 설치)
rem *****************************************************
chcp 65001 > nul
setlocal
cd /d "%~dp0"

set PROTOC=..\NetworkServer\vcpkg_installed\x64-windows\x64-windows\tools\protobuf\protoc.exe
set SERVER_PROTO=..\NetworkServer\OZServer\proto
set CLIENT_PROTO=..\Client\proto
set path_cpp=cpp_out
set path_csharp=csharp_out

if not exist "%PROTOC%" (
	echo [오류] protoc 없음: %PROTOC%
	echo        서버 솔루션 OZServer.sln을 먼저 빌드하세요 - vcpkg 설치
	goto fail
)
"%PROTOC%" --version
if errorlevel 1 goto fail

if not exist "%SERVER_PROTO%\" (
	echo [오류] 서버 proto 폴더 없음: %SERVER_PROTO%
	goto fail
)
if not exist "%CLIENT_PROTO%\" (
	echo [오류] 클라이언트 proto 폴더 없음: %CLIENT_PROTO%
	goto fail
)

rem ---- 출력 폴더 비우기 ----
if not exist %path_cpp% mkdir %path_cpp%
if not exist %path_csharp% mkdir %path_csharp%
del /Q %path_cpp%\*.* > nul 2>&1
del /Q %path_csharp%\*.* > nul 2>&1

rem ---- 코드 생성 (실패 시 복사하지 않음) ----
echo 코드 생성...
"%PROTOC%" -I . --cpp_out=%path_cpp% *.proto
if errorlevel 1 goto fail
"%PROTOC%" -I . --csharp_out=%path_csharp% *.proto
if errorlevel 1 goto fail

rem ---- 서버/클라이언트로 복사 ----
echo 서버로 복사: %SERVER_PROTO%
copy /Y %path_cpp%\*.pb.h  "%SERVER_PROTO%\" > nul || goto fail
copy /Y %path_cpp%\*.pb.cc "%SERVER_PROTO%\" > nul || goto fail
echo 클라이언트로 복사: %CLIENT_PROTO%
copy /Y %path_cpp%\*.pb.h  "%CLIENT_PROTO%\" > nul || goto fail
copy /Y %path_cpp%\*.pb.cc "%CLIENT_PROTO%\" > nul || goto fail

echo 완료: 생성 + 서버/클라이언트 복사
endlocal
pause
exit /b 0

:fail
echo [실패] 생성 또는 복사가 실패했습니다. 서버/클라이언트 파일은 바뀌지 않았거나 일부만 바뀌었을 수 있습니다.
endlocal
pause
exit /b 1
