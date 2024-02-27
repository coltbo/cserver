#ifndef BUFFER_H
#define BUFFER_H

typedef struct RequestBuffer RequestBuffer;

RequestBuffer *request_buffer_alloc();
int request_buffer_write(RequestBuffer *buffer, int req);
int request_buffer_read(RequestBuffer *buffer);
void request_buffer_free(RequestBuffer *buffer);

#endif
