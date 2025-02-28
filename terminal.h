bool term_notify(HWND hwnd, LPARAM lParam);
void setup_terminal_window(HWND hwnd, uint StatusBarHeight, uint bottom_ref_control);

void term_set_attr(uint fgnd, uint bgnd);
void term_put(char *term_str);
void term_append(char *term_str);
void term_replace(char *term_str);
uint term_get_columns(void);
uint term_get_rows(void);

