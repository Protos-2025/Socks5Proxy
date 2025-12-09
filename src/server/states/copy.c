#include "copy.h"
#include "socks5nio.h"
#include "logger.h"
#include "errno.h"
#include <string.h>
#include "selector.h"

#include <fcntl.h>

#define IS_CLIENT_DATA(connection, key) (connection->client_fd == key->fd)

static int update_target_interests(FdSelector s, CopySt * target) {
    FdInterest ret = OP_NOOP; /* interests for target->fd */
    FdInterest oet = OP_NOOP; /* interests for target->otherCopySt->fd */
    CopySt *other = target->otherCopySt;

    // target can read if target's buffer has space to write
    if (buffer_can_write(other->buffer)) {
		ret |= OP_READ;
	}

    // target can write if target's buffer has data to read
    if (buffer_can_read(target->buffer)) {
		ret |= OP_WRITE;
	}

    // other can read if other's buffer has space to write
    if (buffer_can_write(target->buffer)) {
		oet |= OP_READ;
	}

    // other can write if other's buffer has data to read
    if (buffer_can_read(other->buffer)) {
		oet |= OP_WRITE;
	}

	if ((SELECTOR_SUCCESS != selector_set_interest(s, target->fd, ret)) || (SELECTOR_SUCCESS != selector_set_interest(s, other->fd, oet))) {
        LOG_ERROR("Failed to update interests for fd=%d", target->fd);
		return -1;
	};

    target->interests = ret;
    other->interests = oet;
	return ret;
}

void socksv5_copy_arrival(const unsigned int state, struct selector_key * key) {
    struct socks5 * connection = ATTACHMENT(key);
	CopySt * clientCopy = &connection->client.copy;
    CopySt * originCopy = &connection->origin_st.copy;

    clientCopy->buffer = &connection->client_buffer;
    clientCopy->fd = connection->client_fd;
    
    originCopy->buffer = &connection->origin_buffer;
    originCopy->fd = connection->origin_fd;

    originCopy->otherCopySt = clientCopy;
    clientCopy->otherCopySt = originCopy;

    LOG_TRACE("Entering COPY state. client_fd=%d, origin_fd=%d", clientCopy->fd, originCopy->fd);
    clientCopy->interests = OP_READ;
    originCopy->interests = OP_READ;
    selector_set_interest(key->s, clientCopy->fd, clientCopy->interests);
    selector_set_interest(key->s, originCopy->fd, originCopy->interests);
}

unsigned socksv5_copy_read(struct selector_key * key) {
	struct socks5* connection = ATTACHMENT(key);
    CopySt * from = IS_CLIENT_DATA(connection, key) ? &connection->client.copy : &connection->origin_st.copy;
	CopySt * to = from->otherCopySt;
    
    if (!buffer_can_write(to->buffer)) {
        return COPY;
    }
    
    LOG_TRACE("Attempting READ data from %s to %s buffer (fd=%d to fd=%d)", IS_CLIENT_DATA(connection, key) ? "client" : "origin", IS_CLIENT_DATA(connection, key) ? "origin" : "client", from->fd, to->fd);
    
    size_t canWrite = 0;
	uint8_t* writePtr = buffer_write_ptr(to->buffer, &canWrite);

	ssize_t readBytes = recv(from->fd, writePtr, canWrite, 0);

    LOG_TRACE("%s interests before read from %s: OP_READ=%s, OP_WRITE=%s", IS_CLIENT_DATA(connection, key) ? "origin" : "client", IS_CLIENT_DATA(connection, key) ? "client" : "origin", (to->interests & OP_READ) ? "true" : "false", (to->interests & OP_WRITE) ? "true" : "false");
	LOG_TRACE("%s interests before read from %s: OP_READ=%s, OP_WRITE=%s", IS_CLIENT_DATA(connection, key) ? "client" : "origin", IS_CLIENT_DATA(connection, key) ? "client" : "origin", (from->interests & OP_READ) ? "true" : "false", (from->interests & OP_WRITE) ? "true" : "false");

    if (readBytes > 0) {
        LOG_TRACE("Read %zd bytes from %s... writing to %s buffer (fd=%d into fd=%d): '%.*s'", readBytes, IS_CLIENT_DATA(connection, key) ? "client" : "origin", IS_CLIENT_DATA(connection, key) ? "origin" : "client", from->fd, to->fd, (int)readBytes, writePtr);
        buffer_write_adv(to->buffer, readBytes);
        if (update_target_interests(key->s, from) < 0) {
            LOG_ERROR("Failed to update interests after read");
            return ERROR;
        };
    } else {
        LOG_TRACE("Couldn't read from %s (fd=%d)", IS_CLIENT_DATA(connection, key) ? "client" : "origin", from->fd);
        if (readBytes < 0) {
            LOG_WARN("Read error (%d) from fd=%d: %s", errno, from->fd, strerror(errno));
        }
		shutdown(from->fd, SHUT_RD);
        from->interests &= ~OP_READ;
        selector_set_interest(key->s, from->fd, from->interests);
        shutdown(to->fd, SHUT_WR);
        to->interests &= ~OP_WRITE;
        selector_set_interest(key->s, to->fd, to->interests);
	}

    LOG_TRACE("%s interests after read from %s: OP_READ=%s, OP_WRITE=%s", IS_CLIENT_DATA(connection, key) ? "origin" : "client", IS_CLIENT_DATA(connection, key) ? "client" : "origin", (to->interests & OP_READ) ? "true" : "false", (to->interests & OP_WRITE) ? "true" : "false");
	LOG_TRACE("%s interests after read from %s: OP_READ=%s, OP_WRITE=%s", IS_CLIENT_DATA(connection, key) ? "client" : "origin", IS_CLIENT_DATA(connection, key) ? "client" : "origin", (from->interests & OP_READ) ? "true" : "false", (from->interests & OP_WRITE) ? "true" : "false");
    return (from->interests | to->interests) ^ OP_NOOP ? COPY : DONE;
}

unsigned socksv5_copy_write(struct selector_key * key) {
    struct socks5 * connection = ATTACHMENT(key);
	CopySt* from = IS_CLIENT_DATA(connection, key) ? &connection->client.copy : &connection->origin_st.copy;
	int toFd = from->fd;

	if (!buffer_can_read(from->buffer)) {
        return COPY;
	}
   
    LOG_TRACE("Attempting WRITE data to %s (fd=%d)", IS_CLIENT_DATA(connection, key) ? "client" : "origin", toFd);

	size_t canRead = 0;
    uint8_t* readPtr = buffer_read_ptr(from->buffer, &canRead);

    ssize_t writtenBytes = send(toFd, readPtr, canRead, 0);

    LOG_TRACE("%s interests before write to %s: OP_READ=%s, OP_WRITE=%s", IS_CLIENT_DATA(connection, key) ? "origin" : "client", IS_CLIENT_DATA(connection, key) ? "client" : "origin", (from->otherCopySt->interests & OP_READ) ? "true" : "false", (from->otherCopySt->interests & OP_WRITE) ? "true" : "false");
    LOG_TRACE("%s interests before write to %s: OP_READ=%s, OP_WRITE=%s", IS_CLIENT_DATA(connection, key) ? "client" : "origin", IS_CLIENT_DATA(connection, key) ? "client" : "origin", (from->interests & OP_READ) ? "true" : "false", (from->interests & OP_WRITE) ? "true" : "false");

    if (writtenBytes > 0) {
        LOG_TRACE("Wrote %zd bytes to %s (fd=%d) from %s (fd=%d) - '%.*s'", writtenBytes, IS_CLIENT_DATA(connection, key) ? "client" : "origin", toFd, IS_CLIENT_DATA(connection, key) ? "origin" : "client", from->fd, (int)writtenBytes, readPtr);
        buffer_read_adv(from->buffer, writtenBytes);
        if (update_target_interests(key->s, from) < 0) {
            LOG_ERROR("Failed to update interests after write");
            return ERROR;
        };
    } else {
        LOG_TRACE("Couldn't write to %s (fd=%d)", IS_CLIENT_DATA(connection, key) ? "client" : "origin", toFd);
        if (writtenBytes < 0) {
            LOG_WARN("Write error (%d) to fd=%d: %s", errno, toFd, strerror(errno));
        }
        shutdown(toFd, SHUT_WR);
        from->interests &= ~OP_WRITE;
        selector_set_interest(key->s, from->fd, from->interests);
        shutdown(from->otherCopySt->fd, SHUT_RD);
        from->otherCopySt->interests &= ~OP_READ;
        selector_set_interest(key->s, from->otherCopySt->fd, from->otherCopySt->interests);
    }

    LOG_TRACE("%s interests after write to %s: OP_READ=%s, OP_WRITE=%s", IS_CLIENT_DATA(connection, key) ? "origin" : "client", IS_CLIENT_DATA(connection, key) ? "client" : "origin", (from->otherCopySt->interests & OP_READ) ? "true" : "false", (from->otherCopySt->interests & OP_WRITE) ? "true" : "false");
    LOG_TRACE("%s interests after write to %s: OP_READ=%s, OP_WRITE=%s", IS_CLIENT_DATA(connection, key) ? "client" : "origin", IS_CLIENT_DATA(connection, key) ? "client" : "origin", (from->interests & OP_READ) ? "true" : "false", (from->interests & OP_WRITE) ? "true" : "false");
	return (from->interests | from->otherCopySt->interests) ^ OP_NOOP ? COPY : DONE;
}

void socksv5_copy_close(const unsigned int state, struct selector_key * key) {
    struct socks5 * connection = ATTACHMENT(key);
    LOG_DEBUG("Closing copy connections: client_fd=%d, origin_fd=%d", connection->client_fd, connection->origin_fd);
}