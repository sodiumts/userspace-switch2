#include "../include/ringbuffer.h"
#include <stdlib.h>
#include <string.h>

ring_buffer_t *ring_buffer_create(size_t size) {
    ring_buffer_t *rb = malloc(sizeof(ring_buffer_t));
    rb->buffer = malloc(size);
    rb->size = size;
    rb->head = rb->tail = 0;
    pthread_mutex_init(&rb->lock, NULL);
    return rb;
}

size_t ring_buffer_write(ring_buffer_t *rb, const uint8_t *data, size_t len) {
    pthread_mutex_lock(&rb->lock);
    size_t free = (rb->tail + rb->size - rb->head - 1) % rb->size;
    if (len > free) len = free;

    size_t first = rb->size - rb->head;
    if (len <= first) {
        memcpy(rb->buffer + rb->head, data, len);
    } else {
        memcpy(rb->buffer + rb->head, data, first);
        memcpy(rb->buffer, data + first, len - first);
    }
    rb->head = (rb->head + len) % rb->size;
    pthread_mutex_unlock(&rb->lock);
    return len;
}

size_t ring_buffer_read(ring_buffer_t *rb, uint8_t *data, size_t len) {
    pthread_mutex_lock(&rb->lock);
    size_t avail = (rb->head + rb->size - rb->tail) % rb->size;
    if (len > avail) len = avail;

    size_t first = rb->size - rb->tail;
    if (len <= first) {
        memcpy(data, rb->buffer + rb->tail, len);
    } else {
        memcpy(data, rb->buffer + rb->tail, first);
        memcpy(data + first, rb->buffer, len - first);
    }
    rb->tail = (rb->tail + len) % rb->size;
    pthread_mutex_unlock(&rb->lock);
    return len;
}
