# Seed based preambula (32bit) generator

Maximum 65 iteration need for generate preambula with autocorrelation factor lower then 6.  
Tested with all uint32_t range seeds.

Atolic True Studio 9.2.0  
Performance tests  
MCU configuration STM32L071KB (Cortex M0+) tacted from HSI 16MHz  

|										|	Os		|	O3		|	O2	 	|	O1		|	O0		|
|---------------------------------------|-----------|-----------|-----------|-----------|-----------|
| ROM, byte								|	220		|	264		|	264		|	292		|	396		|
| 100 iterarion 100 times, ms			|	12908	|	7998	|	7998	|	7926	|	26307	|
