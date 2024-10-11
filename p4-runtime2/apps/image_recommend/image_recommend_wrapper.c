#define _GNU_SOURCE
#define __USE_GNU
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <net/skbuff.h>
#include <sys/socket.h>
#include <net/if.h>
#include <linux/if_packet.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <signal.h>

#include <Python.h>

PyObject *recommend_func;

#if LATENCY_BREAKDOWN
struct pktgen_tstamp {
	uint16_t magic;
    uint64_t send_start;
    uint64_t network_recv;
    uint64_t network_udp_input;
    uint64_t server_recv;
    uint64_t server_recvfrom;

    uint64_t app_total_time;
    uint64_t app_preprocess_time;
    uint64_t app_predict_time;
    uint64_t app_recommend_time;
    uint64_t app_compress_time;

    uint64_t server_sendto;
    uint64_t network_udp_output;
    uint64_t network_send;
} __attribute__((packed));
#endif

// #define MAX_PACKETS 64
// #define BUFFER_SIZE 4096

void sighandle(int signal) {
    Py_Finalize();
    exit(EXIT_SUCCESS);
}

#if LATENCY_BREAKDOWN
int image_recommand(struct pktgen_tstamp * ts, uint8_t * data, int len)
#else
int image_recommand(uint8_t * data, int len)
#endif
{
    PyObject *pBuf = PyBytes_FromStringAndSize((const char *)data, len);
    PyObject *pArgs = PyTuple_New(1);
    if (!pArgs) {
        PyErr_Print();
        Py_DECREF(pBuf);
        return 0; // Handle the error appropriately
    }

    if (PyTuple_SetItem(pArgs, 0, pBuf) != 0) { // PyTuple_SetItem steals the reference to pBuf
        PyErr_Print();
        Py_DECREF(pArgs); // This will also decrement the reference count of pBuf
        return 0; // Handle the error appropriately
    }

    PyObject *pValue = PyObject_CallObject(recommend_func, pArgs);
    if (pValue) {
        if (!PyDict_Check(pValue)) {
            fprintf(stderr, "Unexpected return type from Python function\n");
            Py_DECREF(pValue);
            Py_DECREF(pArgs);
            return 0;
        }

        // Extract latency data from the dictionary
        PyObject *pPreprocessTime = PyDict_GetItemString(pValue, "preprocess_time");
        PyObject *pPredictTime = PyDict_GetItemString(pValue, "predict_time");
        PyObject *pRecommendationTime = PyDict_GetItemString(pValue, "recommendation_time");
        PyObject *pTotalCompressionTime = PyDict_GetItemString(pValue, "total_compression_time");
        PyObject *pTotalTime = PyDict_GetItemString(pValue, "total_time");

        if (pPreprocessTime && pPredictTime && pRecommendationTime && pTotalCompressionTime && pTotalTime) {
            uint64_t preprocess_time = PyLong_AsUnsignedLongLong(pPreprocessTime);
            uint64_t predict_time = PyLong_AsUnsignedLongLong(pPredictTime);
            uint64_t recommendation_time = PyLong_AsUnsignedLongLong(pRecommendationTime);
            uint64_t total_compression_time = PyLong_AsUnsignedLongLong(pTotalCompressionTime);
            uint64_t total_time = PyLong_AsUnsignedLongLong(pTotalTime);

            // printf("Preprocess time: %llu ns\n", (unsigned long long)preprocess_time);
            // printf("Predict time: %llu ns\n", (unsigned long long)predict_time);
            // printf("Recommendation time: %llu ns\n", (unsigned long long)recommendation_time);
            // printf("Total compression time: %llu ns\n", (unsigned long long)total_compression_time);
            // printf("Total time: %llu ns\n", (unsigned long long)total_time);
#if LATENCY_BREAKDOWN
            ts->app_total_time = total_time;
            ts->app_preprocess_time = preprocess_time;
            ts->app_predict_time = predict_time;
            ts->app_recommend_time = recommendation_time;
            ts->app_compress_time = total_compression_time;
#endif
        } else {
            fprintf(stderr, "Failed to extract all required items from the dictionary\n");
        }

        Py_DECREF(pValue);
    } else {
        PyErr_Print();
    }

    return 0;
}

#define PORT 1234
#define BUFFER_SIZE 4096

int main(void) {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    char buffer[BUFFER_SIZE];
    socklen_t addr_len;
    ssize_t num_bytes;
    // int ret;

    // Set up signal handler for SIGINT
    signal(SIGINT, sighandle);

    // Initialize Python interpreter
    Py_Initialize();

    // Set Python path to include the directory with your module
    PyObject *sys_path = PySys_GetObject("path");
    PyObject *path = PyUnicode_DecodeFSDefault("/home/ubuntu/Nutcracker/p4-runtime2/apps/image_recommend/");
    if (sys_path && path) {
        PyList_Append(sys_path, path);
        Py_DECREF(path);
    } else {
        PyErr_Print();
        fprintf(stderr, "Failed to set Python path\n");
        Py_Finalize();
        return EXIT_FAILURE;
    }

    // Import the Python module
    PyObject *pName = PyUnicode_DecodeFSDefault("image_recommend");
    PyObject *pModule = PyImport_Import(pName);
    Py_DECREF(pName);
    if (!pModule) {
        PyErr_Print();
        fprintf(stderr, "Failed to load Python module\n");
        Py_Finalize();
        return EXIT_FAILURE;
    }

    // Get the reference to the image_recommend function
    recommend_func = PyObject_GetAttrString(pModule, "image_recommend");
    if (!recommend_func || !PyCallable_Check(recommend_func)) {
        if (PyErr_Occurred())
            PyErr_Print();
        fprintf(stderr, "Cannot find function 'image_recommend'\n");
        Py_DECREF(pModule);
        Py_Finalize();
        return EXIT_FAILURE;
    }

    // Create a UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Set up the server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;  // Listen on any interface
    server_addr.sin_port = htons(PORT);  // Bind to port 1234

    // Bind the socket to the specified port
    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    while (1) {
        addr_len = sizeof(client_addr);
        // Receive a UDP packet
        num_bytes = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&client_addr, &addr_len);
        if (num_bytes < 0) {
            perror("recvfrom");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        // Process the packet
        // printf("Received a packet of size %zd bytes from %s:%d\n", num_bytes,
        //        inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        // Pass the UDP payload to the Python function for image classification
#if LATENCY_BREAKDOWN
        struct pktgen_tstamp * ts = (struct pktgen_tstamp *)buffer;
        image_recommand(ts, (uint8_t *)&ts[1], num_bytes - sizeof(struct pktgen_tstamp));
#else
        image_recommand(&buffer[sizeof(struct pktgen_tstamp)], num_bytes - sizeof(struct pktgen_tstamp));
#endif
        // Send the packet back to the sender
        num_bytes = sendto(sockfd, buffer, num_bytes, 0, (struct sockaddr*)&client_addr, addr_len);
        if (num_bytes < 0) {
            perror("sendto");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        // printf("Sent a packet of size %zd bytes back to %s:%d\n", num_bytes,
        //     inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    }

    // Clean up
    close(sockfd);
    Py_DECREF(recommend_func);
    Py_DECREF(pModule);
    Py_Finalize();

    return 0;
}