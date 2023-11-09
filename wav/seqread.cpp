#include <cstdio>
#include <cstdint>


int main() {
    auto file = fopen("wav.cpp", "rb");

    char yo[100];

    uint32_t max = 59;
    uint32_t chunk = 10;
    uint32_t bytesread = 0;
    int i = 0; 

    while (bytesread < max) { 
        fread(&yo[0]+i*chunk, chunk, sizeof(char), file);
        bytesread += chunk;
        i++;
    }
    printf("bytes read: %d\n", bytesread);
    printf("%s\n", yo);

    return 0;
}
