#include <iostream>
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

using std::cout;
using std::cin;

int main() {

    ma_result result;  
    ma_engine engine;  
    std::string filePath;
    ma_sound sound;    
    bool isEnd = false;

    // オーディオエンジン設定（7.1ch）
    ma_engine_config config = ma_engine_config_init();
    config.channels = 8;  

    // エンジン初期化
    result = ma_engine_init(&config, &engine);  
    if (result != MA_SUCCESS) {
        cout << "本体の初期化に失敗したので強制終了\n";
        cout << "スペースキーを押して終了...\n";
        return -1;
    }

    while (!isEnd) {
    Start:
        cout << "ファイルパスを入力してください -> ";
        cin >> filePath;

        // ファイルからサウンドを読み込む
        result = ma_sound_init_from_file(&engine,
            filePath.c_str(), 0, nullptr, nullptr, &sound);

        if (result != MA_SUCCESS) {
            cout << "音の読み込みができませんでした\n";
            break;
        }

        // リスナーと音源の初期位置を原点に設定
        ma_engine_listener_set_position(&engine, 0, 0.0f, 0.0f, 0.0f);
        ma_sound_set_position(&sound, 0.0f, 0.0f, 0.0f);

        while (!isEnd) {
            unsigned short cmd;

            // ユーザー入力によるコマンド選択
            while (true) {
                cout << "オプションを選んでください\n"
                    << "1   -> 再生\n"
                    << "2   -> 停止\n"
                    << "3   -> 再生設定\n"
                    << "4   -> 位置設定\n"
                    << "5   -> ファイルパス再入力\n"
                    << "0   -> 終了\n"
                    << "cmd -> ";

                cin >> cmd;
                if (!cin.fail()) break;

                cout << "整数型を入力してください\n";
                cin.clear(); cin.ignore(1000, '\n');
            }

            switch (cmd) {
            case 1:
                ma_sound_seek_to_pcm_frame(&sound, 0); 
                ma_sound_start(&sound);               
                break;
            case 2:
                ma_sound_stop(&sound);               
                break;
            case 3: {
                // 再生設定（ループと音量）
                while (true) {
                    cout << "ループ設定(0 -> OFF,1 -> ON) -> ";
                    cin >> cmd;
                    if (!cin.fail() && (cmd == 0 || cmd == 1)) break;
                    cout << "0か1を入力してください\n";
                    cin.clear(); cin.ignore(1000, '\n');
                }
                ma_sound_set_looping(&sound, cmd);

                unsigned short volume;
                while (true) {
                    cout << "音量設定(0〜100) -> ";
                    cin >> volume;
                    if (!cin.fail() && volume <= 100 && volume >=0) break;
                    cout << "0〜100の整数を入力してください\n";
                    cin.clear(); cin.ignore(1000, '\n');
                }
                ma_sound_set_volume(&sound, static_cast<float>(volume) * 0.01f);
                break;
            }
            case 4: {
                // 位置設定
                float listenerPos[3];
                float soundPos[3];
                cout << "リスナーの位置設置\n";
                for (int i = 0; i < 3; i++) {
                    cout << static_cast<char>('x' + i) << " -> ";
                    cin >> listenerPos[i];
                    if (cin.fail()) { cin.clear(); cin.ignore(1000, '\n'); i--; }
                }
                ma_engine_listener_set_position(&engine, 0, listenerPos[0], listenerPos[1], listenerPos[2]);

                cout << "音の位置設置\n";
                for (int i = 0; i < 3; i++) {
                    cout << static_cast<char>('x' + i) << " -> ";
                    cin >> soundPos[i];
                    if (cin.fail()) { cin.clear(); cin.ignore(1000, '\n'); i--; }
                }
                ma_sound_set_position(&sound, soundPos[0], soundPos[1], soundPos[2]);
                break;
            }
            case 5:
                // 音を解放して再入力へ
                ma_sound_uninit(&sound);  
                goto Start;
            case 0:
                isEnd = true;
                ma_sound_uninit(&sound);
                break;
            default:
                cout << "オプション番号以外を入力しないでください\n";
                break;
            }
        }
    }

    cout << "スペースキーを押して終了...\n";
    while (true) {
        if (GetAsyncKeyState(VK_SPACE) & 0x8000) break;
    }

    ma_engine_uninit(&engine); 
    return 0;
}
