#include <iostream>
#include <vector>
#include <fstream>

    // https://www-mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html
    // http://soundfile.sapp.org/doc/WaveFormat/

int main(int argc, char * argv[]) {

    
    if (!argv[1])
    {
        printf("No Input filE!\n");
        abort();
    }

    std::string fn {argv[1]};
    std::ifstream file;
    
    file.open(fn, std::ios::in | std::ios::binary);
  
    if (!file.is_open())
    {
        printf("Error!\n"); 
        abort();
    } 

    constexpr uint32_t RIFF = 0x46464952; 
    constexpr uint32_t WAVE = 0x45564157;
    constexpr uint32_t fmt = 0x20746D66;
    constexpr uint32_t fact = 0x74636166;
    constexpr uint32_t PEAK = 0x4B414550;
    constexpr uint32_t data = 0x61746164;

    
    struct waveFile {
        uint16_t format;
        uint16_t channels;
        uint32_t samplerate;
        uint32_t bitrate;
        uint32_t num_samples;
        uint32_t file_size;
    };
   
    waveFile wf;
    int bytesread = 0;

    bool dataFound = false;

    while (!dataFound) {

        uint32_t res;
        file.read((char *)&res, sizeof(uint32_t)); 

        switch (res) {
            case fmt: {
                    file.seekg(4, std::ios::cur);
                    file.read((char *)&wf.format, sizeof(uint16_t));
                    file.read((char *)&wf.channels, sizeof(uint16_t));
                    file.read((char *)&wf.samplerate, sizeof(uint32_t));
                    file.seekg(6, std::ios::cur);
                    file.read((char *)&wf.bitrate, sizeof(uint16_t));
                    break;
                }
            case data: {
                    file.read((char *)&wf.num_samples, sizeof(uint32_t));
                    wf.num_samples = wf.num_samples / wf.channels/ 4;                    
                    dataFound = true;
                    break;
                }
            default:
                break;
        }

        bytesread = file.tellg(); 
    }

    file.close();
    

    printf("format: %d\n", wf.format);
    printf("channels: %d\n", wf.channels);
    printf("samplerate: %d\n", wf.samplerate);
    printf("bitrate: %d\n", wf.bitrate);
    printf("num samples: %d\n", wf.num_samples);

    printf("header size: %d\n", bytesread );

    return 0;
}
