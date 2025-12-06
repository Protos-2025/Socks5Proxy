#ifndef __METRICS_H__
#define __METRICS_H__

#include <stddef.h>
struct metricSnapshot {

  size_t currentConnections;

  size_t totalConnections;

  size_t totalBytesSent;

  size_t totalBytesReceived;
};

void metricsInit();

void registerNewConnection();

void registerConnectionClosed();

void registerBytesTransferred(int bytesSent, int bytesReceived);

void getMetricsSnapshot(struct metricSnapshot *snapshot);


#endif
