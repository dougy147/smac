/* declarations of shared functions, macros, variables between main.c and src/gui.h */

#define MAC_LEN (2 * 6)
#define FULL_MAC_STR_LEN MAC_LEN + 5 + 1
#define FULL_ENCODED_MAC_STR_LEN MAC_LEN + (5 * 3) + 1
#define MAX_DNS_LEN       512
#define MAX_EXP_DATE_LEN  128

pthread_t SCAN_THREAD;

char server_url[MAX_DNS_LEN] = {0};
char mac[FULL_MAC_STR_LEN]   = {0}; // 00:AA:11:BB:22:CC\0
char exp_date[MAX_EXP_DATE_LEN] = {0};

enum {
    UNSET,
    SEQUENTIAL,
    RANDOM,
    MAC_FILE,
} Scan_Mode;

int SCAN_MODE = SEQUENTIAL;
int MAC_SCANNED_COUNT = 0;

#define set_server_url(DNS) \
    int i = 0; \
    int j = strlen((DNS)); \
    for (;i<strlen((DNS));i++) if ((DNS)[i] != ' ') break; \
    for (;j>0;j--) if ((DNS)[j-1] != ' ') break; \
    char trimmed[MAX_DNS_LEN] = {0};\
    for (int k=i;k<j;k++) trimmed[strlen(trimmed)] = (DNS)[k]; \
    strcpy(server_url,trimmed);

#define set_mac(MAC) \
    strcpy(mac,(MAC));\
    encode_mac((MAC));

/*main.c*/
static void scan_start(void);
static void scan_stop(void);

/*src/gui.h*/
static void GUI_update_mac_label(void);
static void GUI_add_to_accounts_listbox(void);
static void GUI_set_server_url_from_entry(void);
static void GUI_display_error_on_mac_label(char*);
static void GUI_scan_button_set_label(void);
static void GUI_scan_button_toggle(void);

