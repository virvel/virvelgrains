#include "daisy_patch.h"
#include "daisysp.h"
#include "granulator.h"
#include "src/util/CpuLoadMeter.h"

#include <limits>

using namespace daisy;
using namespace daisysp;


DaisyPatch patch;
constexpr uint32_t BUFFER_SIZE = 8*1024*1024;
Granulator gran;
ReverbSc rev;

enum DISPLAYVALS {
    LOOPER,
    REVERB
};

DISPLAYVALS display = LOOPER;

uint32_t n = 0;
bool gate = false;
float lastOffset = 0;
int looper = 0;
uint32_t ACTUAL_DURATION = 0;

float ctrls[4] = {0.f, 0.f, 0.f, 0.f};

DSY_SDRAM_BSS float buffer[BUFFER_SIZE];

CpuLoadMeter cpu;


void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size) {
    cpu.OnBlockStart();
	for (size_t i = 0; i < size; i++)
	{
		out[0][i] = 0.f;
		out[1][i] = 0.f;
		out[2][i] = 0.f;
        out[3][i] = 0.f;
    }

    float revL, revR;
    for(size_t i = 0; i < size; ++i) {
        if (gate) {
        buffer[n] = in[0][i];
        buffer[BUFFER_SIZE/2 + n] = in[1][i];
        }
        n = (n + 1) % ACTUAL_DURATION;
        out[0][i] = ctrls[3]*gran.play();
        out[1][i] = out[0][i];

        rev.Process(out[0][i], out[1][i], &revL, &revR);
        out[0][i] += revL;
        out[1][i] += revR;
    }
    cpu.OnBlockEnd();
}
static SdmmcHandler sdcard;
FatFSInterface fsi;
/** Global File object for working with test file */
FIL SDFile;

const int maxFiles = 1;
daisy::WavFileInfo m_file_info_[maxFiles];
int m_file_sizes[maxFiles];

void loadWavFiles()
{
  std::string m_sd_debug_msg="no (fat32) sdcard";
  int m_file_cnt_ = 0;
  FRESULT result = FR_OK;
  FILINFO fno;
  DIR     dir;
  char *  fn;

  //patch.DelayMs(1000);
  // Get a reference to the SD card file system
  FATFS& fs = fsi.GetSDFileSystem();
  //patch.DelayMs(1000);
  // Mount SD Card
  f_mount(&fs, "/", 1);
 // patch.DelayMs(1000);
  // Open Dir and scan for files.
  if(f_opendir(&dir, "/") == FR_OK)
  {
    m_sd_debug_msg = "sd: no files";
    do
    {
        result = f_readdir(&dir, &fno);
        // Exit if bad read or NULL fname
        if(result != FR_OK || fno.fname[0] == 0)
            break;
        // Skip if its a directory or a hidden file.
        if(fno.fattrib & (AM_HID | AM_DIR))
            continue;
        // Now we'll check if its .wav and add to the list.
        fn = fno.fname;
        if(m_file_cnt_ < maxFiles)
        {
            if(strstr(fn, ".wav") || strstr(fn, ".WAV"))
            {
                strcpy(m_file_info_[m_file_cnt_].name, fn);

                UINT bytesread;
                if(f_open(&SDFile, m_file_info_[m_file_cnt_].name, (FA_OPEN_EXISTING | FA_READ))
                   == FR_OK)
                {
                  m_file_sizes[m_file_cnt_] = f_size(&SDFile);

                  uint32_t size = m_file_sizes[m_file_cnt_];
                  size = (size-88)/4;
                  ACTUAL_DURATION = size;

                  if (&buffer[0])
                  {
                    // Jump ahead of header
                    f_lseek(&SDFile, 88);
                    UINT chunk = 16392*4;
                    FRESULT res = FR_OK; 

                    if (size < chunk)
                        chunk = size;

                    uint32_t offset = 0;
                    uint32_t br = 0;

                    while (size > offset) { 
                        if (chunk > (size - offset))
                            chunk = size - offset;

                        res = f_read(&SDFile,&buffer[offset],chunk,&bytesread);
                        patch.DelayMs(500);
                        br += bytesread;
                        offset += chunk;
                        if (res != FR_OK) {
                            f_close(&SDFile);
                            break;
                            return;
                        }
                        
                    }
                    if (res == FR_OK)
                    {
                         gran.setNumSamples(br/2);
                        uint32_t mid = BUFFER_SIZE/2;
                        uint32_t i,j;
                        for (i = 1; i < BUFFER_SIZE; ++i) {
                            j = i<mid ? i*2 : ((i-mid)*2 + 1);
                            while (j<i) {
                                j = j<mid ? j*2 : ((j-mid)*2 + 1);
                            }
                            float tmp = buffer[i];
                            buffer[i] = buffer[j];
                            buffer[j] = tmp;
                        }
                        m_file_cnt_++;
                    }
                    else {
                        f_close(&SDFile);
                    }
                    /*if(f_read(&SDFile,&buffer[0],(size-88)/4,&bytesread) == FR_OK)
                    {
                        if (bytesread != (size-88)/4) {
                            abort();
                        }
                        
                        gran.setNumSamples((size-88)/8);
                        uint32_t mid = BUFFER_SIZE/2;
                        uint32_t i,j;
                        for (i = 1; i < BUFFER_SIZE; ++i) {
                            j = i<mid ? i*2 : ((i-mid)*2 + 1);
                            while (j<i) {
                                j = j<mid ? j*2 : ((j-mid)*2 + 1);
                            }
                            float tmp = buffer[i];
                            buffer[i] = buffer[j];
                            buffer[j] = tmp;
                        }
                    
                      m_file_cnt_++;
                    }
                    else {
                        abort();
                    }
                    */
                    
                  }
                  f_close(&SDFile);
                }
            }
        }
        else
        {
            break;
        }
    } while(result == FR_OK);
  }
  f_closedir(&dir); 

}

void UpdateControls() {

    patch.ProcessAllControls();

    gate = patch.gate_input[0].State();

    if(patch.encoder.RisingEdge())
        display = DISPLAYVALS((int(display) + 1 ) % 2);
    
    switch (display) {

        case LOOPER: 
        {

            float ctrlSpeed = patch.GetKnobValue((DaisyPatch::Ctrl)0)*3.f;
            float ctrlOffset = patch.GetKnobValue((DaisyPatch::Ctrl)1);
            float ctrlSize = patch.GetKnobValue((DaisyPatch::Ctrl)2);
            float ctrlVol = patch.GetKnobValue((DaisyPatch::Ctrl)3);


            if (abs(ctrls[0]-ctrlSpeed) > 0.001) {
                ctrls[0] = ctrlSpeed;
                gran.setSpeed(ctrlSpeed); 
            }


            if (abs(ctrls[1]-ctrlOffset) > 0.001) {
                ctrls[1] = ctrlOffset;
                gran.setOffset(ctrlOffset); 
            }

            if (abs(ctrls[2]-ctrlSize) > 0.001) {
                ctrls[2] = ctrlSize;
                gran.setDuration(ctrlSize); 
            }

            if (abs(ctrls[3]-ctrlVol) > 0.001) {
                ctrls[3] = ctrlVol;
            }
            break;
        }
         case REVERB:
        { 
            float ctrlFreq = patch.GetKnobValue((DaisyPatch::Ctrl)1) * 10000;
            float ctrlRev = patch.GetKnobValue((DaisyPatch::Ctrl)0);
            if (abs(ctrls[1]-ctrlFreq) > 0.001) {
                ctrls[1] = ctrlFreq;
                rev.SetLpFreq(ctrlFreq); 
                
            }
            if (abs(ctrls[0]-ctrlRev) > 0.001) {
                ctrls[0] = ctrlRev;
                rev.SetFeedback(ctrlRev); 
                
            }
            
            break;
        }
        
        default:
            break;
    }

}

void UpdateOled() {

    patch.display.Fill(false);

    switch (display) {

        case LOOPER: 
        {
        
            uint32_t f = ACTUAL_DURATION/128;
            for (uint32_t i = 0; i < 128; i++) {
                patch.display.DrawPixel(i, static_cast<int>(-buffer[i*f]*8) + 30, true);
                patch.display.DrawPixel(i, static_cast<int>(-buffer[BUFFER_SIZE/2-1 + i*f]*8) + 50, true);
            }

            patch.display.DrawLine(128*ctrls[2] , 36,
                                    128*ctrls[2], 60,
                                    true);
            patch.display.DrawLine(64*(ctrls[1]+ctrls[2]), 60,
                                   64*(ctrls[1]+ctrls[2]), 36,
                                        true);

            patch.display.SetCursor(0, 0);
            std::string str  = "Yoyo";
            char*       cstr = &str[0];
            patch.display.WriteString(cstr, Font_6x8, true);
         
            patch.display.SetCursor(0, 15);
            str = std::to_string(int(ctrls[0]*100.f));
            patch.display.WriteString(cstr, Font_6x8, true);

            patch.display.SetCursor(30, 15);
            str = std::to_string(int(ctrls[1]*100.f));
            patch.display.WriteString(cstr, Font_6x8, true);

            patch.display.SetCursor(70, 15);
            str = std::to_string(int(ctrls[2]*100.f));
            patch.display.WriteString(cstr, Font_6x8, true);

            patch.display.SetCursor(70, 0);
            str = std::to_string(int(ctrls[3]*100.f));
            patch.display.WriteString(cstr, Font_6x8, true);

            patch.display.SetCursor(0, 50);
            str = std::to_string(uint32_t(ctrls[2]*ACTUAL_DURATION));
            patch.display.WriteString(cstr, Font_6x8, true);

            patch.display.SetCursor(0, 40);
            str = std::to_string(ACTUAL_DURATION);
            patch.display.WriteString(cstr, Font_6x8, true);

            patch.display.SetCursor(90, 0);
            str = std::to_string(int(cpu.GetAvgCpuLoad()*100));
            patch.display.WriteString(cstr, Font_6x8, true);
            break;
        }
        case REVERB:
        { 
            patch.display.SetCursor(0, 0);
            std::string str  = "FX";
            char*       cstr = &str[0];
            patch.display.WriteString(cstr, Font_6x8, true);

            patch.display.SetCursor(0, 40);
            str = std::to_string(int(ctrls[0]*100)) + "%";
            patch.display.WriteString(cstr, Font_6x8, true);

            patch.display.SetCursor(30, 40);
            str = std::to_string(int(ctrls[1])) + "Hz";
            patch.display.WriteString(cstr, Font_6x8, true);

            break;
        }
        
        default:
            break;
    }

    patch.display.Update();
}

int main(void)
{	

    patch.Init();

    patch.display.Fill(false);
    patch.display.Update();
    patch.SetAudioBlockSize(48); 
	patch.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

    cpu.Init(48000, 48);

    gran.init(&buffer[0], BUFFER_SIZE);
    std::fill(&buffer[0], &buffer[BUFFER_SIZE-1], 0.f);


    rev.Init(48000);
    rev.SetLpFreq(10000);
    rev.SetFeedback(0.6);

    SdmmcHandler::Config sd_cfg;
    sd_cfg.Defaults();
    sdcard.Init(sd_cfg);

    // Link hardware and FatFS
    fsi.Init(FatFSInterface::Config::MEDIA_SD); 

    loadWavFiles();

	patch.StartAdc();
	patch.StartAudio(AudioCallback);

	while(1) {
        UpdateControls(); 
        UpdateOled();
    }
}
