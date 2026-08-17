#ifndef SMAC_UTILS_H
#define SMAC_UTILS_H

bool is_whitespace(char c) {
    const char *ws = " \t\n\r";
    while (ws[0] != '\0') if (c == *ws++) return true;
    return false;
}

void trim(char *s) {
    char *p = s;
    int l = strlen(s);
    while (p[0] != '\0' && is_whitespace(p[0])) p++;
    for (int i=0;i<=l-(p-s);i++) s[i] = s[i+(p-s)];
    while (l-1 >= 0 && is_whitespace(s[l-1])) l--;
    s[l] = '\0';
}

void path_to_windows_path(char *path) {
    char *p = path;
    char win_path[MAX_URL_LEN] = {0};
    while (p[0] != '\0') {
        if (strlen(win_path) == MAX_URL_LEN) {
            // TODO: properly alert user we are not changing the path because of this:
            printf("[!] Could not convert to Windows path: buffer overflow, path too long");
            strcpy(win_path,path);
            break;
        }
        if (p[0] == '/') {
            win_path[strlen(win_path)] = '\\';
            win_path[strlen(win_path)] = '\\';
        } else {
            win_path[strlen(win_path)] = p[0];
        }
        p++;
    }
    win_path[strlen(win_path)] = '\0';
    strcpy(path,win_path);
}

#endif //SMAC_UTILS_H
