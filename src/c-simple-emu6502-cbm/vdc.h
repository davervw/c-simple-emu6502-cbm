class VDC8563
{
private:
	byte* registers;
	byte* vdc_ram;
	byte register_addr;
	byte data;
	bool ready = false;
  bool active = false;

public:
	VDC8563();
	~VDC8563();

	byte GetAddressRegister();
	void SetAddressRegister(byte value);
	byte GetDataRegister();
	void SetDataRegister(byte value);
  void Activate();
  void Deactivate();
  int VDCColorToLCDColor(byte value);
  void DrawChar(byte c, int col, int row, int fg, int bg, byte attrib);
  void DrawChar(int offset);
  void RedrawScreen();
  // void BlinkCursor();
  // void HideCursor();
  // void ShowCursor();
};
