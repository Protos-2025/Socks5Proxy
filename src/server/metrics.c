#include "include/metrics.h"

static struct metricSnapshot metrics;


void metrics_init() {
    metrics.currentConnections = 0;
    metrics.totalConnections = 0;
    metrics.totalBytesSent = 0;
    metrics.totalBytesReceived = 0;
}


void register_new_connection() {
    metrics.currentConnections++;
    metrics.totalConnections++;
}


void register_connection_closed() {
    if (metrics.currentConnections > 0) {
        metrics.currentConnections--;
    }
}


void register_bytes_transferred(int bytes_received, int bytes_sent) {
    metrics.totalBytesReceived += bytes_received;
    metrics.totalBytesSent += bytes_sent;
}


void get_metrics_snapshot(struct metricSnapshot *snapshot) {
    snapshot->currentConnections = metrics.currentConnections;
    snapshot->totalConnections = metrics.totalConnections;
    snapshot->totalBytesSent = metrics.totalBytesSent;
    snapshot->totalBytesReceived = metrics.totalBytesReceived;
}

