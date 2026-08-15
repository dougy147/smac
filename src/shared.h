#ifndef SMAC_SHARED_H
#define SMAC_SHARED_H

#define MAX_URL_LEN 2048
#define MAX_EXP_LEN 128
#define MAX_TOKEN_LEN 128

#define MAX_PROXY_SETTINGS_LEN 128

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
#ifdef _WIN32
    char output_dir[MAX_URL_LEN] = "..\\results";
    char output_dir_checkpoints[MAX_URL_LEN] = "..\\results\\checkpoints";
#else
    char output_dir[MAX_URL_LEN] = "./results";
    char output_dir_checkpoints[MAX_URL_LEN] = "./results/checkpoints";
#endif
char output_filename_accounts[MAX_URL_LEN+4] = {0};
char output_filename_checkpoints[MAX_URL_LEN+4] = {0};

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
} UserProxy; // curl already took Proxy

// https://curl.se/libcurl/c/CURLOPT_PROXY.html
char proxy_url[MAX_URL_LEN] = {0}; // if empty no proxy will be used even if declared in our make_request function
// note that it is also recommended to specify the port directly in the proxy_url
char proxy_username[MAX_PROXY_SETTINGS_LEN] = {0};
char proxy_password[MAX_PROXY_SETTINGS_LEN] = {0};

enum {
    SEQUENTIAL,
    RANDOM,
} Scan_Mode;

int SCAN_MODE = SEQUENTIAL;

// this is to avoid crashes updating the GUI
// maybe we also should ensure C++ does not 
// mangle our C function? but i am not sure 
// it is necessary, so everything is prepared
// just in case ;)

//#ifdef __cplusplus
//extern "C" {
//#endif

void GUI_update_scanning_labels(const char*);
void GUI_add_account_to_accounts_list(const char*,const char*);

//#ifdef __cplusplus
//}
//#endif

#endif // SMAC_SHARED_H
