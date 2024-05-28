
마이크로서비스_매뉴얼.docx 참고<br/>
<br/>
<br/>
1.	소프트웨어<br/>
<br/>
마이크로서비스 시험기<br/>
마이크로서비스 장치 에뮬레이터<br/>
	- 마이크로서비스 프로토콜 및 X 4506 프로토콜 지원<br/>
프로토콜 변환기<br/>
	- X 4506 프로토콜 – 마이크로서비스 프로토콜 간 변환<br/>
	- Ethernet – 시리얼 포트 간 X 4506 메시지 전달 모듈 포함<br/>
<br/>
<br/>
2.	개발 환경 및 빌드<br/>
<br/>
2.1.	개발 환경<br/>
<br/>
마이크로서비스 시험기<br/>
	- Windows 11	Microsoft Visual Studio 2017<br/>
마이크로서비스 장치 에뮬레이터<br/>
	- Winndows 11	Microsoft Visual Studio 2017<br/>
	- Ubuntu 20.04	g++ 9.3.0 make 4.2.1<br/>
	- Android 	Android Studio Flamingo Target SDK 23<br/>
프로토콜 변환기<br/>
	- Winndows 11	Microsoft Visual Studio 2017<br/>
	- Ubuntu 20.04	g++ 9.3.0 make 4.2.1<br/>
	- Android 	Android Studio Flamingo Target SDK 23<br/>
<br/>
<br/>
2.2.	빌드<br/>
<br/>
마이크로서비스 시험기<br/>
	- Windows 11	./windows/X4506AuthTool/X4506AuthTool.snl 파일을 열어 X4506AuthTool 프로젝트 빌드<br/>
마이크로서비스 장치 에뮬레이터	<br/>
	- Winndows 11	./windows/X4506AuthTool/X4506AuthTool.snl 파일을 열어 HomeDevice 프로젝트 빌드<br/>
	- Ubuntu 20.04	./linux/app으로 이동하여 make 실행<br/>
	- Android <br/>
		: Phone 장치: ./baresip-studio-55.0.2 폴더를 연 후, 빌드<br/>
		: Phone 이 외의 장치: ./android 폴더를 연 후, 빌드<br/>
프로토콜 변환기<br/>
	- Winndows 11	./windows/X4506AuthTool/X4506AuthTool.snl 파일을 열어 HomeDevice 프로젝트 빌드<br/>
	- Ubuntu 20.04	./linux/app으로 이동하여 make 실행<br/>
	- Android 	./android 폴더를 연 후, 빌드<br/>
