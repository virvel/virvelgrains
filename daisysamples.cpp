#include "daisy_patch.h"
#include "daisysp.h"
#include "pooper.h"

using namespace daisy;
using namespace daisysp;

DaisyPatch hw;
#define MAX_BUFFER_SIZE 192000
Pooper pooper;

float DSY_SDRAM_BSS buffer[MAX_BUFFER_SIZE];
uint32_t n = 0;
bool gate = false;
float lastOffset = 0;

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size) {

	for (size_t i = 0; i < size; i++)
	{
		out[0][i] = in[0][i];
		out[1][i] = in[1][i];
		out[2][i] = in[2][i];
        out[3][i] = in[3][i];
    }

    for(size_t i = 0; i < size; ++i) {
        if (gate) {
        buffer[n] = in[0][i];
        }
        n = (n + 1) % MAX_BUFFER_SIZE;
        out[0][i] = pooper.read();        
    }
}
void UpdateControls() {

    hw.ProcessAnalogControls();
    float ctrl = hw.GetKnobValue((DaisyPatch::Ctrl)0);
    pooper.setSpeed(ctrl*2); 
    float ctrlDelay = hw.GetKnobValue((DaisyPatch::Ctrl)1);
    pooper.setDelayTime(ctrlDelay*2.f); 
    float ctrlOffset = hw.GetKnobValue((DaisyPatch::Ctrl)3);
    if (abs(lastOffset-ctrlOffset) > 0.01) {
        lastOffset = ctrlOffset;
        pooper.setOffset(float(int(10.f*lastOffset)/10.f)); 
    }
    
    gate = hw.gate_input[0].State();

}

int main(void)
{
	hw.Init();
    memset((float *)&buffer[0],0, MAX_BUFFER_SIZE);
	hw.SetAudioBlockSize(4); // number of samples handled per callback
	hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    hw.display.Fill(false);
    hw.display.Update();

    pooper.init(&buffer[0], 192000);
//    pooper.setOffset(0.25);
	hw.StartAdc();
	hw.StartAudio(AudioCallback);
	while(1) {
    UpdateControls(); 
    }
}
