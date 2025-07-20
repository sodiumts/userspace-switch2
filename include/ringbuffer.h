#pragma once

#include <pthread.h>
#include <stdint.h>

typedef struct {
    uint8_t *buffer;
    size_t size;
    size_t head;
    size_t tail;
    pthread_mutex_t lock;
} ring_buffer_t;


ring_buffer_t *ring_buffer_create(size_t size);
size_t ring_buffer_write(ring_buffer_t *rb, const uint8_t *data, size_t len);
size_t ring_buffer_read(ring_buffer_t *rb, uint8_t *data, size_t len);
