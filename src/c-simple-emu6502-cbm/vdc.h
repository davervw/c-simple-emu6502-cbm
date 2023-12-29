class VDC8563
{
private:
	byte* registers;
	byte* vdc_ram;
	byte register_addr;
	byte data;
	bool ready = false;

public:
	VDC8563();
	~VDC8563();

	byte GetAddressRegister();
	void SetAddressRegister(byte value);
	byte GetDataRegister();
	void SetDataRegister(byte value);
};
