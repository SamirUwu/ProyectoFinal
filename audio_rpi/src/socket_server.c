#include "socket_server.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#define SOCKET_PATH "/tmp/audio_socket"

static int server_fd;
static int client_fd = -1;

int socket_init() {
    struct sockaddr_un addr;

    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCKET_PATH);

    unlink(SOCKET_PATH);

    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    chmod(SOCKET_PATH, 0777);  // cualquier usuario puede conectarse
    listen(server_fd, 1);

    printf("Esperando cliente...\n");
    client_fd = accept(server_fd, NULL, NULL);
    printf("Cliente conectado\n");

    // FIX #3: poner el socket en modo no-bloqueante DESPUÉS de accept.
    // Así send() retorna EAGAIN en lugar de bloquear el hilo de audio
    // cuando el buffer del kernel está lleno (Python ocupado renderizando).
    int flags = fcntl(client_fd, F_GETFL, 0);
    if (flags >= 0)
        fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

    return 0;
}

int socket_receive(char *buffer, int max_len) {
    int n = recv(client_fd, buffer, max_len - 1, MSG_DONTWAIT);
    if (n > 0)
        buffer[n] = '\0';
    return n;
}

// Legacy — manda un solo par (evitar en el loop de audio)
int socket_send_two_floats(float pre, float post) {
    float buf[2] = { pre, post };
    return send(client_fd, buf, sizeof(buf), MSG_DONTWAIT);
}

// FIX #3: send_batch ya no bloquea el hilo de audio.
// Si el buffer del kernel está lleno (Python lento) simplemente descarta
// el frame de visualización — inaudible para el oyente.
int socket_send_batch(const float *pre, const float *post, int n) {
    float *interleaved = malloc(n * 2 * sizeof(float));
    if (!interleaved) return -1;

    for (int i = 0; i < n; i++) {
        interleaved[i * 2]     = pre[i];
        interleaved[i * 2 + 1] = post[i];
    }

    // Si no hay cliente válido, no intentar enviar
    if (client_fd < 0) {
        free(interleaved);
        return -1;
    }

    int total = n * 2 * sizeof(float);
    int sent = send(client_fd, interleaved, total, MSG_DONTWAIT);
    free(interleaved);

    if (sent < 0) {
        if (errno == EBADF || errno == EPIPE || errno == ECONNRESET) {
            // Cliente desconectado — marcar fd como inválido y callar
            client_fd = -1;
        } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("socket_send_batch");
        }
    }

    return sent;
}

void socket_close() {
    close(client_fd);
    close(server_fd);
    unlink(SOCKET_PATH);
}