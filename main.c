#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>

typedef struct Response{
  char *memory;
  size_t size;
} Response;

size_t callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t totalSize = size * nmemb;
    Response *res = (Response*) userp;
    // The write callback may be called multiple times depending on response size.
    // Resize the buffer so that the incoming chunk fits.
    size_t new_mem = res->size + totalSize + 1;  // +1 for null terminator.
    char *ptr = realloc(res->memory, new_mem);
    if (ptr == NULL) {
        fprintf(stderr, "Failed to reallocate %ld bytes\n", new_mem);
        return 0;
    }
    res->memory = ptr;
    // Add chunk to the buffer.
    memcpy(&(res->memory[res->size]), contents, totalSize);
    res->size += totalSize;

    // Add null terminator.
    res->memory[res->size] = '\0';

    return totalSize;

}

void print_response(char *response){
    cJSON *root = cJSON_Parse(response);
    // cJSON stores
    if (!root) {
        printf("Error parsing JSON\n");
        return;
    }
    int arr_size = cJSON_GetArraySize(root);
    for (int i = 0; i < arr_size; i++){
        cJSON *item = cJSON_GetArrayItem(root, i);
        cJSON *type = cJSON_GetObjectItem(item, "type");
        cJSON *repo = cJSON_GetObjectItem(item, "repo");
        cJSON *repo_name = cJSON_GetObjectItem(repo, "name");

        if (cJSON_IsString(type) && cJSON_IsString(repo_name)) {
            printf("%s\n -> Repo: %s\n", type->valuestring, repo_name->valuestring);
        }
    }
}

char* build_url(char *username){
    char *base = "https://api.github.com/users/";
    char *suffix = "/events";
    // Build buffer
    char *url = malloc(strlen(base) + strlen(username) + strlen(suffix) +1);
    strcpy(url, base);
    strcat(url, username);
    strcat(url, suffix);

    return url;
}

int main(int argc, char *argv[]) {
  CURLcode ret;
  CURL *hnd;
  if (argc != 2) {
    printf("You should specify a single GitHub username.\n");
    return 1;
  }

  curl_global_init(CURL_GLOBAL_DEFAULT);

  Response chunk;
  chunk.memory = malloc(1);
  chunk.size = 0;

  char *url = build_url(argv[1]);
  hnd = curl_easy_init();
  if (hnd) {
    
    curl_easy_setopt(hnd, CURLOPT_URL, url);
    curl_easy_setopt(hnd, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(hnd, CURLOPT_ACCEPT_ENCODING, "");
    curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, callback);
    curl_easy_setopt(hnd, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(hnd, CURLOPT_USERAGENT, "test-consumer/1.0");
  }

  ret = curl_easy_perform(hnd);

  curl_easy_cleanup(hnd);
  hnd = NULL;
  if (ret == CURLE_OK) {
    print_response(chunk.memory);
  } else {
    fprintf(stderr, "Curl request failed: %s\n", curl_easy_strerror(ret));
  }

  free(url);
  free(chunk.memory);
  curl_global_cleanup();
  
  return (int)ret;
}