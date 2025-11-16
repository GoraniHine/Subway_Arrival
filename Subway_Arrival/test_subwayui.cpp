#include <iostream>
#include <string>
#include <curl/curl.h>
#include "json.hpp"
#include <ctime>
#include <vector>
#include <algorithm>
#include <locale.h>
#include <ncurses.h>
#include <unistd.h> // sleep
#include <string>

using json = nlohmann::json;
using namespace std;

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

int main() {
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
        auto data = j["data"]; // j["data"] → JSON 객체에서 "data" 키에 해당하는 값 추출

        // auto → 타입 자동 추론, 여기서는 nlohmann::json 타입이 됨

        time_t now = time(NULL); // time(NULL) : 현재 시간을 반환
        tm* lt = localtime(&now); // localtime() : 사람이 읽을 수 있는 날짜/시간 구조체(tm)으로 변환
        // localtime은 내부에서 정적 구조체 하나를 만들어 놓고 그 구조체의 주소를 반환함
        // It은 tm 구조체를 가리키는 포인터가 됨
        int w = lt->tm_wday; // tm_wday : 요일을 숫자로 알려줌, w == 0 : 일요일, w == 6 : 토요일

        // JSON 구조에 맞게 todayType 설정
        string todayType;
        if (w == 0) todayType = "휴일(상)";
        else if (w == 6) todayType = "토요일(상)";
        else todayType = "평일(상)"; // 평일(상) 또는 평일(하) 확인 필요

        cout << "오늘 요일: " << todayType << endl;

        int nowMin = lt->tm_hour * 60 + lt->tm_min;
        vector<int> arrivalTimes; // vector<int>는 정수형 데이터를 여러 개 저장할 수 있는 동적 배열

        for (auto& row : data) {
            if (row["역명"] == "두류" && row["요일별"] == todayType) {
                for (auto it = row.begin(); it != row.end(); ++it) {
                    string key = it.key();
                    if (key == "역명" || key == "요일별" || key == "구분") continue;

                    if (!it.value().is_null()) {
                        string val = it.value().get<string>();
                        if (!val.empty()) {
                            // HH:MM:SS -> HH:MM로 변환
                            int t = timeToMin(val.substr(0,5)); // val → 예를 들어 "12:34" 같은 시간 문자열, substr(0,5) → 0번째 인덱스부터 5글자 추출
                            if (t >= nowMin) // 지금 이후에 도착하는 열차만 선별
                                arrivalTimes.push_back(t); // 조건을 만족하면 arrivalTimes 벡터에 추가
                        }
                    }
                }
            }
        }

        sort(arrivalTimes.begin(), arrivalTimes.end());

        cout << "두류역 다음 열차 3개 (몇 분 후 도착):" << endl;
        mvprintw(0, 0, "두류역 다음 열차 3개 (몇 분 후 도착):");
        for (int i = 0; i < 3 && i < arrivalTimes.size(); i++) {
            int diff = arrivalTimes[i] - nowMin;
            cout << " - " << diff << "분 후" << endl;
            mvprintw(i + 2, 0, "- %d분 후 도착", diff);
        }        

    } catch (json::parse_error& e) {
        cerr << "JSON 파싱 실패: " << e.what() << endl; // cerr: c++에서 오류 메시지를 표준 에러 스트림으로 버퍼링 없이 출력하는 객체
        cerr << "=== Response ===\n" << jsonStr << endl;
        return 1;
    }

    refresh();
    getch();           
    endwin();
    
    return 0;
}
