#pragma once


#ifdef __cplusplus
extern "C" {
#endif

size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp);

int compareVersions(const char *localVersion, const char *onlineVersion);

// Modify downloadFile function to create directory if needed
void downloadFile(const char *url, const char *outputFilename);

void downloadFilesFromVersionFile(const char *versionFileURL, const char *localVersion);

void UpdateIsConfirmed();

#ifdef __cplusplus
}
#endif