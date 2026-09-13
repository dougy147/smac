#include <assert.h>

#define ARG_HOST_URL_LONG  "--url"
#define ARG_HOST_URL_SHORT "-u"

#define ARG_SEQUENTIAL_SCAN_LONG "--seq"
#define ARG_RANDOM_SCAN_LONG "--random"

#define ARG_MAC_FIRST_LONG  "--first"
#define ARG_MAC_FIRST_SHORT "-F"
#define ARG_MAC_LAST_LONG   "--last"
#define ARG_MAC_LAST_SHORT  "-L"

#define ARG_GENRE_MATCH_LONG "--genre-match"

#define ARG_PLAYABLE_LONG "--playable"

#define ARG_STOP_COUNT_LONG "--stop"

#define ARG_MAC_FILE_LONG "--mac-file"

#define ARG_MAX_RETRY_LONG "--max-retry"

#define ARG_SAVE_DIR_LONG "--save-dir"

#define ARG_THREADS_LONG "--threads"

#define ARG_NO_CHECKPOINT_LONG "--no-checkpoint"

#define ARG_MAC_PREFIX_LONG "--prefix"

#define ARG_PROXY_FROM_URL_LONG "--proxy-from-url"

#define ARG_PROXY_FILE_LONG "--proxy-file"

#define ARG_PROXY_PASSWORD_LONG "--proxy-password"

#define ARG_PROXY_USERNAME_LONG "--proxy-username"

#define ARG_MANUAL_PROXY_LONG "--proxy"

#define ARG_PAUSE_DELAY_LONG "--pause-for"

#define ARG_PAUSE_COUNT_LONG "--pause"  

#define ARG_REQUEST_TIMEOUT_LONG "--timeout"

#define ARG_REQUEST_DELAY_LONG "--delay"

void shift(int *argc, char ***argv) {
    assert(*argc > 1);
    *argc-=1;
    *argv+=1;
}

#define shift() shift(&argc, &argv);

#define arg_is(ARG,STR) strcmp(ARG,STR) == 0

void parse_args(int argc, char **argv) {
    char *prog_name = argv[0];
    
    while (argc > 1) {
        shift();
        
        char *arg = argv[0];
        
        if (arg_is(arg,ARG_HOST_URL_SHORT) || arg_is(arg,ARG_HOST_URL_LONG)) {
            shift();
            strcpy(host, argv[0]);
        }

        else if (arg_is(arg,ARG_SEQUENTIAL_SCAN_LONG)) {
            SCAN_MODE = SEQUENTIAL;
        }

        else if (arg_is(arg,ARG_RANDOM_SCAN_LONG)) {
            SCAN_MODE = RANDOM;
        }
        
        else if (arg_is(arg,ARG_MAC_FIRST_LONG)) {
            shift();
            strcpy(mac_first, argv[0]);
        }
                
        else if (arg_is(arg,ARG_MAC_LAST_LONG)) {
            shift();
            strcpy(mac_last, argv[0]);
        }

        else if (arg_is(arg,ARG_REQUEST_DELAY_LONG)) {
            shift();
            request_delay = atoi(argv[0]);
        }
                
        else if (arg_is(arg,ARG_REQUEST_TIMEOUT_LONG)) {
            shift();
            request_timeout = atoi(argv[0]);
        }

        else if (arg_is(arg,ARG_PAUSE_COUNT_LONG)) {
            shift();
            pause_nb = atoi(argv[0]);
        }
        
        else if (arg_is(arg,ARG_PAUSE_DELAY_LONG)) {
            shift();
            pause_duration = atoi(argv[0]);
        }
        
        else if (arg_is(arg,ARG_MANUAL_PROXY_LONG)) {
            shift();
            USE_PROXY = true;
            if (PROXY_MODE == NONE) PROXY_MODE = MANUAL;
            strcpy(PROXY_MANUAL_URL,argv[0]);
        }

        else if (arg_is(arg,ARG_PROXY_USERNAME_LONG)) {
            shift();
            USE_PROXY = true;
            if (PROXY_MODE == NONE) PROXY_MODE = MANUAL;
            strcpy(PROXY_MANUAL_USERNAME,argv[0]);
        }
        
        else if (arg_is(arg,ARG_PROXY_PASSWORD_LONG)) {
            shift();
            USE_PROXY = true;
            if (PROXY_MODE == NONE) PROXY_MODE = MANUAL;
            strcpy(PROXY_MANUAL_PASSWORD,argv[0]);
        }
                
        else if (arg_is(arg,ARG_PROXY_FILE_LONG)) {
            shift();
            USE_PROXY = true;
            PROXY_MODE = FROM_FILE;
            strcpy(PROXY_FILE_FILEPATH,argv[0]);
        }

        else if (arg_is(arg,ARG_PROXY_FROM_URL_LONG)) {
            shift();
            USE_PROXY = true;
            PROXY_MODE = FROM_URL;
            strcpy(PROXY_FILE_URL,argv[0]);
        }

        else if (arg_is(arg,ARG_MAC_PREFIX_LONG)) {
            strcpy(mac_prefix,argv[0]);
        }
                
        else if (arg_is(arg,ARG_NO_CHECKPOINT_LONG)) {
            USE_CHECKPOINTS = false;
        }
        
        else if (arg_is(arg,ARG_THREADS_LONG)) {
            shift();
            NB_THREADS = atoi(argv[0]);
            assert(NB_THREADS <= THREADS_LIMIT);
        }
                                        
        else if (arg_is(arg,ARG_SAVE_DIR_LONG)) {
            shift();
            strcpy(results_dir,argv[0]);
            strcpy(checkpoints_dir,results_dir);
            strcat(checkpoints_dir,DEFAULT_PATH_SEPARATOR "checkpoints");
        }
                                    
        else if (arg_is(arg,ARG_MAX_RETRY_LONG)) {
            shift();
            MAX_REQUESTS_RETRY = atoi(argv[0]);
        }

        else if (arg_is(arg,ARG_MAC_FILE_LONG)) {
            shift();
            SCAN_MODE = FROM_MAC_FILE;
            strcpy(MAC_FILE_FILEPATH,argv[0]);
        }
        
        else if (arg_is(arg,ARG_STOP_COUNT_LONG)) {
            shift();
            MAX_MAC_COUNT = atoi(argv[0]);
        }

        else if (arg_is(arg,ARG_GENRE_MATCH_LONG)) {
            shift();
            CHECK_GENRE_MATCH = true;
            strcpy(GENRE_PATTERN,argv[0]);
        }

        else if (arg_is(arg,ARG_PLAYABLE_LONG)) {
            CHECK_PLAYABLE = true;
        }

        else if (arg_is(arg,"-h") || arg_is(arg,"--help") || arg_is(arg, "-help") || arg_is(arg, "help")) {
            // sure we never miss a help request x')
            usage(0);
        }

        // to be continued

        else {
            // --url is the only mandatory variable
            // consider "untagged" argument as --url "host"
            // only if "host" was not defined yet
            if (strlen(host) == 0) {
                strcpy(host, arg);
            } else {
                fprintf(stderr, "[!] Unknown argument: %s\n", arg);
                usage(1);
            }
        }
    }

    if (strlen(host) == 0) {
        fprintf(stderr,"[!] No target server was provided\n");
        usage(1);
    //} else {
    //    printf("scanning host = %s\n", host);
    }
}
