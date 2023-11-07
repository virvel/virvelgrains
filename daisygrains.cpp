#include "daisy_patch.h"
#include "daisysp.h"
#include "granulator.h"

using namespace daisy;
using namespace daisysp;


DaisyPatch patch;
constexpr uint32_t BUFFER_SIZE = 8*1024*1024;
Granulator gran;
ReverbSc rev;

uint32_t n = 0;
bool gate = false;
float lastOffset = 0;
int looper = 0;
uint32_t ACTUAL_DURATION = 0;

float ctrls[4] = {0.f, 0.f, 0.f, 0.f};

DSY_SDRAM_BSS float buffer[BUFFER_SIZE];

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size) {
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
        n = (n + 1) % (BUFFER_SIZE/2);
        out[0][i] = ctrls[3]*gran.play();
        out[1][i] = out[0][i];

        rev.Process(out[0][i], out[1][i], &revL, &revR);
        out[0][i] += revL;
        out[1][i] += revR;
    }
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

  patch.DelayMs(1000);
  // Get a reference to the SD card file system
  FATFS& fs = fsi.GetSDFileSystem();
  patch.DelayMs(1000);
  // Mount SD Card
  f_mount(&fs, "/", 1);
  patch.DelayMs(1000);
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
                // For now lets break anyway to test.
                //                break;
                UINT bytesread;
                if(f_open(&SDFile, m_file_info_[m_file_cnt_].name, (FA_OPEN_EXISTING | FA_READ))
                   == FR_OK)
                {
                  m_file_sizes[m_file_cnt_] = f_size(&SDFile);
                  //float* memoryBuffer = 0;
                  UINT size = m_file_sizes[m_file_cnt_];
                  ACTUAL_DURATION = (size-88)/8;
                  //memoryBuffer = (uint16_t*) custom_pool_allocate(size);
                  //memoryBuffer = (float *) buffer;
                  if (&buffer[0])
                  {
                    // Read the whole WAV file
                    f_lseek(&SDFile, 88);
                    if(f_read(&SDFile,&buffer[0],(size-88)/4,&bytesread) == FR_OK)
                    {
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
            
            float ctrlSpeed = patch.GetKnobValue((DaisyPatch::Ctrl)0)*3.f;
            float ctrlSize = patch.GetKnobValue((DaisyPatch::Ctrl)1);
            float ctrlOffset = patch.GetKnobValue((DaisyPatch::Ctrl)2);
            float ctrlVol = patch.GetKnobValue((DaisyPatch::Ctrl)3);


            if (abs(ctrls[0]-ctrlSpeed) > 0.001) {
                ctrls[0] = ctrlSpeed;
                gran.setSpeed(ctrlSpeed); 
            }

            auto k = gran.getNumSamples();

            if (abs(ctrls[1]-ctrlSize) > 0.001) {
                ctrls[1] = ctrlSize;
                gran.setPosition(ctrlSize*k); 
            }

            if (abs(ctrls[2]-ctrlOffset) > 0.001) {
                ctrls[2] = ctrlOffset;
                gran.setDuration(ctrlOffset*k); 
            }

            if (abs(ctrls[3]-ctrlVol) > 0.001) {
                ctrls[3] = ctrlVol;
            }
 
}

void UpdateOled() {

    patch.display.Fill(false);

            //patch.display.SetCursor(0, 20);
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

    patch.display.Update();
}

int main(void)
{	

    patch.Init();

    patch.display.Fill(false);
    patch.display.Update();
    patch.SetAudioBlockSize(48); 
	patch.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

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
