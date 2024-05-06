#pragma once

#include "emu6502.h"

class M6850
{
public:
	M6850();
	~M6850();
	byte read_data();
	void write_data(byte value);
	byte read_status();
	void write_control(byte value);
	bool read_irq();
	void clear_irq();
	void set_irq();
	void clear_receive_data_register_full();
	void set_receive_data_register_full();
	void clear_transmit_data_register_empty();
	void set_transmit_data_register_empty();
private:
	byte control;
	byte status;
};