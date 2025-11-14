#include <iostream>
#include <string>
#include <curl/curl.h>
#include "json.hpp"
#include <ctime>
#include <vector>
#include <algorithm>

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
    CURL* curl = curl_easy_init();
    string readBuffer;

    if (curl) {
        string serviceKey = "2db1394a40efd87fd4c968de7304e9f1b2620c7a46664780b2aa49c43de35716";
        char* encodedKey = curl_easy_escape(curl, serviceKey.c_str(), serviceKey.length());

        string url =
          "https://api.odcloud.kr/api/3033376/v1/uddi:b5aaee53-d3a4-4f1a-9fad-da05c77ca049"
          "?page=1&perPage=200&returnType=JSON&serviceKey=" + string(encodedKey);

        curl_free(encodedKey);

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
        }
        curl_easy_cleanup(curl);
    }

    return readBuffer;
}

int main() {
    string jsonStr = getAPIJson();

    if (jsonStr.rfind("<html", 0) == 0) {
        cerr << "응답이 HTML입니다! JSON이 아닙니다." << endl;
        cerr << "=== Response ===\n" << jsonStr << endl;
        return 1;
    }

    try {
        json j = json::parse(jsonStr);
        auto data = j["data"];

        time_t now = time(NULL);
        tm* lt = localtime(&now);
        int w = lt->tm_wday;

        // JSON 구조에 맞게 todayType 설정
        string todayType;
        if (w == 0) todayType = "공휴일";
        else if (w == 6) todayType = "토요일";
        else todayType = "평일(상)"; // 평일(상) 또는 평일(하) 확인 필요

        cout << "오늘 요일: " << todayType << endl;

        int nowMin = lt->tm_hour * 60 + lt->tm_min;
        vector<int> arrivalTimes;

        for (auto& row : data) {
            if (row["역명"] == "두류" && row["요일별"] == todayType) {
                for (auto it = row.begin(); it != row.end(); ++it) {
                    string key = it.key();
                    if (key == "역명" || key == "요일별" || key == "구분") continue;

                    if (!it.value().is_null()) {
                        string val = it.value().get<string>();
                        if (!val.empty()) {
                            // HH:MM:SS -> HH:MM로 변환
                            int t = timeToMin(val.substr(0,5));
                            if (t >= nowMin)
                                arrivalTimes.push_back(t);
                        }
                    }
                }
            }
        }

        sort(arrivalTimes.begin(), arrivalTimes.end());

        cout << "두류역 다음 열차 3개 (몇 분 후 도착):" << endl;
        for (int i = 0; i < 3 && i < arrivalTimes.size(); i++) {
            int diff = arrivalTimes[i] - nowMin;
            cout << " - " << diff << "분 후" << endl;
        }

    } catch (json::parse_error& e) {
        cerr << "JSON 파싱 실패: " << e.what() << endl;
        cerr << "=== Response ===\n" << jsonStr << endl;
        return 1;
    }

    return 0;
}
