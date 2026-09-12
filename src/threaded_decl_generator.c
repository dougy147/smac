#include <stdio.h>
#include "shared.h"

void generate_write_callback_declarations() {
    FILE *f = fopen("./src/write_callback_declarations.h","w");
    for (int i=0;i<THREADS_LIMIT;i++) {
        fprintf(f,
                "size_t static write_callback_%d (void *buffer, size_t size, size_t nmemb, void *ptr) {\n"
                "    strcpy(responses[%d],(const char*)buffer);\n"
                "    return strlen(responses[%d]);\n"
                "}\n",i,i,i);
    }
    fclose(f);
}

void generate_write_callback_calls() {
    FILE *f = fopen("./src/write_callback_calls.h","w");
    fprintf(f, "if (thread_index < 0) return;\n");
    for (int i=0;i<THREADS_LIMIT;i++) {
        fprintf(f, "else if (thread_index == %d) curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback_%d);\n",i,i); 
    }
    fclose(f);
}

int main() {
    generate_write_callback_declarations();
    generate_write_callback_calls();
    return 0;
}
