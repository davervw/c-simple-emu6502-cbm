#pragma once

#include "emu6502.h"

class MC6850
{
public:
	MC6850();
	~MC6850();
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