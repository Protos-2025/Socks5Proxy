#include "include/metrics.h"

static struct metricSnapshot metrics;


void metricsInit() {
    metrics.currentConnections = 0;
    metrics.totalConnections = 0;
    metrics.totalBytesSent = 0;
    metrics.totalBytesReceived = 0;
}


void registerNewConnection() {
    metrics.currentConnections++;
    metrics.totalConnections++;
}


void registerConnectionClosed() {
    if (metrics.currentConnections > 0) {
        metrics.currentConnections--;
    }
}


void registerBytesTransferred(int bytesReceived, int bytesSent) {
    metrics.totalBytesReceived += bytesReceived;
    metrics.totalBytesSent += bytesSent;
}


void getMetricsSnapshot(struct metricSnapshot *snapshot) {
    snapshot->currentConnections = metrics.currentConnections;
    snapshot->totalConnections = metrics.totalConnections;
    snapshot->totalBytesSent = metrics.totalBytesSent;
    snapshot->totalBytesReceived = metrics.totalBytesReceived;
}

