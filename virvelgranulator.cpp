#include "daisy_patch.h"
#include "daisysp.h"
#include "granulator.h"
#include <cmath>

daisy::DaisyPatch patch;
constexpr uint32_t BUFFER_SIZE = 8 * 1024 * 1024;
dsp::Granulator gran;

enum DISPLAYVALS
{
    GRAIN1,
    GRAIN2
};

DISPLAYVALS display = GRAIN1;

uint32_t n = 0;
bool gate = false;
uint32_t ACTUAL_DURATION = 0;

float ctrls[4] = {0.f, 0.f, 0.f, 0.f};

DSY_SDRAM_BSS float buffer[BUFFER_SIZE];

static daisy::SdmmcHandler sdcard;
daisy::FatFSInterface fsi;
FIL SDFile;

const int maxFiles = 1;
daisy::WavFileInfo m_file_info_[maxFiles];
int m_file_sizes[maxFiles];

struct waveFile {
    uint16_t format;
    uint16_t channels;
    uint32_t samplerate;
    uint16_t bitrate;
    uint32_t num_samples;
    uint16_t header_size;
};

waveFile parseHeader(FIL * file) {
    waveFile wf;

    uint32_t res;
    UINT bytesread;
    bool dataFound = false;    

    while (!dataFound) {

        f_read(file, &res, sizeof(uint32_t), &bytesread);

        switch (res) {
            case 0x20746D66: {
                    f_read(file, &res, 4, &bytesread);
                    f_read(file, &wf.format, sizeof(uint16_t), &bytesread);
                    f_read(file, &wf.channels, sizeof(uint16_t), &bytesread);
                    f_read(file, &wf.samplerate, sizeof(uint32_t), &bytesread);
                    f_read(file, &res, sizeof(uint32_t), &bytesread);
                    f_read(file, &res, sizeof(uint16_t), &bytesread);
                    f_read(file, &wf.bitrate, sizeof(uint16_t), &bytesread);
                    break;
                }
            case 0x61746164: {
                    f_read(file, &wf.num_samples, sizeof(uint32_t), &bytesread);
                    wf.num_samples = wf.num_samples / wf.channels/ 4;
                    dataFound = true;
                    break;
                }
            default:
                break;
        }

    }
 
    wf.header_size = f_tell(file);
    return wf;

}

void AudioCallback(daisy::AudioHandle::InputBuffer in, daisy::AudioHandle::OutputBuffer out, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        out[0][i] = 0.f;
        out[1][i] = 0.f;
        out[2][i] = 0.f;
        out[3][i] = 0.f;
    }

    float granL, granR;
    for (size_t i = 0; i < size; ++i)
    {
        if (gate)
        {
            buffer[n] = in[0][i];
            buffer[BUFFER_SIZE / 2 + n] = in[1][i];
        }
        n = (n + 1) % ACTUAL_DURATION;
        granL = 0.f;
        granR = 0.f;
        gran.play(&granL, &granR);
        out[0][i] = ctrls[3] * granL;
        out[1][i] = ctrls[3] * granR;
    }
    
}

void loadWavFiles()
{
    std::string m_sd_debug_msg = "no (fat32) sdcard";
    int m_file_cnt_ = 0;
    FRESULT result = FR_OK;
    FILINFO fno;
    DIR dir;
    char *fn;

    FATFS &fs = fsi.GetSDFileSystem();
    f_mount(&fs, "/", 1);

    if (f_opendir(&dir, "/") == FR_OK)
    {
        m_sd_debug_msg = "sd: no files";
        do
        {
            result = f_readdir(&dir, &fno);
            // Exit if bad read or NULL fname
            if (result != FR_OK || fno.fname[0] == 0)
                break;
            // Skip if its a directory or a hidden file.
            if (fno.fattrib & (AM_HID | AM_DIR))
                continue;
            // Now we'll check if its .wav and add to the list.
            fn = fno.fname;

            if (m_file_cnt_ < maxFiles)
            {
                if (strstr(fn, ".wav") || strstr(fn, ".WAV"))
                {
                    strcpy(m_file_info_[m_file_cnt_].name, fn);

                    UINT bytesread;
                    if (f_open(&SDFile, m_file_info_[m_file_cnt_].name, (FA_OPEN_EXISTING | FA_READ)) == FR_OK)
                    {
                        m_file_sizes[m_file_cnt_] = f_size(&SDFile);

                        waveFile wf = parseHeader(&SDFile);

                        if (wf.bitrate != 32)
                            continue;
                        if (wf.samplerate != 48000)
                            continue;
                        if (wf.channels != 2)
                            continue;
                        if (wf.format != 3)
                            continue;
                        
                        uint32_t size = m_file_sizes[m_file_cnt_] - wf.header_size;
                        ACTUAL_DURATION = size / ( wf.channels * sizeof(float));

                        if (&buffer[0])
                        {
                            if (f_read(&SDFile, &buffer[0], size, &bytesread) == FR_OK)
                            {
                                gran.setNumSamples(size / 8);
                                uint32_t mid = BUFFER_SIZE / 2;
                                uint32_t i, j;
                                for (i = 1; i < BUFFER_SIZE; ++i)
                                {
                                    j = i < mid ? i * 2 : ((i - mid) * 2 + 1);
                                    while (j < i)
                                    {
                                        j = j < mid ? j * 2 : ((j - mid) * 2 + 1);
                                    }
                                    float tmp = buffer[i];
                                    buffer[i] = buffer[j];
                                    buffer[j] = tmp;
                                }

                                m_file_cnt_++;
                            }
                        }
                        f_close(&SDFile);
                    }
                }
            }
            else
            {
                break;
            }
        } while (result == FR_OK);
    }
    f_closedir(&dir);
}

void UpdateControls()
{

    patch.ProcessAllControls();

    gate = patch.gate_input[0].State();

    display = DISPLAYVALS((display + (int(patch.encoder.Increment()))) % 3);

    float ctrl0 = patch.GetKnobValue((daisy::DaisyPatch::Ctrl)0);
    float ctrl1 = patch.GetKnobValue((daisy::DaisyPatch::Ctrl)1);
    float ctrl2 = patch.GetKnobValue((daisy::DaisyPatch::Ctrl)2);
    float ctrl3 = patch.GetKnobValue((daisy::DaisyPatch::Ctrl)3);

    if (abs(ctrls[0] - ctrl0) > 0.001) {
        if (ctrl0 < 0.0001)
            ctrl0 = 0.f;
        ctrls[0] = ctrl0;
    }
    if (abs(ctrls[1] - ctrl1) > 0.001) {
        if (ctrl1 < 0.0001)
            ctrl1 = 0.f;
        ctrls[1] = ctrl1;
    }
    
    if (abs(ctrls[2] - ctrl2) > 0.001) {
        if (ctrl2 < 0.0001)
            ctrl2 = 0.f;
        ctrls[2] = ctrl2;
    }

    if (abs(ctrls[3] - ctrl3) > 0.001) {
        if (ctrl3 < 0.0001)
            ctrl3 = 0.f;
        ctrls[3] = ctrl3;
    }
    
    switch (display) {
        case GRAIN1:
        {
            gran.setRate(ctrls[0]);
            gran.setOffset(ctrls[1]);
            gran.setDuration(ctrls[2]);
            
            break;
        }
        case GRAIN2:
        {
            gran.setJitter(ctrls[0]);
            break;
        }

        default:
            break;
        }
}

void UpdateOled()
{

    patch.display.Fill(false);


        float g = ctrls[1] * ACTUAL_DURATION;
        float f = ((ACTUAL_DURATION-g)/128 - 1) * ctrls[2] + 1;

        g = daisysp::fmin(ACTUAL_DURATION-128, g);
        f = daisysp::fmax(1.f, f);
        int prev = 32, curr;
        for (uint32_t i = 0; i < 128; i++)
        {
            curr = -buffer[int(i*f) + int(g)]*20 + 44;
            patch.display.DrawLine(i, prev, i, curr, true);
            prev = curr;
        }

    switch (display)
    {

    case GRAIN1:
    {

        patch.display.SetCursor(0, 0);
        std::string str = "Yoyo";
        char *cstr = &str[0];
        patch.display.WriteString(cstr, Font_6x8, true);

        patch.display.SetCursor(0, 15);
        str = std::to_string(int(ctrls[0] * 100.f)) + "%";
        patch.display.WriteString(cstr, Font_6x8, true);

        patch.display.SetCursor(30, 15);
        str = std::to_string(int(ctrls[1] * 100.f));
        patch.display.WriteString(cstr, Font_6x8, true);

        patch.display.SetCursor(70, 15);
        str = std::to_string(int(ctrls[2] * 100.f));
        patch.display.WriteString(cstr, Font_6x8, true);

        patch.display.SetCursor(70, 0);
        str = std::to_string(int(ctrls[3] * 100.f));
        patch.display.WriteString(cstr, Font_6x8, true);

        break;
    }
    case GRAIN2:
    {
        patch.display.SetCursor(0, 0);
        std::string str = "GRAIN2";
        char *cstr = &str[0];
        patch.display.WriteString(cstr, Font_6x8, true);

        patch.display.SetCursor(0, 15);
        str = std::to_string(int(ctrls[0] * 100.f));
        patch.display.WriteString(cstr, Font_6x8, true);

        break;
    }
/*/    case 0:
    {
        patch.display.Fill(false);
        patch.display.SetCursor(0, 0);
        std::string str = "FX";
        char *cstr = &str[0];
        patch.display.WriteString(cstr, Font_6x8, true);

        patch.display.SetCursor(0, 20);
        str = std::to_string(int(ctrls[0] * 100.f));
        patch.display.WriteString(cstr, Font_6x8, true);

        patch.display.SetCursor(0, 40);
        str = std::to_string(int(ctrls[1] * 100.f));
        patch.display.WriteString(cstr, Font_6x8, true);

        break;
    }
    */

    default:
        break;
    }

    patch.display.Update();
}

int main(void){
    patch.Init();

    patch.display.Fill(false);
    patch.display.Update();

    patch.controls[0].SetCoeff(0.5);
    patch.controls[1].SetCoeff(0.5);
    patch.controls[2].SetCoeff(0.5);
    patch.controls[3].SetCoeff(0.5);

    patch.SetAudioBlockSize(48);
    patch.SetAudioSampleRate(daisy::SaiHandle::Config::SampleRate::SAI_48KHZ);

    gran.init(&buffer[0], BUFFER_SIZE);
    std::fill(&buffer[0], &buffer[BUFFER_SIZE - 1], 0.f);

    //rev.Init(48000);
    //rev.SetFeedback(0.5);
    //rev.SetLpFreq(5000);

    daisy::SdmmcHandler::Config sd_cfg;
    sd_cfg.speed = daisy::SdmmcHandler::Speed::MEDIUM_SLOW;
    sd_cfg.width = daisy::SdmmcHandler::BusWidth::BITS_4;
    sdcard.Init(sd_cfg);

    fsi.Init(daisy::FatFSInterface::Config::MEDIA_SD);

    loadWavFiles();

    patch.StartAdc();
    patch.StartAudio(AudioCallback);

    while (1)
    {
        UpdateControls();
        UpdateOled();
    }
}
