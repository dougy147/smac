main: main.c
	cc -o main main.c -lcurl

## for statically linking probing
#cc -Wall -Wextra -o http http.c C:/troll/test/curl-8.21.0/lib/.libs/libcurl.a -lssl -lcrypto -ldl -lm -lz -DCURL_STATICLIB 
