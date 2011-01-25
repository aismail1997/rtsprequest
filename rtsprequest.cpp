/*
 * Copyright (c) 2011, Jim Hollinger
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Jim Hollinger nor the names of its contributors
 *     may be used to endorse or promote products derived from this
 *     software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>  // _getch()

#include <curl/curl.h>

#define VERSION_STR  "V1.0"

// error handling macros
#define my_curl_easy_setopt(A,B,C) \
  if((res = curl_easy_setopt((A),(B),(C))) != CURLE_OK) \
    fprintf(stderr, "curl_easy_setopt(%s, %s, %s) failed: %d\n", #A, #B, #C, res);

#define my_curl_easy_perform(A) \
  if((res = curl_easy_perform((A))) != CURLE_OK) \
    fprintf(stderr, "curl_easy_perform(%s) failed: %d\n", #A, res);


// send RTSP OPTIONS request
void rtsp_options(CURL *curl, const char *uri) {
	CURLcode res = CURLE_OK;
	printf("\nRTSP: OPTIONS %s\n", uri);
	my_curl_easy_setopt(curl, CURLOPT_RTSP_STREAM_URI, uri);
	my_curl_easy_setopt(curl, CURLOPT_RTSP_REQUEST, CURL_RTSPREQ_OPTIONS);
	my_curl_easy_perform(curl);
}


// send RTSP DESCRIBE request and write sdp response to a file
void rtsp_describe(CURL *curl, const char *uri) {
	CURLcode res = CURLE_OK;
	printf("\nRTSP: DESCRIBE %s\n", uri);
	bool sdp_filename_new = false;
	char *sdp_filename = NULL;
	const char *s = strrchr(uri, '/');
	if (s != NULL) {
		s++;
		if (s[0] != '\0') {
			sdp_filename = new char[strlen(s) + 8];
			if (sdp_filename != NULL) {
				sprintf(sdp_filename, "%s.sdp", s);
				sdp_filename_new = true;
			}
		}
	}
	if (sdp_filename == NULL) {
		sdp_filename = "video.sdp";
	}
	FILE *sdp_fp = fopen(sdp_filename, "wt");
	if (sdp_fp == NULL) {
		fprintf(stderr, "Could not open '%s' for writing\n", sdp_filename);
		sdp_fp = stdout;
	} else {
		printf("Writing SDP to '%s'\n", sdp_filename);
	}
	my_curl_easy_setopt(curl, CURLOPT_WRITEDATA, sdp_fp);
	my_curl_easy_setopt(curl, CURLOPT_RTSP_REQUEST, CURL_RTSPREQ_DESCRIBE);
	my_curl_easy_perform(curl);
	my_curl_easy_setopt(curl, CURLOPT_WRITEDATA, stdout);
	fclose(sdp_fp);
	if (sdp_filename_new) {
		delete []sdp_filename;
	}
}


// send RTSP SETUP request
void rtsp_setup(CURL *curl, const char *uri, const char *transport) {
	CURLcode res = CURLE_OK;
	printf("\nRTSP: SETUP %s\n", uri);
	printf(  "      TRANSPORT %s\n", transport);
	my_curl_easy_setopt(curl, CURLOPT_RTSP_STREAM_URI, uri);
	my_curl_easy_setopt(curl, CURLOPT_RTSP_TRANSPORT, transport);
	my_curl_easy_setopt(curl, CURLOPT_RTSP_REQUEST, CURL_RTSPREQ_SETUP);
	my_curl_easy_perform(curl);
}


// send RTSP PLAY request
void rtsp_play(CURL *curl, const char *uri, const char *range) {
	CURLcode res = CURLE_OK;
	printf("\nRTSP: PLAY %s\n", uri);
	my_curl_easy_setopt(curl, CURLOPT_RTSP_STREAM_URI, uri);
	my_curl_easy_setopt(curl, CURLOPT_RANGE, range);
	my_curl_easy_setopt(curl, CURLOPT_RTSP_REQUEST, CURL_RTSPREQ_PLAY);
	my_curl_easy_perform(curl);
}


// send RTSP TEARDOWN request
void rtsp_teardown(CURL *curl, const char *uri) {
	CURLcode res = CURLE_OK;
	printf("\nRTSP: TEARDOWN %s\n", uri);
	my_curl_easy_setopt(curl, CURLOPT_RTSP_REQUEST, CURL_RTSPREQ_TEARDOWN);
	my_curl_easy_perform(curl);
}


// main app
int main(int argc, char **argv) {
	const char *transport = "RTP/AVP;unicast;client_port=1234-1235";
	const char *range = "0.000-";
	int rc = EXIT_SUCCESS;

	printf("\nRTSP request %s\n", VERSION_STR);
	printf("    Project web site: http://code.google.com/p/rtsprequest/\n");
	printf("    Requires cURL V7.20 or greater\n\n");

	// check command line
	char *basename = NULL;
	if ((argc != 2) && (argc != 3)) {
		basename = strrchr(argv[0], '/');
		if (basename == NULL) {
			basename = strrchr(argv[0], '\\');
		}
		if (basename == NULL) {
			basename = argv[0];
		} else {
			basename++;
		}
		printf("Usage:   %s url [transport]\n", basename);
		printf("         url of video server\n");
		printf("         transport (optional) specifier for media stream protocol\n");
		printf("         default transport: %s\n", transport);
		printf("Example: %s rtsp://192.168.0.2/media/video1\n\n", basename);
		rc = EXIT_FAILURE;
	} else {
		const char *url = argv[1];
		char *uri = new char[strlen(url) + 32];
		if (argc == 3) {
			transport = argv[2];
		}

		// initialize curl
		CURLcode res = CURLE_OK;
		res = curl_global_init(CURL_GLOBAL_ALL);
		if (res == CURLE_OK) {
			curl_version_info_data *data = curl_version_info(CURLVERSION_NOW);
			fprintf(stderr, "    cURL V%s loaded\n", data->version);

			// initialize this curl session
			CURL *curl = curl_easy_init();
			if (curl != NULL) {
				my_curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
				my_curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
				my_curl_easy_setopt(curl, CURLOPT_WRITEHEADER, stdout);
				my_curl_easy_setopt(curl, CURLOPT_URL, url);

				// request server options
				sprintf(uri, "%s", url);
				rtsp_options(curl, uri);

				// open sdp file and request session description
				rtsp_describe(curl, uri);

				// setup media stream
				sprintf(uri, "%s/video", url);
				rtsp_setup(curl, uri, transport);

				// start playing media stream
				sprintf(uri, "%s/", url);
				rtsp_play(curl, uri, range);
				printf("Playing video, press any key to stop ...");
				_getch();
				printf("\n");

				// teardown session
				rtsp_teardown(curl, uri);

				// cleanup
				curl_easy_cleanup(curl);
				curl = NULL;
			} else {
				fprintf(stderr, "curl_easy_init() failed\n");
			}
			curl_global_cleanup();
		} else {
			fprintf(stderr, "curl_global_init(%s) failed\n", "CURL_GLOBAL_ALL", res);
		}
	}

	return rc;
}