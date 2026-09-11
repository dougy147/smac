#include <assert.h>

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
        
        if (arg_is(arg,"-u") || arg_is(arg,"--url")) {
            shift();
            strcpy(host, argv[0]);
        }

        else if (arg_is(arg,"--seq")) {
            SCAN_MODE = SEQUENTIAL;
        }

        else if (arg_is(arg,"--random")) {
            SCAN_MODE = RANDOM;
        }
        
        else if (arg_is(arg,"-F") || arg_is(arg,"--from")) {
            shift();
            strcpy(mac_first, argv[0]);
        }
        
        else if (arg_is(arg,"-L") || arg_is(arg,"--to")) {
            shift();
            strcpy(mac_last, argv[0]);
        }

        else if (arg_is(arg,"-w") || arg_is(arg,"--wait")) {
            shift();
            request_delay = atoi(argv[0]);
        }
        
        else if (arg_is(arg,"-t") || arg_is(arg,"--timeout")) {
            shift();
            request_timeout = atoi(argv[0]);
        }
        
        else if (arg_is(arg,"-b") || arg_is(arg,"--break")) {
            shift();
            pause_nb = atoi(argv[0]);
        }
        
        else if (arg_is(arg,"-d") || arg_is(arg,"--pause-for")) {
            shift();
            pause_duration = atoi(argv[0]);
        }

        else if (arg_is(arg,"-P") || arg_is(arg,"--proxy")) {
            shift();
            USE_PROXY = true;
            if (PROXY_MODE == NONE) PROXY_MODE = MANUAL;
            strcpy(PROXY_MANUAL_URL,argv[0]);
        }
        
        else if (arg_is(arg,"-Pu") || arg_is(arg,"--proxy-user")) {
            shift();
            USE_PROXY = true;
            if (PROXY_MODE == NONE) PROXY_MODE = MANUAL;
            strcpy(PROXY_MANUAL_USERNAME,argv[0]);
        }
        
        else if (arg_is(arg,"-Pp") || arg_is(arg,"--proxy-password")) {
            shift();
            USE_PROXY = true;
            if (PROXY_MODE == NONE) PROXY_MODE = MANUAL;
            strcpy(PROXY_MANUAL_PASSWORD,argv[0]);
        }

        else if (arg_is(arg,"-Pfile") || arg_is(arg,"--proxy-file")) {
            shift();
            USE_PROXY = true;
            PROXY_MODE = FROM_FILE;
            strcpy(PROXY_FILE_FILEPATH,argv[0]);
        }

        else if (arg_is(arg,"-Purl") || arg_is(arg,"--proxy-from-url")) {
            shift();
            USE_PROXY = true;
            PROXY_MODE = FROM_URL;
            strcpy(PROXY_FILE_URL,argv[0]);
        }
        
        else if (arg_is(arg,"--prefix")) {
            strcpy(mac_prefix,argv[0]);
        }
        
        else if (arg_is(arg,"--no-checkpoint")) {
            USE_CHECKPOINTS = false;
        }

        else if (arg_is(arg,"--threads")) {
            shift();
            NB_THREADS = atoi(argv[0]);
        }

        else if (arg_is(arg,"--results-dir")) {
            shift();
            strcpy(results_dir,argv[0]);
            strcpy(checkpoints_dir,results_dir);
            strcat(checkpoints_dir,DEFAULT_PATH_SEPARATOR "checkpoints");
        }

        else if (arg_is(arg,"--max-retry")) {
            shift();
            MAX_REQUESTS_RETRY = atoi(argv[0]);
        }
        
        else if (arg_is(arg,"--mac-file")) {
            shift();
            SCAN_MODE = FROM_MAC_FILE;
            strcpy(MAC_FILE_FILEPATH,argv[0]);
        }

        else if (arg_is(arg,"--stop")) {
            shift();
            MAX_MAC_COUNT = atoi(argv[0]);
        }

        else if (arg_is(arg,"--genre-regex")) {
            // TODO: change name: this is not a regex!!!
            shift();
            CHECK_GENRE_MATCH = true;
            strcpy(GENRE_PATTERN,argv[0]);
        }

        else if (arg_is(arg,"--playable")) {
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
                fprintf(stderr, "[!] Unknown argument: %s\n", arg);
                usage(1);
            } else {
                strcpy(host, arg);
            }
        }
    }

    if (strlen(host) == 0) {
        fprintf(stderr,"[!] No target server was provided\n");
        usage(1);
    } else {
        printf("scanning host = %s\n", host);
    }
}
