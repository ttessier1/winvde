#pragma once

#define SE 240 // End of negotiation(or data block) of a sub - service of a protocol mechanism
#define NOP 241	// Data packet that does nothing
#define DATAMARLK 242 // 
#define BREAK 243 //
#define INTPROC 244 // Request that other party ends current process
#define ABORTOUT 245 // Request that other party stops sending output
#define RUTHERE 246
#define ERASECHAR 247
#define ERASELINE 248
#define GAHEADE 249 // 
#define SB 250 // Initiate the negotiation of a sub - service of a protocol mechanism
#define WILL 251 // Informs other party that this party will use a protocol mechanism
#define WONT 252 //Informs other party that this party will not use a protocol mechanism	
#define DO 253 // Instruct other party to use a protocol mechanism
#define DONT 254 // Instruct other party to not use a protocol mechanism	
#define IAC 255	// Sequence Initializer / Escape Character

#define TELOPT_BINARY 0x00
#define TELOPT_ECHO 0x01
#define TELOPT_SGA 0x03
#define TELOPT_STATUS 0x05
#define TELOPT_TTYPE 0x18