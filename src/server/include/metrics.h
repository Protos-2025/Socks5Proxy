#ifndef __METRICS_H__
#define __METRICS_H__

#include <stddef.h>
struct metricSnapshot {

  size_t currentConnections;

  size_t totalConnections;

  size_t totalBytesSent;

  size_t totalBytesReceived;
};

void metrics_init();

void register_new_connection();

void register_connection_closed();

void register_bytes_transferred(int bytes_sent, int bytes_received);

void get_metrics_snapshot(struct metricSnapshot *snapshot);


#endif
