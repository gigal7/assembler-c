;
;



.entry LOOP
.entry LENGTH
.extern L3
.extern W
MAIN: mov S1.1.1, LENGTH
	  add r2, STR
LOOP: jmp END, r3
	  prn #-5,      r1
	  sub r1, r4
	  inc #40
	  mov S1.2, #3
	  bne LOOP
END:  stop
STR:    .string "abcdef"
LENGTH: .data 6,-9,       ,15
K:      .data 1,2,
S1:     .struct 8,"ab" 