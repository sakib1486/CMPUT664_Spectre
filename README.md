# CMPUT664 Final Project : Isolataing Speculative Data from Transient Execution
This repository contains the project works from CMPUT 664 that was carried out in Winter2022 at the University of Alberta.

<details open="open">
<summary>Table of Contents</summary>

- [Participants](#participants)
- [Acknowledgement](#Acknowledgement)
- [Tasks](#tasks)
- [Tools Used](#tools)
- [Setting Up Gem5](#setup)
- [Experiments](#experiments)
- [Outputs](#outputs)
- [Results](#results)
- [Spectre Attack on your Device](#spectreondevice)
- [Report](#report)
- [Bibliography](#bibliography)

</details>

## Participants:

|Student name|  CCID  |
|------------|--------|
|Sakib Hasan |sakib2  |
|Noshin Tabassum  |ntabassu|
|Karan Chadha  |kchadha1|


## Acknowledgement 

In accordance with the UofA Code of Student Behaviour, we acknowledge that we have listed all external resources we consulted for this assignment. Non-detailed oral discussion with others is permitted as long as any such discussion is summarized and acknowledged by all parties.

We also express our kind thanks to the Course Instructore Dr. Karim Ali. You can visit his personal website right [here](https://karimali.ca/).

## Tasks

The main task in focus here is the widely known **Spectre Attack** on the microprocessorr architectures which poses a significant security threat, and attackers take leverage of this very vulnerabillity to leak secured data. In brief, a Spectre attack tricks the processor into executing instructions along the wrong path. Even though the processor recovers and correctly completes its task, hackers can access confidential data while the processor is heading the wrong way. For more details on the introduction of this very vulnerability could be found at [Sepctre Introduction](https://meltdownattack.com/).

For our project the problem statement was to "_Isolate the Speculative data from Transient Executions to protect exposure of the Cache Memory or to secure the secret data placed in memory_". **Transient Executions** are CPU instructions that tries to load the memory to the CPU through a wrong path for forwarding. Basically, attackers exploit this very path to reveal the saved data in Cache. So our idea here is to isolate the Cache memory so that without a verified request from CPU, the data is never loaded to the memory.

## Tools

Unfortunately, this sort of study are not possible to test wtihout a proper, vast, and controlled infrastracture to test the techniques on physical microprocessors within a course project scope. But very close simulations could be performed with various simulators available as open-source project like- [_Gem5_](https://www.gem5.org/) and [_Multi2Sim_](http://www.multi2sim.org/). Therefore, for this project we implemented our technique on the _Gem5_ open-source simulation tool.

## Setting up Gem5

We set up the Gem5 simulator on a `Linux OS v21.04` machine which is alike with the `v20.04 LTS` as well with a _Ryzen 7 5800_ CPU in its core. If all the requirements are already satisfied on the machine that one is using, cloning this repository would suffice setting up the _X86_ architecture binary on Gem5 that was used for our demonstration. Try building our X86 build with the following command in the top directory:

`scons build/X86/gem5.opt -j 4`

This should build gem5 X86 architecture of ours on 4 threads if eveything is OK. If not, you can follow the very basic and simple to follow installation instructions from [Gem5 Building Documentation](https://www.gem5.org/documentation/learning_gem5/part1/building/).

So if every requirements are installed and satisfied, you can build with the above command for _X86_. To build an _ARM_ architecture binary just replace 'X86' with 'ARM'. You can buildthe architectures in this way. One downside to Gem5 is we sometimes cannot build two architecture binaries(say X86 and ARM) together for use. If you want to use ARM after you are done with ARM, you  may need to clean the previous build folder first by:

`python3 $(which scons) --clean --no-cache`

And rebuild again with the architecture that you want.

## Experiments

We designed our experiments in two ways:
  1. _Spectre Attack_ on base X86 and ARM architectures.
  2. _Spectre Attack_ on modified X86 and ARM architectures.

### 1. _Spectre Attack_ on base X86 and ARM architectures

This experiment will demonstrate if we can perform spectre attacks to expose the secretly held data in the memory by speculative executions. To do that we are using a .C file named [spectrev1.c](tests/test-progs/spectre/src/spectrev1.c). We need to compile this first for the X86 architecture to run it with Gem5. But if the internal terminal and default _gcc_ is used, it might not support in Gem5's X86 or ARM architecture becuase of the mismatch in the CPU architecture and its supported _gcc_ version might be different from what is used in X86 based ones.

The wayaround to this is **_dockercross_** which is a tool to cross-compile C/C++ codes for the simulators. Using this is pretty neat and easy. To build and compile a .C file, the commands that needs to followed are as follows:

- Setup the dockross-x86 for X86 architecture:

`docker run -rm dockcross/linux-x86 > ./dockcross-x86`
`chmod +x ./dockcross-x86`

- Compiling a .c file for x86:

`./dockcross-x86 bash -c '$CC {filename}.c -o {output filename} -static'`

This should be done on the folder where your .c file is saved. Our spectre code file and its corresponding compiled output with dockcross can be found [here](test/test-progs/spectre/src).

Now from the top directory, run the following command to use the built x86 architecture on Gem5 to run the output file:

`build/X86/gem5.opt configs/example/se.py --cmd=tests/test-progs/spectresrc/spectre`

### 2. _Spectre Attack_ on modified X86 and ARM architectures

To run our shielding technique, you need to use all the files and folders from this repository. Two options that we followed to isolate the cache memory are-

  - Firstly, Reducing the size of the Cache Memory by setting the Cache memorysize
  - Secondly, Disabling the Cache Memory while running the output.

For the first option, from the top directory, run the following command:

`build/X86/gem5.opt configs/example/se.py --cmd=tests/test-progs/hello/bin/x86/linux/hello --cpu-type=TimingSimpleCPU --l1d_size=64kB --l1i_size=16kB`

And for the second option, run the following command:

`build/X86/gem5.opt configs/example/se.py --cmd=tests/test-progs/hello/bin/x86/linux/hello --cpu-type=TimingSimpleCPU --l1d_size=64kB --l1i_size=16kB --caches`

To run the similar experiment on ARM binaries, you just need to switch 'X86' to 'ARM' after it has been built up correctly on Gem5.

## Outputs

Based on CACHE_HIT_THRESHOLD value present in the Spectre code explained in the [Project Report](Report/report.pdf), the expectd output from the first experiment should be as follows:

[!alt text](https://github.com/sakib1486/CMPUT664_Spectre/edit/main/Results/Option1_1.jpg?raw=true)

[!alt text](https://github.com/sakib1486/CMPUT664_Spectre/edit/main/Results/Option1_2.jpg?raw=true)

Similar execution for the second experiment should be as follows irrespective of the CACHE_HIT_THRESHOLD in the attack code, and irrespective of the architectures as well.

[!alt text](https://github.com/sakib1486/CMPUT664_Spectre/edit/main/Results/Option2.jpg?raw=true)











