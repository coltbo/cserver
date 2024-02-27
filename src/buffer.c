#include "buffer.h"
#include <stdlib.h>

#define MAX_BUFFER_SIZE 100 // TODO - research a good number

struct RequestBuffer {
  int buffer[MAX_BUFFER_SIZE];
  int start;
  int end;
  size_t capacity;
};

RequestBuffer *request_buffer_alloc() {
  RequestBuffer *buffer = (RequestBuffer *)calloc(1, sizeof(RequestBuffer));
  if (buffer != NULL) {
    buffer->capacity = MAX_BUFFER_SIZE;
  }

  return buffer;
}

int request_buffer_write(RequestBuffer *buffer, int req) {
  // buffer capacity is full; will not rewrite
  if (buffer->capacity == (MAX_BUFFER_SIZE - 1)) {
    return -1;
  }

  *(buffer->buffer + (buffer->end)) = req;

  // reset if we've reached the end
  buffer->end = (buffer->end + 1) == MAX_BUFFER_SIZE ? 0 : buffer->end + 1;

  buffer->capacity++;
  return 0;
}

int request_buffer_read(RequestBuffer *buffer) {
  // nothing in the buffer
  if (buffer->capacity == 0) {
    return -1;
  }

  int result = *(buffer->buffer + (buffer->start));

  // reset if we've reached the end
  buffer->start = (buffer->start + 1) == MAX_BUFFER_SIZE ? 0 : buffer->start + 1;

  buffer->capacity--;
  return result;
}

// TODO - how am I going to make sure all requests are freed?
void request_buffer_free(RequestBuffer *buffer) {
  if (buffer == NULL)
    return;
  free(buffer->buffer);
  free(buffer);
}
