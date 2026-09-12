#ifndef SMAC_SHARED_H
#define SMAC_SHARED_H

#define MAX_URL_LEN 2048
#define MAX_EXP_LEN 128
#define MAX_TOKEN_LEN 128

#define MAX_PATH_LEN 256

#define MAX_PROXY_SETTINGS_LEN 128

#define MAX_FILE_SIZE 1024 * 1024 // 1MB

int MAX_MAC_COUNT = 0;

bool CHECK_GENRE_MATCH = false;
bool CHECK_PLAYABLE = false;
char GENRE_PATTERN[MAX_TOKEN_LEN] = {0};

#define THREADS_LIMIT 32

#ifdef _WIN32
#define DEFAULT_RESULTS_DIR "..\\results"
#define DEFAULT_PATH_SEPARATOR "\\"
#else
#define DEFAULT_RESULTS_DIR "./results"
#define DEFAULT_PATH_SEPARATOR "/"
#endif

int NB_THREADS = 1; // user defined

char host[MAX_URL_LEN] = "http://localhost:8008";
//char host[MAX_URL_LEN] = {0};
char host_previous[MAX_URL_LEN] = {0};

char mac[STR_MAC_LEN]       = {0};
char next_mac[STR_MAC_LEN]  = {0};;
char mac_first[STR_MAC_LEN] = "00:1A:79:00:00:00";
char mac_last[STR_MAC_LEN]  = "00:1A:79:FF:FF:FF";

char mac_prefix[STR_MAC_LEN] = "00:1A:79";

bool AUTO_SAVE_ACCOUNTS = true;
bool USE_CHECKPOINTS    = true;

bool USE_PROXY          = false;
// https://curl.se/libcurl/c/CURLOPT_PROXY.html
char PROXY_MANUAL_URL[MAX_URL_LEN] = {0}; // if empty no proxy will be used even if declared in our make_request function
// note that it is also recommended to specify the port directly in the proxy_url
char PROXY_MANUAL_USERNAME[MAX_PROXY_SETTINGS_LEN] = {0};
char PROXY_MANUAL_PASSWORD[MAX_PROXY_SETTINGS_LEN] = {0};
char PROXY_FILE_FILEPATH[MAX_PATH_LEN]  = {0};
//char PROXY_FILE_URL[MAX_URL_LEN] = {0};
char PROXY_FILE_URL[MAX_URL_LEN] = "https://raw.githubusercontent.com/stormsia/proxy-list/main/http.txt";

enum {
    NONE,
    MANUAL,
    FROM_FILE,
    FROM_URL,
} Proxy_Mode;

int PROXY_MODE = NONE;

int MAX_REQUESTS_RETRY = -1; // disabled if < 0

char results_dir[MAX_URL_LEN] = DEFAULT_RESULTS_DIR;
char checkpoints_dir[MAX_URL_LEN] = DEFAULT_RESULTS_DIR DEFAULT_PATH_SEPARATOR "checkpoints";

char accounts_filename[MAX_URL_LEN+4] = {0};
char checkpoint_filename[MAX_URL_LEN+4] = {0};

/* request settings */
int request_delay = 0;
int request_timeout = 2 * 1000;
int pause_nb = 0;
int pause_duration = 10 * 1000;

/* proxy settings */
typedef struct {
    char *url;
    char *username;
    char *password;
} User_Proxy; // curl already took Proxy

enum {
    SEQUENTIAL,
    RANDOM,
    FROM_MAC_FILE,
} Scan_Mode;

char MAC_FILE_FILEPATH[MAX_PATH_LEN]  = {0};

int SCAN_MODE = SEQUENTIAL;
bool SCANNING = false;
bool GRACEFUL_EXIT_ASKED = false;

typedef struct {
    char *host;
    char *mac_first;
    char *mac_last;
    int scan_mode;
    int proxy_mode;
    User_Proxy proxy;
} Scan_Session;

// this is to avoid crashes updating the GUI
// maybe we also should ensure C++ does not 
// mangle our C function? but i am not sure 
// it is necessary, so everything is prepared
// just in case ;)

// #ifdef __cplusplus
// extern "C" {
// #endif

void GUI_update_scanning_labels(const char*);
void GUI_add_account_to_accounts_list(const char*,const char*);
void GUI_scan_ended_by_itself(void);

// #ifdef __cplusplus
// }
// #endif

#endif // SMAC_SHARED_H
