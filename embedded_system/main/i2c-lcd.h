void lcd_init(void); // initialize lcd

void lcd_send_cmd(char cmd); // send command to the lcd

void lcd_send_data(char data); // send data to the lcd

void lcd_send_string(char *str); // send string to the lcd

void lcd_continuous_scroll(char *str, int row, int delay_ms, int cycles); // scroll string on the lcd

void lcd_stop_scroll(void); // Stop any active scrolling

void lcd_put_cur(int row, int col); // put cursor at the entered position row (0 or 1), col (0-15);

void lcd_clear(void);