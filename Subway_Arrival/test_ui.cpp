#include <locale.h>
#include <ncurses.h>
#include <unistd.h> // sleep
#include <string>

int main() {
    setlocale(LC_ALL, ""); // 로케일 설정
    initscr();             // ncurses 시작
    noecho();              // 입력 문자 표시 안 함
    curs_set(FALSE);       // 커서 숨기기
    start_color(); 

    // LED 상태별 글자색 초기화 (배경은 기본 터미널 색상)
    init_pair(1, COLOR_GREEN, COLOR_BLACK);  
    init_pair(2, COLOR_YELLOW, COLOR_BLACK); 
    init_pair(3, COLOR_RED, COLOR_BLACK);    

    int minutes_left[] = {1, 3, 5, 2, 0}; // 예시 도착 시간 배열 (분)
    int n = sizeof(minutes_left)/sizeof(minutes_left[0]);

    for(int i = 0; i < n; i++) {
        clear(); // 화면 초기화

        // 도착 정보 출력
        mvprintw(0, 0, "강남역 도착 정보: %d분 후 도착", minutes_left[i]);

        // LED 상태 결정
        int color_pair;
        std::string led_status;

        if(minutes_left[i] <= 1) {
            color_pair = 1; // 초록
            led_status = "초록";
        } else if(minutes_left[i] <= 5) {
            color_pair = 2; // 노랑
            led_status = "노랑";
        } else {
            color_pair = 3; // 빨강
            led_status = "빨강";
        }

        // 글자색만 적용
        attron(COLOR_PAIR(color_pair));
        mvprintw(2, 0, "LED 상태: %s", led_status.c_str());
        attroff(COLOR_PAIR(color_pair));

        refresh();
        sleep(1);
    }

    mvprintw(4, 0, "끝났습니다. 아무 키나 누르세요.");
    refresh();
    getch();           
    endwin();
    return 0;
}
