#include "daisy_patch.h"
#include "daisysp.h"
#include "pooper.h"

using namespace daisy;
using namespace daisysp;


struct looperparams {
    float speed = 1.f;
    float size = 1.f;
    float offset = 0.f;
    float volume = 0.25f;
}; 

enum DISPLAYVALS {
    LOOPER,
    REVERB
};

DaisyPatch patch;
constexpr uint32_t BUFFER_SIZE = 8*1024*1024;
const int NUM_LOOPERS = 3;
Pooper poopers[NUM_LOOPERS];
looperparams loopers[NUM_LOOPERS];
DISPLAYVALS display = LOOPER;

ReverbSc rev;

uint32_t n = 0;
bool gate = false;
float lastOffset = 0;
int looper = 0;

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
        int K = poopers[0].getNumSamples();
        n = (n + 1) % K;
        poopers[0].process();
        out[0][i] = loopers[0].volume *  poopers[0].readL();
        out[1][i] = loopers[0].volume * poopers[0].readR();
       out[1][i] += loopers[0].volume * poopers[0].readL();
        poopers[1].process();
        out[0][i] += loopers[1].volume * poopers[1].readL();
        out[1][i] += loopers[1].volume * poopers[1].readR();
        poopers[2].process();
        out[0][i] += loopers[2].volume * poopers[2].readL();
        out[1][i] += loopers[2].volume * poopers[2].readR();

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
                  //memoryBuffer = (uint16_t*) custom_pool_allocate(size);
                  //memoryBuffer = (float *) buffer;
                  if (&buffer[0])
                  {
                    // Read the whole WAV file
                    f_lseek(&SDFile, 88);
                    if(f_read(&SDFile,&buffer[0],(size-88)/4,&bytesread) == FR_OK)
                    {
                      for(int i = 0; i < NUM_LOOPERS; ++i) {
                        poopers[i].setNumSamples((size-88)/8);
                      }
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
            
    if(patch.encoder.RisingEdge())
        display = DISPLAYVALS((int(display) + 1 ) % 2);
    
    switch (display) {

        case LOOPER: 
        {
            float ctrlSpeed = patch.GetKnobValue((DaisyPatch::Ctrl)0)*3.f;
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
            //patch.display.SetCursor(0, 20);
            uint32_t f = poopers[0].getNumSamples()/128;
            for (uint32_t i = 0; i < 128; i++) {
                patch.display.DrawPixel(i, static_cast<int>(-buffer[i*f]*8) + 30, true);
                patch.display.DrawPixel(i, static_cast<int>(-buffer[BUFFER_SIZE/2-1 + i*f]*8) + 50, true);
            }

            patch.display.DrawLine(128*loopers[looper].offset , 16,
                                    128*loopers[looper].offset, 60,
                                    true);
            patch.display.DrawLine(128*(loopers[looper].offset+loopers[looper].size), 60,
                                   128*(loopers[looper].offset+loopers[looper].size), 16,
                                        true);

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

            f = poopers[0].getNumSamples();

            patch.display.SetCursor(30, 15);
            str = std::to_string(int(loopers[looper].size * 1000.f * f / 48000.f)) + "ms";
            patch.display.WriteString(cstr, Font_6x8, true);

            patch.display.SetCursor(70, 15);
            str = std::to_string(int(loopers[looper].offset * 1000.f * f / 48000.f)) + "ms";
            patch.display.WriteString(cstr, Font_6x8, true);

            patch.display.SetCursor(70, 0);
            str = std::to_string(int(loopers[looper].volume * 100)) + "%";
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

#if 0

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
#endif

int main(void)
{	

    patch.Init();

    patch.display.Fill(false);
    patch.display.Update();
    patch.SetAudioBlockSize(48); 
	patch.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

    poopers[0].init(&buffer[0], BUFFER_SIZE);
    std::fill(&buffer[0], &buffer[BUFFER_SIZE-1], 0.f);
    poopers[1].init(&buffer[0], BUFFER_SIZE);
    poopers[2].init(&buffer[0], BUFFER_SIZE);

    //DrawSplash();


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
