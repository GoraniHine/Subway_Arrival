#define SERVO 2

#include <iostream>
#include <wiringPi.h>
#include <softPwm.h>
#include <string>
#include <curl/curl.h>
#include "json.hpp"
#include <ctime>
#include <vector>
#include <algorithm>
#include <locale.h>
#include <ncurses.h>
#include <unistd.h> // sleep
#include <thread> // thread
#include <atomic>

using namespace std;
using json = nlohmann::json;

int servoPos = 5;

atomic<bool> trainArrived(false); // 기차 도착 플래그
atomic<bool> running(true); // 종료를 위한 플래그
json trainData;
string todayType;

class dist{
    private:
        float result, s, e;
    public:
        void measures(float i);
        void measuree(float j);
        float calc();
};

void dist::measures(float i)
{
    s = i;
}

void dist::measuree(float j)
{
    e = j;
}

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
int timeToMin(const string& t);
string getAPIJson();
int servorControlUp();
int servorControlDown();
void runUI();
void runServo();

int main(void)
{
    if(wiringPiSetup() == -1)
    {
        return -1;
    }

    softPwmCreate(SERVO, 0, 200);

    pinMode(0, OUTPUT);
    pinMode(1, INPUT);

    string jsonStr = getAPIJson(); // getAPIJson() → API 호출 후 받은 문자열

    setlocale(LC_ALL, ""); // 로케일 설정
    initscr();             // ncurses 시작
    noecho();              // 입력 문자 표시 안 함
    curs_set(FALSE);       // 커서 숨기기
    start_color(); 

    if (jsonStr.rfind("<html", 0) == 0) {
        cerr << "응답이 HTML입니다! JSON이 아닙니다." << endl;
        cerr << "=== Response ===\n" << jsonStr << endl;
        return 1;
    }

    try {
        json j = json::parse(jsonStr);
        trainData = j["data"]; // j["data"] → JSON 객체에서 "data" 키에 해당하는 값 추출

        // auto → 타입 자동 추론, 여기서는 nlohmann::json 타입이 됨

        time_t now = time(NULL); // time(NULL) : 현재 시간을 반환
        tm* lt = localtime(&now); // localtime() : 사람이 읽을 수 있는 날짜/시간 구조체(tm)으로 변환
        // localtime은 내부에서 정적 구조체 하나를 만들어 놓고 그 구조체의 주소를 반환함
        // It은 tm 구조체를 가리키는 포인터가 됨
        int w = lt->tm_wday; // tm_wday : 요일을 숫자로 알려줌, w == 0 : 일요일, w == 6 : 토요일

        // JSON 구조에 맞게 todayType 설정
        if (w == 0) todayType = "휴일(상)";
        else if (w == 6) todayType = "토요일(상)";
        else todayType = "평일(상)"; // 평일(상) 또는 평일(하) 확인 필요
    }

    catch (json::parse_error& e) {
        cerr << "JSON 파싱 실패: " << e.what() << endl; // cerr: c++에서 오류 메시지를 표준 에러 스트림으로 버퍼링 없이 출력하는 객체
        cerr << "=== Response ===\n" << jsonStr << endl;
        return 1;
    }

    thread uiThread(runUI); // 스레드 생성과 동시에 함수 실행
    thread servoThread(runServo); // 스레드 생성과 동시에 함수 실행

    getch();     
    running = false;
    
    uiThread.join(); // 스레드가 끝날때 까지 기다림
    servoThread.join(); // 스레드가 끝날때 까지 기다림

    endwin();
    
    return 0;
}

float dist::calc()
{
    result = (e - s) / 58;
    cout << "distance(cm): " << result << "\n";
    return result;
}

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

int timeToMin(const string& t) {
    if (t.empty()) return -1;
    int h = stoi(t.substr(0, 2));
    int m = stoi(t.substr(3, 2));
    return h * 60 + m;
}

string getAPIJson() {
    CURL* curl = curl_easy_init(); // libcurl 객체 만들기
    string readBuffer;

    if (curl) {
        string serviceKey = "2db1394a40efd87fd4c968de7304e9f1b2620c7a46664780b2aa49c43de35716";
        char* encodedKey = curl_easy_escape(curl, serviceKey.c_str(), serviceKey.length());

        string url =
          "https://api.odcloud.kr/api/3033376/v1/uddi:b5aaee53-d3a4-4f1a-9fad-da05c77ca049"
          "?page=1&perPage=200&returnType=JSON&serviceKey=" + string(encodedKey);

        curl_free(encodedKey);

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str()); // 요청 URL 설정
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback); // 응답 저장 함수
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer); // 응답 저장 위치

        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        CURLcode res = curl_easy_perform(curl); // 실제로 요청 보내기
        if (res != CURLE_OK) {
            cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
        }
        curl_easy_cleanup(curl); // 사용 종료
    }

    return readBuffer;
}

void runUI()
{
    while(running)
    {
        time_t now = time(NULL);
        tm* lt = localtime(&now);
        int nowMin = lt->tm_hour * 60 + lt->tm_min;

        // arrivalTimes 계산
        vector<int> arrivalTimes;
        for (auto& row : trainData) {
            if (row["역명"] == "감삼" && row["요일별"] == todayType) 
            {
                for (auto it = row.begin(); it != row.end(); ++it) 
                {
                    string key = it.key();
                    if (key == "역명" || key == "요일별" || key == "구분") continue;
                    
                    if (!it.value().is_null()) 
                    {
                        string val = it.value().get<string>();
                        if (!val.empty()) 
                        {
                            int t = timeToMin(val.substr(0,5));
                            if (t >= nowMin)
                                arrivalTimes.push_back(t);
                        }
                    }
                }
            }
        }

        sort(arrivalTimes.begin(), arrivalTimes.end());

        // cout << "감삼역 다음 열차 3개 (몇 분 후 도착):" << endl;
        const char* title = "감삼역 다음 열차 3개 (몇 분 후 도착):";
        int titleX = (COLS - strlen(title)) / 2;  // COLS는 ncurses 전체 화면 가로
        mvprintw(0, titleX, "%s", title);

        /*
        if (!arrivalTimes.empty()) // 맨 처음 기차만
        {
            int diff = arrivalTimes[0] - nowMin;
            trainArrived = (diff <= 0);
        }
        */
        
        // 과거 열차 제거 (0분 제외)
        while (!arrivalTimes.empty() && arrivalTimes[0] < nowMin)
        {
            arrivalTimes.erase(arrivalTimes.begin());
        }    

        // 가장 가까운 열차 기준으로 trainArrived 판단
        trainArrived = !arrivalTimes.empty() && (arrivalTimes[0] - nowMin <= 0);

        for (int i = 0; i < 3 && i < arrivalTimes.size(); i++) {
            int diff = arrivalTimes[i] - nowMin;

            char buffer[50];
            sprintf(buffer, "- %d분 후 도착", diff);

            // 중앙 좌표 계산
            int x = (COLS - strlen(buffer)) / 2;

            // 출력 (i+2행)
            mvprintw(i + 2, x, "%s", buffer);
        }        
        
        // (OPTIONAL) 첫 번째 열차 위치 5칸 지도
        int diff = arrivalTimes[0] - nowMin;

        int pos = diff / 2;
        if (pos > 4) pos = 4;
        pos = 4 - pos;

        char line[20];
        for (int i = 0; i < 5; i++) {
            if (i == pos) line[0 + i * 2] = '*';
            else line[0 + i * 2] = '-';
        }
        line[1] = ' '; line[3] = ' '; line[5] = ' '; line[7] = ' ';
        line[9] = '\0';

        int mapX = (COLS - strlen(line)) / 2;
        mvprintw(6, mapX, "%s", line);
        refresh(); // 갱신
        sleep(15);
    }
}

void runServo()
{
    dist sonic;

    static time_t lastServoTime = 0;        // 마지막 서보 동작 시간
    const int servoCooldown = 20;           // 초 단위, 서보 최소 재동작 시간

    while(running)
    {
        digitalWrite(0, 0); // 초음파 측정
        delayMicroseconds(2);
        digitalWrite(0, 1);
        delayMicroseconds(10);
        digitalWrite(0, 0);   

        while(digitalRead(1) == 0) sonic.measures(micros());
        while(digitalRead(1) == 1) sonic.measuree(micros());

        static bool servoUp = false;
        static time_t servoStartTime = 0;

        time_t currentTime = time(NULL);

        // trainArrived가 새로 true가 되고, 거리 조건 만족, 쿨다운 시간 지남
        if(trainArrived && !servoUp && sonic.calc() > 30 && difftime(currentTime, lastServoTime) > servoCooldown)
        {
            servorControlUp();
            servoUp = true;
            servoStartTime = currentTime;
            lastServoTime = currentTime; // 마지막 동작 시간 기록
        }
        
        // 10초 지나면 내려가기
        if(servoUp && difftime(currentTime, servoStartTime) >= 10)
        {
            servorControlDown();
            servoUp = false;
        }

        delay(100);
    }
}

int servorControlUp()
{
    int dir = -1;
    while(servoPos > 5)
    {
        servoPos += dir;
        softPwmWrite(SERVO, servoPos);
        delay(10);
    }

    return 0;
}

int servorControlDown()
{
    int dir = 1;
    while(servoPos < 14)
    {
        servoPos += dir;
        softPwmWrite(SERVO, servoPos);
        delay(10);
    }

    return 0;
}
