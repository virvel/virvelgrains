#include "daisy_patch.h"
#include "daisysp.h"
#include "pooper.h"
#include "splash.h"


using namespace daisy;
using namespace daisysp;


struct looperparams {
    float speed = 1.f;
    float size = 1.f;
    float offset = 0.f;
    float volume = 1.f;
}; 

enum DISPLAYVALS {
    LOOPER,
    REVERB
};

DaisyPatch patch;
const int32_t BUFFER_SIZE = 240000;
const int NUM_LOOPERS = 3;
Pooper poopers[3];
looperparams loopers[NUM_LOOPERS];
ReverbSc rev;
DISPLAYVALS display = LOOPER;


uint32_t n = 0;
bool gate = false;
float lastOffset = 0;
int looper = 0;

float ctrls[4] = {1.f, 1.f, 1.f, 1.f};

DSY_SDRAM_BSS float buffer[BUFFER_SIZE];
const uint32_t CUSTOM_POOL_SIZE = 10*1024*1024;
DSY_SDRAM_BSS uint16_t custom_pool[CUSTOM_POOL_SIZE];
size_t pool_index = 0;
int allocation_count = 0;
void* custom_pool_allocate(size_t size)
{
        if (pool_index + size >= CUSTOM_POOL_SIZE)
        {
                return 0;
        }
        void* ptr = &custom_pool[pool_index];
        pool_index += size;
        return ptr;
}


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
        }
        n = (n + 1) % BUFFER_SIZE;
        out[0][i] = loopers[0].volume * poopers[0].read();        
        out[0][i] += loopers[1].volume * poopers[1].read();        
        out[0][i] += loopers[2].volume * poopers[2].read();        
        rev.Process(out[0][i], out[0][i], &revL, &revR);
        out[1][i] = out[0][i];
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
                size_t bytesread;
                if(f_open(&SDFile, m_file_info_[m_file_cnt_].name, (FA_OPEN_EXISTING | FA_READ))
                   == FR_OK)
                {
                  m_file_sizes[m_file_cnt_] = f_size(&SDFile);
                  uint16_t* memoryBuffer = 0;
                  UINT size = m_file_sizes[m_file_cnt_];
                  size_t bytesread;
                  memoryBuffer = (uint16_t*) custom_pool_allocate(size);
                  if (memoryBuffer)
                  {
                    // Read the whole WAV file
                    f_lseek(&SDFile, 44);
                    if(f_read(&SDFile,(void *)memoryBuffer,size-44,&bytesread) == FR_OK)
                    {
                      for (size_t i = 0; i < size/2; ++i) {
                        buffer[i] = s162f(memoryBuffer[i]);
                      }
                      for(int i = 0; i < 3; ++i) {
                        poopers[i].setNumSamples(size/2);
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
            
    if(patch.encoder.RisingEdge())
        display = DISPLAYVALS((int(display) + 1 ) % 2);
    
    switch (display) {

        case LOOPER: 
        {
            float ctrlSpeed = patch.GetKnobValue((DaisyPatch::Ctrl)0)*2.f;
            float ctrlSize = patch.GetKnobValue((DaisyPatch::Ctrl)1);
            float ctrlOffset = patch.GetKnobValue((DaisyPatch::Ctrl)2);
            float ctrlVol = patch.GetKnobValue((DaisyPatch::Ctrl)3);


            if (abs(ctrls[0]-ctrlSpeed) > 0.001) {
                ctrls[0] = ctrlSpeed;
                loopers[looper].speed = ctrlSpeed;
                poopers[looper].setSpeed(ctrlSpeed); 
            }

            if (abs(ctrls[1]-ctrlSize) > 0.001) {
                ctrls[1] = ctrlSize;
                loopers[looper].size = ctrlSize;
                poopers[looper].setDelayTime(ctrlSize); 
            }

            if (abs(ctrls[2]-ctrlOffset) > 0.001) {
                ctrls[2] = ctrlOffset;
                loopers[looper].offset = ctrlOffset;
                poopers[looper].setOffset(ctrlOffset); 
            }

            if (abs(ctrls[3]-ctrlVol) > 0.001) {
                ctrls[3] = ctrlVol;
                loopers[looper].volume = ctrlVol;
            }



            looper += patch.encoder.Increment();
            looper = ((looper % 3) + 3) % 3;
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
            patch.display.SetCursor(0, 0);
            std::string str  = "Pooper";
            char*       cstr = &str[0];
            patch.display.WriteString(cstr, Font_6x8, true);

            patch.display.SetCursor(42, 0);
            str = std::to_string(looper);
            patch.display.WriteString(cstr, Font_6x8, true);
         
            patch.display.SetCursor(0, 15);
            str = std::to_string(int(loopers[looper].speed * 100)) + "%";
            patch.display.WriteString(cstr, Font_6x8, true);

            patch.display.SetCursor(30, 15);
            str = std::to_string(int(loopers[looper].size * 1000)) + "ms";
            patch.display.WriteString(cstr, Font_6x8, true);

            patch.display.SetCursor(70, 15);
            str = std::to_string(int(loopers[looper].offset * 1000)) + "ms";
            patch.display.WriteString(cstr, Font_6x8, true);

            patch.display.SetCursor(70, 0);
            str = std::to_string(int(loopers[looper].volume * 100)) + "%";
            patch.display.WriteString(cstr, Font_6x8, true);
    
            //patch.display.SetCursor(0, 20);
            int f = poopers[0].getNumSamples() / 128;
            for (int i = 0; i < 128; i++) {
                patch.display.DrawPixel(i, buffer[f*i]*8 + 40, true);
            }

            patch.display.DrawLine(loopers[looper].offset*128, 30, loopers[looper].offset*128, 50,true);
            patch.display.DrawLine(128*(loopers[looper].offset+loopers[looper].size), 50, 128*(loopers[looper].offset+loopers[looper].size), 30 ,true);
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
void DrawSplash() {

        uint32_t i, b, j;
        uint32_t currentX_ = 10;
        uint32_t currentY_ = 8;

        for(i = 0; i < 808; i++)
        {
            b = tema[i];
            uint32_t f = i/104;
              for(j = 0; j < 8; j++)
              {
                  if((b << j) & 0x80) {
                    patch.display.DrawPixel(currentX_ + i%104, currentY_ + 8*f-j, true);
                  }
                  else {
                    patch.display.DrawPixel(currentX_ + i%104, currentY_ + 8*f-j, false);
                  }
              }
        }
    
    patch.display.Update(); 
}

int main(void)
{
	patch.Init();
    memset((float *)&buffer[0],0.f, BUFFER_SIZE);
    patch.SetAudioBlockSize(48); // number of samples handled per callback
	patch.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

    poopers[0].init(&buffer[0], BUFFER_SIZE);
    poopers[1].init(&buffer[0], BUFFER_SIZE);
    poopers[2].init(&buffer[0], BUFFER_SIZE);

    DrawSplash();

    SdmmcHandler::Config sd_cfg;
    sd_cfg.Defaults();
    sdcard.Init(sd_cfg);

    // Link hardware and FatFS
    fsi.Init(FatFSInterface::Config::MEDIA_SD); 


    rev.Init(48000);
    rev.SetLpFreq(10000); 

    loadWavFiles();

	patch.StartAdc();
	patch.StartAudio(AudioCallback);

	while(1) {
        UpdateControls(); 
        UpdateOled();
    }
}
