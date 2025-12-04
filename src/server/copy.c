#include "copy.h"
#include "socks5nio.h"
#include "logger.h"
#include "errno.h"
#include <string.h>
#include "selector.h"

#define IS_CLIENT_DATA(connection, key) (connection->client_fd == key->fd)

static fd_interest update_target_interests(fd_selector s, copy_st * target) {
	fd_interest ret = OP_NOOP;
	if ((target->interests & OP_WRITE) && buffer_can_read(target->buffer)) {
		ret |= OP_WRITE;
	} else if ((target->interests & OP_READ) && buffer_can_write(target->otherCopySt->buffer)) {
		ret |= target->interests |= OP_READ;
	}
	selector_set_interest(s, target->fd, ret);
	return ret;
}

void socksv5_copy_arrival(const unsigned int state, struct selector_key * key) {
    struct socks5 * connection = ATTACHMENT(key);
	copy_st * clientCopy = &connection->client.copy;
    copy_st * originCopy = &connection->origin_st.copy;

    clientCopy->buffer = &connection->client_buffer;
    clientCopy->fd = connection->client_fd;
    
    originCopy->buffer = &connection->origin_buffer;
    originCopy->fd = connection->origin_fd;

    originCopy->otherCopySt = clientCopy;
    clientCopy->otherCopySt = originCopy;

    LOG_TRACE("Entering COPY state: client_fd=%d, origin_fd=%d", clientCopy->fd, originCopy->fd);

    selector_set_interest(key->s, clientCopy->fd, OP_READ);
    selector_set_interest(key->s, originCopy->fd, OP_READ);
}

unsigned socksv5_copy_read(struct selector_key * key) {
	struct socks5* connection = ATTACHMENT(key);
    copy_st * from = IS_CLIENT_DATA(connection, key) ? &connection->client.copy : &connection->origin_st.copy;
	copy_st * to = from->otherCopySt;

	LOG_TRACE("Attemping READ data from fd=%d to fd=%d", from->fd, to->fd);

    if (!buffer_can_write(to->buffer)) {
        return COPY;
    }

    size_t can_write = 0;
	uint8_t* write_ptr = buffer_write_ptr(to->buffer, &can_write);

	ssize_t readBytes = recv(from->fd, write_ptr, can_write, 0);

    if (readBytes > 0) {
        LOG_TRACE("Read %zd bytes from fd=%d into fd=%d's buffer", readBytes, from->fd, to->fd);
		buffer_write_adv(to->buffer, readBytes);
    } else {
        if (readBytes < 0) {
            LOG_WARN("Read error (%d) from fd=%d: %s", errno, from->fd, strerror(errno));
        }
		shutdown(from->fd, SHUT_RD);
        from->interests &= ~OP_READ;
        shutdown(to->fd, SHUT_WR);
        to->interests &= ~OP_WRITE;
	}

	update_target_interests(key->s, from);
	update_target_interests(key->s, to);
	return (from->interests | to->interests) ^ OP_NOOP ? COPY : DONE;
}

unsigned socksv5_copy_write(struct selector_key * key) {
    struct socks5 * connection = ATTACHMENT(key);
	copy_st* from = IS_CLIENT_DATA(connection, key) ? &connection->client.copy : &connection->origin_st.copy;
	int toFd = from->fd;

	LOG_TRACE("Attempting WRITE data to fd=%d\n", toFd);

	if (!buffer_can_read(from->buffer)) {
        return COPY;
	}

	size_t can_read = 0;
    uint8_t* read_ptr = buffer_read_ptr(from->buffer, &can_read);

    ssize_t writtenBytes = send(toFd, read_ptr, can_read, 0);


    if (writtenBytes > 0) {
        LOG_TRACE("Wrote %zd bytes to fd=%d from fd=%d's buffer", writtenBytes, toFd, from->fd);
        buffer_read_adv(from->buffer, writtenBytes);
    } else {
        if (writtenBytes < 0) {
            LOG_WARN("Write error (%d) to fd=%d: %s", errno, toFd, strerror(errno));
        }
        shutdown(toFd, SHUT_WR);
        from->otherCopySt->interests &= ~OP_WRITE;
        shutdown(from->fd, SHUT_RD);
        from->interests &= ~OP_READ;
    }

    update_target_interests(key->s, from);
    update_target_interests(key->s, from->otherCopySt);
	return (from->interests | from->otherCopySt->interests) ^ OP_NOOP ? COPY : DONE;
}

void socksv5_copy_close(const unsigned int state, struct selector_key * key) {
    struct socks5 * connection = ATTACHMENT(key);
    LOG_INFO("Closing copy connections: client_fd=%d, origin_fd=%d", connection->client_fd, connection->origin_fd);
}