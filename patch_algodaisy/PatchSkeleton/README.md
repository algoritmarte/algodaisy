# PatchSkeleton

A simple skeleton program that shows some basic methods to control the Daisy Patch.

By Algoritmarte

Web: [https://www.algoritmarte.com](https://www.algoritmarte.com)

GitHub: [https://github.com/algoritmarte/algodaisy](https://github.com/algoritmarte/algodaisy) 

## Description

The patch is for demonstration purposes only:


| Port             | Description |
|------------------|-------------|
|AUDIO1 OUT        | Simple scale using a sine wave |
|AUDIO2 OUT        | White noise |
|AUDIO3 OUT        | Mix AUDIO1 IN + AUDIO2 IN |
|AUDIO4 OUT        | Mix AUDIO3 IN + AUDIO4 IN |
|GATE1 IN          | a pulse on GATE1 IN inverts the outputs of GATE OUT and led status |
|GATE2 IN          | unused |
|CV1 OUT           | the 1V/OCT pitch of the current note |
|CV2 OUT           | unused |
|GATE OUT          | simple on/off gate triggered by GATE1 IN or encoder button |
|LED               | same state of GATE OUT |
|Encoder press     | same as a trigger on GATE1 IN |
|Encoder increment | used to change the value `encoder_track` on the screen |
|KNOBs and CVs     | their current (raw) values are displayed on the screen |

