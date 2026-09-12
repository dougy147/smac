if (thread_index < 0) return;
else if (thread_index == 0) curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback_0);
else if (thread_index == 1) curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback_1);
else if (thread_index == 2) curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback_2);
else if (thread_index == 3) curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback_3);
else if (thread_index == 4) curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback_4);
else if (thread_index == 5) curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback_5);
else if (thread_index == 6) curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback_6);
else if (thread_index == 7) curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback_7);
