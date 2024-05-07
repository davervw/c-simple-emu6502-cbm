#pragma once

#include "emu6502.h"

// Derived from MC6850 datasheet
//
// Data Register 76543210 (use depends on bits, parity configuration)
// Control Register CR7 CR6 CR5 CR4 CR3 CR2 CR1 CR0
//                 rint rts tin? 8           0   0   /1
//                                      odd? 0   1   /16
//                                  1stop?   1   0   /64
//                                           1   1   RESET
//                               0   0   0           7e2
//                               0   0   1           7o2
//                               0   1   0           7e1
//                               0   1   1           7o1
//                               1   0   0           8n2
//                               1   0   1           8n1
//                               1   1   0           8e1
//                               1   1   1           8o1
//                       0   0                       rtsn low, transmitting interrupt disabled
//                       0   1                       rtsn low, transmitting interrupt enabled
//                       1   0                       rtsn high, transmitting interrupt disabled
//                       1   1                       rtsn low, transmits break on data output, transmitting interrupt disabled
//                   0                               receive interrupt disabled
//                   1                               receive interrupt enabled
// Status Register IRQN, PE, OVRN, FE, CTSN, DCD, TDRE, RDRF
//                 1=interrupt     1=framing error
//                       1=parity error           1=transmit data register empty
//                           1=overrun 0=ctsn 1=carrier  1=read data register full

class MC6850
{
	#pragma pack(push, 1)
	typedef union _STATUS
	{
		struct {
			unsigned rdrf : 1;
			unsigned tdre : 1;
			unsigned dcdn : 1;
			unsigned ctsn : 1;
			unsigned fe : 1;
			unsigned ovrn : 1;
			unsigned pe : 1;
			unsigned irqn : 1;
		};
		byte value;
	} STATUS;
	#pragma pack(pop)

	#pragma pack(push, 1)
	typedef union _CONTROL
	{
		struct {
			unsigned clk : 2; // CLOCK
			unsigned mode : 3; // MODE
			unsigned rts_tint : 2; // RTS_TINT
			unsigned rint : 1;
		};
		byte value;
	} CONTROL;
	#pragma pack(pop)

	typedef enum _RTS_TINT {
		low_disabled = 0,
		low_enabled = 1,
		high_disabled = 2,
		low_break_disabled=3
	} RTS_TINT;

	typedef enum _MODE { 
		b7e2=0,
		b7o2=1,
		b7e1=2,
		b7o1=3,
		b8n2=4,
		b8n1=5,
		b8e1=6,
		b8o1=7
	} MODE;

	typedef enum _CLOCK {
		DIV1=0,
		DIV16=1,
		DIV64=2,
		RESET=3
	} CLOCK;

public:
	MC6850();
	~MC6850();
	byte read_data();
	void write_data(byte value);
	byte read_status() const;
	void write_control(byte value);
	bool read_irq() const;
private:
	void clear_irq();
	void set_irq();
	void clear_receive_data_register_full();
	void set_receive_data_register_full();
	void clear_transmit_data_register_empty();
	void set_transmit_data_register_empty();
private:
	CONTROL control;
	STATUS status;
};