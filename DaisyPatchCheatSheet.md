# Daisy Patch Cheat Sheet

Version 1.03

Some information about the Daisy Patch that can be useful while becoming familiar with the Daisy platform. **It is not meant to be a substitute for official documentation** which should be read before: [Daisy Wiki documentation](https://github.com/electro-smith/DaisyWiki/wiki).

Sections:

- [Hardware hints](#hw)
- [Software hints](#sw)
- [Build/toolchain](#build)
- [Links](#links)

If you find errors or have suggestions contact me: development [at] algoritmarte [dot] com

Latest version can be found on [github](https://github.com/algoritmarte/algodaisy). Use a Markdown viewer to open this file for better readability.

<a name="hw"></a>
# 1. Hardware hints

## 1.1. Summary (voltages)

Schematics can be downloaded here: [Daisy patch schematics](https://github.com/electro-smith/Hardware/tree/master/reference/daisy_patch)

In the follwing table `hw` is declared as

```
DaisyPatch hw;
```


| Port | Range | C++ type | C++ |
|------|---------------------|------------|---|
|CTRL1-4 CV IN  | [+/-5V] | - | automatically divided by 5 and added to Knob value|
| KNOB 1-4              | [0,1] | float | `hw.GetKnobValue(DaisyPatch::CTRL_1)` |
|AUDIO1-4 IN        | [+/-5V] | float |`AudioHandle::InputBuffer  in[i]` in `AudioCallback` |
|AUDIO1-4 OUT       | [+/-5V] | float |`AudioHandle::OutputBuffer  out[i]`in `AudioCallback`
|CV1-2 OUT          | [0V,+5V]  | uint16_t (12bit) | `hw.seed.dac.WriteValue(DacHandle::Channel::ONE,value);` |
|GATE1-2 IN         | - | bool | `hw.gate_input[0].State(); // or ....Trig();` |
|GATE OUT           | [0V,+5V] | bool | `dsy_gpio_write(&hw.gate_output, trigOut);` |
| ENCODER (button + increment) | -1,0,1 | int | see below
| LED               | - | bool | hw.seed.SetLed( ledStatus ); |

## CV Inputs and Knobs

The CV\_Input voltage is divided by 5 and added to the corresponding Knob_Position; the result is then clamped to [0,1]

```
Knob_Position (0=all way down, 1=all way up)
CV_Input (-5,+5 Volt)

Knob_Value = Knob_Position + CV_Input / 5.0
Knob_Value = clamp( Knob_Value, 0, 1 )  // restrict it to [0,1]
```

For example if a Knob is fully turned CCW (left) and a CV of 1 volt is applied then the value of the knob is 0.200

**Tip**: beware that _the hardware is not that accurate_ (a ~4% error is not so rare); for some parameters/CV you may want to make sure that a knob is 0 when it is fully CCW or 1 when it is fully CW (or 0.5 if it's at 12 o'clock), so you should consider to clamp it to 0/1 if the value read is near those positions.

```
float knob1Value = hw.GetKnobValue(DaisyPatch::CTRL_1);
if ( knob1Value < 0.05f ) knob1Value = 0.f;
  else
    if ( knob1Value > 0.95f ) knob1Value = 1.f;
// ... or define it as an inline function
```
Also condìsider that the values read are often "floating" a little bit so be sure to add some LP filtering if the operations linked to the change of their values are computational expensive.



## CV OUT1 / OUT2

The seed generates a voltage between 0 and 3V, and the circuitry converts it to a value between 0 and 5V.

The DAC has a 12 bit resolution so valid values are 0 to 4095.

```
// writes 1 volt to CV1  (819.2f is 4096/5)
float volt = 1;
hw.seed.dac.WriteValue(DacHandle::Channel::ONE,  (uint16_t)( volt * 819.2f ) );
```

## AUDIO

The Patch uses an AK4556 3V 192kHz 24bit codec chip. The (hot) level at AUDIO inputs is lowered (+/-5V to 3V) using 2 TL074, then raised again on output (3V to +/-5V) using 2 TL074. (*NdA: is it correct ?!?*).


## OLED

### Info

Model: SSD130x ( 128 x 64 Dot Matrix OLED/PLED Segment/Common Driver with Controller)

```
Resolution: 128x64 pixels

Characters using Font_7x10:  18x5 (Columns x Rows)
Characters using Font_11x18: 11x3
Characters using Font_6x8:   21x8
```

### Drawing/printing

```
hw.display.Fill(false);
hw.display.DrawLine(0, 10, 128, 10, true);
//...
int line = 0;
hw.display.SetCursor(0, line++ * 13 );
hw.display.WriteString("Hello world", Font_7x10, true);       
hw.display.SetCursor(0, line++ * 13 );
hw.display.WriteString("Inverse string", Font_7x10, false);       
//...
hw.display.Update();
```

Available fonts:

```
libDaisy\src\util\oled_fonts.c:FontDef
/** These are the different sizes of fonts
(width x height in pixels per character) */
extern FontDef Font_6x8;
extern FontDef Font_7x10;  /**< & */
extern FontDef Font_11x18; /**< & */
extern FontDef Font_16x26; /**< & */
```


## LED

Lil cute small led ...

```

hw.seed.SetLed( true );
hw.seed.SetLed( false );

```

## Memory

Memory stuff defined in `libDaisy/src/sys/system.cpp`


| Region         | Start     | Size(hex)| Size |
|----------------|-----------|----------|------|
|           FLASH|           |0x20000   |128 KB|
|         DTCMRAM|           |          |128 KB|
|            SRAM|           |          |512 KB|
|          RAM_D2|           |          |288 KB|
|          RAM_D3|           |          |64 KB |
|         ITCMRAM|           |          |64 KB |
|           SDRAM|0xC0000000 |0x4000000| 64 MB |
|       QSPIFLASH|           |          |8 MB  |


In order to alloc some space on the SDRAM use the `DSY_SDRAM_BSS` modifier:

```
// 10 second delay line on the external SDRAM
#define MAX_DELAY ((size_t)(10.0f * 48000.0f))
daisysp::DelayLine<float, MAX_DELAY> DSY_SDRAM_BSS delay;

// 16K buffer
#define MYBUFSIZE 16384
uint8_t DSY_SDRAM_BSS mybuf[MYBUFSIZE];
```

Note that there is no `malloc/free` (on embedded systems, memory should be used very carefully); but for most purposes (*when you are sure that objects are allocated only once dynamically*), a simple trivial wrapper could do the job:

```
int memAllocated = 0;
void *malloc( size_t numbytes ) {
	if ( memAllocated + numbytes > MYBUFSIZE ) return 0;
	void *res = & mybuf[ memAllocated ];
	memAllocated += numbytes;
	return res;
}
void free(void *mem) { /* donothing*/ }
```



If your program takes >99.0% of the FLASH memory: you can try to add  
```
OPT = -Os
```  
to your Makefile anywhere above the include `$(SYSTEM_FILES_DIR)/Makefile` line to compile with optimization targeted to reduce code size. Sometimes that may break things, but sometimes you can reduce the size up to ~15 KB. 

<a name="sw"></a>
# 2. Software hints


## DaisyPatch class

    /* These are exposed for the user to access and manipulate directly
       Helper functions above provide easier access to much of what they are capable of.
    */
    DaisySeed       seed;                             /**< Seed object */
    Encoder         encoder;                          /**< Encoder object */
    AnalogControl   controls[CTRL_LAST];              /**< Array of controls*/
    GateIn          gate_input[GATE_IN_LAST];         /**< Gate inputs  */
    MidiUartHandler midi;                             /**< Handles midi*/
    OledDisplay<SSD130x4WireSpi128x64Driver> display; /**< & */
    // TODO: Add class for Gate output
    dsy_gpio gate_output; /**< &  */    
    

## Skeleton program
Simple skeleton program (*NOTE not fully working code - only as a reference for a common program structure*).

For a fully working example download the [PatchSkeleton example](https://github.com/algoritmarte/algodaisy/patch_algodaisy/PatchSkeleton) (main source code: [patchskeleton.cpp](https://github.com/algoritmarte/algodaisy/blob/main/patch_algodaisy/PatchSkeleton/patchskeleton.cpp))


```
using namespace daisysp;
using namespace daisy;

DaisyPatch hw;    
float sample_rate;
bool led_state = false;

const std::string labels[2] = {	"Hello", ""World" }; 
    
int void main() {
  hw.Init();
  //hw.SetAudioBlockSize(32); // if needed (default 48 )
  float sample_rate = hw.AudioSampleRate();
  
  hw.StartAdc();
  hw.StartAudio(AudioCallback);
  while(1) {
	hw.seed.SetLed(led_state);
  	
  	// update screen ...
  	UpdateScreen();
  	
    // Wait 50ms
    DaisyPatch::DelayMs(50); // limit to 20Hz ?!?      	
  }
}

void AudioCallback(AudioHandle::InputBuffer  in,
               AudioHandle::OutputBuffer out,
               size_t                    size) 
{
  UpdateControls(); // process digital/analog values
  float myin0,myin1,myin2,myin3;
  for(size_t i = 0; i < size; i++) {  
    out[0][i] = in[0][i];
    out[1][i] = in[1][i];
    out[2][i] = in[2][i];
    out[3][i] = in[3][i];
  }
}

void UpdateControls()
{
  hw.ProcessAllControls(); 
  // shortcut for: hw.ProcessDigitalControls(); hw.ProcessAnalogControls();
  
  //read knob values

  float ctrl[4];
  for(int i = 0; i < 4; i++)
  {
    ctrl[i] = hw.GetKnobValue((DaisyPatch::Ctrl)i);
  }
  
  //process encoder
  bool encoder_helddown = hw.encoder.Pressed();
  bool encoder_pressed = hw.encoder.RisingEdge();    
  int encoder_inc = hw.encoder.Increment();
  bool encoder_released = hw.encoder.FallingEdge();    
  float encoder_timeheld = hw.encoder.TimeHeldMs();    

  bool gate1_state = hw.gate_input[0].State();
  bool gate1_trig = hw.gate_input[0].Trig();
  bool gate2_state = hw.gate_input[1].State();
  bool gate2_trig = hw.gate_input[1].Trig();
  

  float cv1_out = 2; // 2 volts
  hw.seed.dac.WriteValue(DacHandle::Channel::ONE,  (uint16_t)( cv1_out * 819.2f ) );
  
  // write gate out
  dsy_gpio_write(&hw.gate_output, encoder_press );    
}

void UpdateScreen() {
  hw.display.Fill(false);
//  hw.display.DrawLine(0, 10, 128, 10, true);
  int line = 0;
  hw.display.SetCursor(0, line++ * 13 );
  hw.display.WriteString(labels[0], Font_7x10, true);              
  hw.display.Update();
}
```

## Timing

```
DaisyPatch::DelayMs(uint32_t msec);  // delay del milliseconds
DaisyPatch::DelayUs(uint32_t usec);  // delay del microseconds

# example:
hw.DelayMs(50);

uint32_t hw.seed.system.GetNow(); // current elapsed milliseconds
uint32_t hw.seed.system.GetUs();  // current elapsed microseconds
```
## Performance meter

Include the file:

```
#include "util/CpuLoadMeter.h"
```

Declare in the global section

```
CpuLoadMeter meter;
```

Initialize it in the main section:

```
int main(void) {
  // ...
  meter.Init( hw.AudioSampleRate(), hw.AudioBlockSize() );
  hw.StartAdc();
  hw.StartAudio( AudioCallback );
  // ...
}
```

In the audio callback routine at the beginning / end call `OnBlockStart()` /  `OnBlockEnd()`

```
void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
	meter.OnBlockStart();
	
	// ...

	meter.OnBlockEnd();
}
```

Then you can call  
`meter.GetMaxCpuLoad()` `meter.GetAvgCpuLoad()` `meter.GetMinCpuLoad()` 
to get the Max / Avg / Min average CPU load in the range 0..1; for example to print them on the screen:


```
int main(void) {
  // ...
  // main loop
  while (1) {
            char buf[18] = {};
            hw.display.Fill(false);
            int line = 0;
            
            hw.display.SetCursor(0, line++ * 13);
            float2str(buf, 0, 8, meter.GetMaxCpuLoad(), 3);
            hw.display.WriteString(buf, Font_7x10, true);

            hw.display.SetCursor(0, line++ * 13);
            float2str(buf, 0, 8, meter.GetAvgCpuLoad(), 3);
            hw.display.WriteString(buf, Font_7x10, true);

            hw.display.Update();
            hw.DelayMs(50);
  }
}
```

## MIDI In/Out
In order to use MIDI you must call `hw.midi.StartReceive()` and regularly check if new events are received with `hw.midi.Listen()` and `hw.midi.HasEvents()`. You should  also process the events received with `hw.midi.PopEvent()` in order to prevent the queue from filling up.

A typical program skeleton is:

```
int main(void)
{
	hw.Init();
	hw.midi.StartReceive(); // start MIDI receieve
	hw.StartAdc();
	hw.StartAudio(AudioCallback);
	while(1) {
   	    // fetch data from uart
		hw.midi.Listen();          
		// Handle MIDI Events
        while(hw.midi.HasEvents())
        {
            HandleMidiMessage(hw.midi.PopEvent());
        }
	}
}
// Typical Switch case for Message Type.
// implements a MIIMALISTIC monophonic MIDI to CV/gate converter
void HandleMidiMessage(MidiEvent m)
{
    switch(m.type)
    {
        case NoteOn:
        {
            NoteOnEvent p = m.AsNoteOn();
            // it's quite common to use a NoteOn event with velocity=0 
            // as an alternative to NoteOff
            if(m.data[1] != 0)
            {
                p = m.AsNoteOn();
				int channel = p.channel;
				int vel = p.velocity;
				int midinote = p.note;
				
				midinote -= 36; // C2 as lowest note
				if ( midinote < 0) midinote = 0;
				
				// write 1V/oct pitch on CV OUT 1
				float v = 1.0f * midinote / 12.0f;
				hw.seed.dac.WriteValue(DacHandle::Channel::ONE,
						(uint16_t)( v * 819.2f ) );
				dsy_gpio_write(&hw.gate_output, true);					
            } else 
            {   // m.data[1] == 0 is equivalent to a note off
				dsy_gpio_write(&hw.gate_output, false);					            
            }
        }
        break;
        case NoteOff:
        {
	        NoteOffEvent p = m.AsNoteOff();
			dsy_gpio_write(&hw.gate_output, false);					            
        }
        break;        
        case ControlChange:
        {
            ControlChangeEvent p = m.AsControlChange();
            switch(p.control_number)
            {
                case 1:
                    // CC 1 handling ...
                    break;
                case 2:
                    // CC 1 handling ...
                    break;
                default: break;
            }
            break;
        }
        default: break;
    }
}

```

If you want to send a MIDI message use:


```
hw.midi.SendMessage (uint8_t *bytes, size_t size ) 		
```

For further information see: 

- [MidiHandler class reference](https://electro-smith.github.io/libDaisy/classdaisy_1_1_midi_handler.html)
- [MidiEvent class reference](https://electro-smith.github.io/libDaisy/structdaisy_1_1_midi_event.html)


## Formatting numbers

`sprintf`, `snprintf`, ... (`<cstdio>`) don't work with floats; if you want a lightweight replacement use the great Marco Paland's routines:

[https://github.com/mpaland/printf](https://github.com/mpaland/printf)





<a name="build"></a>
# Building / toolchain

The full toolchain can be downloaded here: [Daisy Toolchain](https://github.com/electro-smith/DaisyToolchain) (follow the instructions on the page)

Note if you already have a MSYS2/MINGW64 installation you can simply add the bin directory of the toolchain to the PATH:

```
PATH=$PATH:/D/SDK/DaisyToolchain-0.3.3/windows/bin
export PATH
```

To manually download/install the packages:

```
GNU Make: 4.3
openocd: 0.11.0
dfu-util: 0.10
clang-format: 10.0.0
arm toolchain: 10-2020-q4-major
```

Use the package manager: 

```
pacman -S mingw-w64-x86_64-dfu-util
pacman -S mingw-w64-x86_64-openocd
....
```


VisualCode: be sure that in the project directory there is a `.vscode` subfolder
 with the files: 

```
c_cpp_properties.json
launch.json
STM32H750x.svd
tasks.json
```

## Creating a new program

In order to create a new program you can use the helper.py program in the DaisyExamples root directory:

```
./helper.py create patch/MyProgram --board patch
```



<a name="links"></a>
# Useful links documentation/stuff


- [libDaisy official documentation](https://electro-smith.github.io/libDaisy/index.html)

- [Main Daisy Patch control code daisy_patch.cpp](https://github.com/electro-smith/libDaisy/blob/master/src/daisy_patch.cpp)

- [Daisy patch schematics](https://github.com/electro-smith/Hardware/tree/master/reference/daisy_patch)

# To-do

Coming soon ... 

* <strike>add a link to a fully working test patch</strike>
* <strike>details on how to measure CPU workload</strike>
* <strike>considerations about memory (where to alloc buffers)</strike>
* <strike>MIDI read/write</strike>
* load/save configuration parameters
* how-to access SD-card / file system
* menu(s)
* daisy library
* hardware abstraction