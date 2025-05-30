#include <iostream>
#include <vector>
#include <string>
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

using namespace std;

/// <summary>
/// MiniAudioで音声ファイルをデコードし、OpenAL用バッファに転送する
/// モノラル化も行っており、3D定位に必要なフォーマットへ変換している
/// </summary>
bool LoadAudioToOpenAL(const char* filename, ALuint* buffer, ma_decoder* decoder) {
    ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 0, 0);
    if (ma_decoder_init_file(filename, &config, decoder) != MA_SUCCESS) {
        cout << "音声ファイル読み込み失敗\n";
        return false;
    }

    ma_uint64 frameCount;
    ma_decoder_get_length_in_pcm_frames(decoder, &frameCount);

    vector<float> pcmData(frameCount * decoder->outputChannels);
    ma_uint64 framesRead;
    ma_decoder_read_pcm_frames(decoder, pcmData.data(), frameCount, &framesRead);

    // 多チャンネル音声をモノラルに平均化して変換
    vector<float> monoData(frameCount);
    for (ma_uint64 i = 0; i < frameCount; ++i) {
        float sum = 0.0f;
        for (ma_uint32 ch = 0; ch < decoder->outputChannels; ++ch) {
            sum += pcmData[i * decoder->outputChannels + ch];
        }
        monoData[i] = sum / decoder->outputChannels;
    }

    // OpenALバッファへ転送
    alGenBuffers(1, buffer);
    alBufferData(*buffer, AL_FORMAT_MONO_FLOAT32, monoData.data(), static_cast<ALsizei>(monoData.size() * sizeof(float)), decoder->outputSampleRate);

    return true;
}

int main() {

    // HRTF有効化
    ALCint attr[] = {
    ALC_HRTF_SOFT, ALC_TRUE,
    };

    // OpenAL 初期化（デバイス＆コンテキスト）
    ALCdevice* device = alcOpenDevice(nullptr);
    ALCcontext* context = alcCreateContext(device, attr);
    alcMakeContextCurrent(context);

    ALuint buffer, source;
    ma_decoder decoder;
    float volume = 1.0f;
    bool isEnd = false;
    string filePath;

Start:
    cout << "ファイルパスを入力してください -> ";
    cin >> filePath;

    // MiniAudioでPCMデコードし、OpenALバッファ化
    if (!LoadAudioToOpenAL(filePath.c_str(), &buffer, &decoder)) {
        cout << "読み込み失敗\n";
        return -1;
    }

    // OpenALソース生成と再生属性の設定
    alGenSources(1, &source);
    alSourcei(source, AL_BUFFER, buffer);
    alSourcef(source, AL_GAIN, volume);

    while (!isEnd) {
        unsigned short cmd;

        while (true) {
            cout <<
                "オプションを選んでください\n"
                "1   -> 再生\n"
                "2   -> 停止\n"
                "3   -> 再生設定\n"
                "4   -> 位置設定\n"
                "5   -> ファイルパス再入力\n"
                "0   -> 終了\n"
                "cmd -> ";

            cin >> cmd;
            if (!cin.fail()) break;

            cout << "整数型を入力してください\n";
            cin.clear(); cin.ignore(1000, '\n');
        }

        switch (cmd) {
        case 1:
            alSourcePlay(source);
            break;
        case 2:
            alSourceStop(source);
            break;
        case 3: {
            unsigned short loopFlag;
            while (true) {
                cout << "ループ(0:OFF 1:ON) -> ";
                cin >> loopFlag;
                if (!cin.fail() && (loopFlag == 0 || loopFlag == 1)) break;
                cout << "0か1を入力してください\n";
                cin.clear(); cin.ignore(1000, '\n');
            }
            alSourcei(source, AL_LOOPING, loopFlag ? AL_TRUE : AL_FALSE);

            unsigned short volume;
            while (true) {
                cout << "音量(0-100) -> ";
                cin >> volume;
                if (!cin.fail() && volume <= 100 && volume >= 0) break;
                cout << "0〜100の整数を入力してください\n";
                cin.clear(); cin.ignore(1000, '\n');
            }
            volume = volume * 0.01f;
            alSourcef(source, AL_GAIN, volume);
            break;
        }
        case 4: {
            ALfloat listenerPos[3];
            ALfloat soundPos[3];

            cout << "リスナーの位置設置\n";
            for (int i = 0; i < 3; i++) {
                cout << static_cast<char>('x' + i) << " -> ";
                cin >> listenerPos[i];
                if (cin.fail()) { cin.clear(); cin.ignore(1000, '\n'); i--; }
            }
            alListenerfv(AL_POSITION, listenerPos);

            cout << "音の位置設置\n";
            for (int i = 0; i < 3; i++) {
                cout << static_cast<char>('x' + i) << " -> ";
                cin >> soundPos[i];
                if (cin.fail()) { cin.clear(); cin.ignore(1000, '\n'); i--; }
            }
            alSourcefv(source, AL_POSITION, soundPos);
            break;
        }
        case 5:
            alSourceStop(source);
            alDeleteSources(1, &source);
            alDeleteBuffers(1, &buffer);
            ma_decoder_uninit(&decoder);
            goto Start;
        case 0:
            isEnd = true;
            break;
        default:
            cout << "正しい番号を入力してください\n";
            break;
        }
    }

    // リソース解放
    alSourceStop(source);
    alDeleteSources(1, &source);
    alDeleteBuffers(1, &buffer);
    ma_decoder_uninit(&decoder);
    alcMakeContextCurrent(nullptr);
    alcDestroyContext(context);
    alcCloseDevice(device);

    return 0;
}
