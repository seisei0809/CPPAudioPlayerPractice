#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <cmath>
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

    // フレーム数を取得
    ma_uint64 frameCount;
    ma_decoder_get_length_in_pcm_frames(decoder, &frameCount);

    // 出力チャンネル数分のバッファ確保
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

    // OpenALバッファに格納
    alGenBuffers(1, buffer);
    alBufferData(*buffer, 
        AL_FORMAT_MONO_FLOAT32,
        monoData.data(),
        static_cast<ALsizei>(monoData.size() * sizeof(float)), 
        decoder->outputSampleRate);

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

    // 音声データ読み込み準備
    ALuint buffer, source;
    ma_decoder decoder;
    string filePath;

    cout << "ファイルパスを入力してください -> ";
    cin >> filePath;

    // ファイルのPCM変換とOpenALバッファ化
    if (!LoadAudioToOpenAL(filePath.c_str(), &buffer, &decoder)) {
        cout << "読み込み失敗\n";
        return -1;
    }

    // 音源生成とプロパティ設定
    alGenSources(1, &source);
    alSourcei(source, AL_BUFFER, buffer);
    alSourcei(source, AL_LOOPING, AL_TRUE);  
    alSourcef(source, AL_GAIN, 1.0f);        

    // リスナーの位置・向き・速度を設定（静止）
    ALfloat listenerPos[] = { 0.0f, 0.0f, 0.0f };
    ALfloat listenerVel[] = { 0.0f, 0.0f, 0.0f };
    ALfloat listenerOri[] = { 0.0f, 0.0f, -1.0f,  
                              0.0f, 1.0f,  0.0f }; 
    alListenerfv(AL_POSITION, listenerPos);
    alListenerfv(AL_VELOCITY, listenerVel);
    alListenerfv(AL_ORIENTATION, listenerOri);

    // ドップラー効果の強さと音速を設定
    alDopplerFactor(1.0f);         // 効果の倍率
    alDopplerVelocity(343.0f);     // 空気中の音速

    // 音源の初期位置
    ALfloat sourcePos[3] = { 0.1f, 0, 10 };
    ALfloat velocity[3] = { 0, 0, -1 };

    alSourcefv(source, AL_POSITION, sourcePos);
    alSourcefv(source, AL_VELOCITY, velocity);

    // 音源再生開始
    alSourcePlay(source);

    // 20秒間、毎フレーム音源を前進させる
    int durationMs = 20000;      // 合計時間
    int intervalMs = 16;         // フレーム間隔
    int steps = durationMs / intervalMs;

    for (int i = 0; i < steps; ++i) {
        for (int j = 0; j < 3; j++) {
            sourcePos[j] += velocity[j] * (intervalMs / 1000.0f);
        }
        alSource3f(source, AL_POSITION, sourcePos[0], sourcePos[1], sourcePos[2]);
        this_thread::sleep_for(chrono::milliseconds(intervalMs));
    }

    // クリーンアップ
    alSourceStop(source);
    alDeleteSources(1, &source);
    alDeleteBuffers(1, &buffer);
    ma_decoder_uninit(&decoder);
    alcMakeContextCurrent(nullptr);
    alcDestroyContext(context);
    alcCloseDevice(device);

    return 0;
}
