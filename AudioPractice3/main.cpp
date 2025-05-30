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
/// MiniAudio�ŉ����t�@�C�����f�R�[�h���AOpenAL�p�o�b�t�@�ɓ]������
/// ���m���������s���Ă���A3D��ʂɕK�v�ȃt�H�[�}�b�g�֕ϊ����Ă���
/// </summary>
bool LoadAudioToOpenAL(const char* filename, ALuint* buffer, ma_decoder* decoder) {
    ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 0, 0);
    if (ma_decoder_init_file(filename, &config, decoder) != MA_SUCCESS) {
        cout << "�����t�@�C���ǂݍ��ݎ��s\n";
        return false;
    }

    // �t���[�������擾
    ma_uint64 frameCount;
    ma_decoder_get_length_in_pcm_frames(decoder, &frameCount);

    // �o�̓`�����l�������̃o�b�t�@�m��
    vector<float> pcmData(frameCount * decoder->outputChannels);
    ma_uint64 framesRead;
    ma_decoder_read_pcm_frames(decoder, pcmData.data(), frameCount, &framesRead);

    // ���`�����l�����������m�����ɕ��ω����ĕϊ�
    vector<float> monoData(frameCount);
    for (ma_uint64 i = 0; i < frameCount; ++i) {
        float sum = 0.0f;
        for (ma_uint32 ch = 0; ch < decoder->outputChannels; ++ch) {
            sum += pcmData[i * decoder->outputChannels + ch];
        }
        monoData[i] = sum / decoder->outputChannels;
    }

    // OpenAL�o�b�t�@�Ɋi�[
    alGenBuffers(1, buffer);
    alBufferData(*buffer, 
        AL_FORMAT_MONO_FLOAT32,
        monoData.data(),
        static_cast<ALsizei>(monoData.size() * sizeof(float)), 
        decoder->outputSampleRate);

    return true;
}

int main() {

    // HRTF�L����
    ALCint attr[] = {
    ALC_HRTF_SOFT, ALC_TRUE, 
    };

    // OpenAL �������i�f�o�C�X���R���e�L�X�g�j
    ALCdevice* device = alcOpenDevice(nullptr);
    ALCcontext* context = alcCreateContext(device, attr);
    alcMakeContextCurrent(context);

    // �����f�[�^�ǂݍ��ݏ���
    ALuint buffer, source;
    ma_decoder decoder;
    string filePath;

    cout << "�t�@�C���p�X����͂��Ă������� -> ";
    cin >> filePath;

    // �t�@�C����PCM�ϊ���OpenAL�o�b�t�@��
    if (!LoadAudioToOpenAL(filePath.c_str(), &buffer, &decoder)) {
        cout << "�ǂݍ��ݎ��s\n";
        return -1;
    }

    // ���������ƃv���p�e�B�ݒ�
    alGenSources(1, &source);
    alSourcei(source, AL_BUFFER, buffer);
    alSourcei(source, AL_LOOPING, AL_TRUE);  
    alSourcef(source, AL_GAIN, 1.0f);        

    // ���X�i�[�̈ʒu�E�����E���x��ݒ�i�Î~�j
    ALfloat listenerPos[] = { 0.0f, 0.0f, 0.0f };
    ALfloat listenerVel[] = { 0.0f, 0.0f, 0.0f };
    ALfloat listenerOri[] = { 0.0f, 0.0f, -1.0f,  
                              0.0f, 1.0f,  0.0f }; 
    alListenerfv(AL_POSITION, listenerPos);
    alListenerfv(AL_VELOCITY, listenerVel);
    alListenerfv(AL_ORIENTATION, listenerOri);

    // �h�b�v���[���ʂ̋����Ɖ�����ݒ�
    alDopplerFactor(1.0f);         // ���ʂ̔{��
    alDopplerVelocity(343.0f);     // ��C���̉���

    // �����̏����ʒu
    ALfloat sourcePos[3] = { 0.1f, 0, 10 };
    ALfloat velocity[3] = { 0, 0, -1 };

    alSourcefv(source, AL_POSITION, sourcePos);
    alSourcefv(source, AL_VELOCITY, velocity);

    // �����Đ��J�n
    alSourcePlay(source);

    // 20�b�ԁA���t���[��������O�i������
    int durationMs = 20000;      // ���v����
    int intervalMs = 16;         // �t���[���Ԋu
    int steps = durationMs / intervalMs;

    for (int i = 0; i < steps; ++i) {
        for (int j = 0; j < 3; j++) {
            sourcePos[j] += velocity[j] * (intervalMs / 1000.0f);
        }
        alSource3f(source, AL_POSITION, sourcePos[0], sourcePos[1], sourcePos[2]);
        this_thread::sleep_for(chrono::milliseconds(intervalMs));
    }

    // �N���[���A�b�v
    alSourceStop(source);
    alDeleteSources(1, &source);
    alDeleteBuffers(1, &buffer);
    ma_decoder_uninit(&decoder);
    alcMakeContextCurrent(nullptr);
    alcDestroyContext(context);
    alcCloseDevice(device);

    return 0;
}
