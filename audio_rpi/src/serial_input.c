#include "serial_input.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

// ─── Protocolo (debe coincidir con el sketch Arduino y el monitor Python) ────
static const uint8_t SYNC_WORD[4] = {0xAA, 0x55, 0xFF, 0x00};

// ─── Tabla interna de handles (mapea fd entero → HANDLE de Windows) ──────────
#define MAX_SERIAL_PORTS 8
static HANDLE serial_handles[MAX_SERIAL_PORTS];
static int    serial_fd_count = 0;

// ─── Autodetectar puerto COM ──────────────────────────────────────────────────
// Prueba COM1-COM32 y devuelve el primero que abre correctamente.
// Devuelve un puntero estático válido hasta la próxima llamada, o NULL si no
// encuentra ninguno.
const char *serial_autodetect(void)
{
    static char port_name[16];
    for (int n = 1; n <= 32; n++) {
        char try_name[24];
        snprintf(try_name, sizeof(try_name), "\\\\.\\COM%d", n);
        HANDLE h = CreateFileA(try_name, GENERIC_READ | GENERIC_WRITE,
                               0, NULL, OPEN_EXISTING, 0, NULL);
        if (h != INVALID_HANDLE_VALUE) {
            CloseHandle(h);
            snprintf(port_name, sizeof(port_name), "COM%d", n);
            printf("[serial] autodetectado: %s\n", port_name);
            return port_name;
        }
    }
    fprintf(stderr, "[serial] no se encontro ningun puerto COM\n");
    return NULL;
}

// ─── Abrir y configurar el puerto COM ────────────────────────────────────────
// Si port == NULL, autodetecta.
// Devuelve un fd (índice en tabla interna) o -1 en error.
int serial_open(const char *port, int baud)
{
    if (port == NULL) {
        port = serial_autodetect();
        if (port == NULL) return -1;
    }

    if (serial_fd_count >= MAX_SERIAL_PORTS) {
        fprintf(stderr, "[serial] demasiados puertos abiertos\n");
        return -1;
    }

    // Usar prefijo \\.\COMx para compatibilidad con puertos > COM9
    char dev_name[32];
    snprintf(dev_name, sizeof(dev_name), "\\\\.\\%s", port);

    HANDLE h = CreateFileA(dev_name, GENERIC_READ | GENERIC_WRITE,
                           0, NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "[serial] no se pudo abrir %s: error %lu\n",
                port, GetLastError());
        return -1;
    }

    DCB dcb;
    memset(&dcb, 0, sizeof(dcb));
    dcb.DCBlength = sizeof(dcb);
    if (!GetCommState(h, &dcb)) {
        fprintf(stderr, "[serial] GetCommState: error %lu\n", GetLastError());
        CloseHandle(h);
        return -1;
    }

    // 8N1, sin control de flujo, baud rate configurable
    dcb.BaudRate        = (DWORD)baud;
    dcb.ByteSize        = 8;
    dcb.Parity          = NOPARITY;
    dcb.StopBits        = ONESTOPBIT;
    dcb.fBinary         = TRUE;
    dcb.fParity         = FALSE;
    dcb.fOutxCtsFlow    = FALSE;
    dcb.fOutxDsrFlow    = FALSE;
    dcb.fDtrControl     = DTR_CONTROL_DISABLE;
    dcb.fRtsControl     = RTS_CONTROL_DISABLE;
    dcb.fOutX           = FALSE;
    dcb.fInX            = FALSE;

    if (!SetCommState(h, &dcb)) {
        fprintf(stderr, "[serial] SetCommState: error %lu\n", GetLastError());
        CloseHandle(h);
        return -1;
    }

    // Timeout de lectura: 2 segundos total
    COMMTIMEOUTS timeouts;
    memset(&timeouts, 0, sizeof(timeouts));
    timeouts.ReadTotalTimeoutConstant    = 2000; // ms
    timeouts.ReadTotalTimeoutMultiplier  = 0;
    timeouts.ReadIntervalTimeout         = 0;
    timeouts.WriteTotalTimeoutConstant   = 0;
    timeouts.WriteTotalTimeoutMultiplier = 0;

    if (!SetCommTimeouts(h, &timeouts)) {
        fprintf(stderr, "[serial] SetCommTimeouts: error %lu\n", GetLastError());
        CloseHandle(h);
        return -1;
    }

    // Vaciar buffer de entrada antes de empezar
    Sleep(100);  // 100 ms: el ESP32 puede estar enviando basura al conectar
    PurgeComm(h, PURGE_RXCLEAR);

    printf("[serial] abierto %s @ %d baud\n", port, baud);
    int fd = serial_fd_count++;
    serial_handles[fd] = h;
    return fd;
}

void serial_close(int fd)
{
    if (fd >= 0 && fd < MAX_SERIAL_PORTS && serial_handles[fd] != INVALID_HANDLE_VALUE) {
        CloseHandle(serial_handles[fd]);
        serial_handles[fd] = INVALID_HANDLE_VALUE;
    }
}

// ─── Leer exactamente n bytes (bloqueante con timeout) ───────────────────────
static int read_exact(HANDLE h, uint8_t *buf, DWORD n)
{
    DWORD total = 0;
    while (total < n) {
        DWORD r = 0;
        if (!ReadFile(h, buf + total, n - total, &r, NULL) || r == 0)
            return -1;  // timeout o error
        total += r;
    }
    return 0;
}

// ─── Buscar sync word con ventana deslizante ─────────────────────────────────
static int find_sync(HANDLE h)
{
    uint8_t window[4] = {0};
    // Intentamos hasta 8 * PACKET_PAYLOAD bytes antes de rendirse
    int max_tries = SERIAL_PACKET_SAMPLES * 8 * 2;
    for (int t = 0; t < max_tries; t++) {
        uint8_t b;
        DWORD r = 0;
        if (!ReadFile(h, &b, 1, &r, NULL) || r == 0) return -1;
        // Desplazar ventana
        window[0] = window[1];
        window[1] = window[2];
        window[2] = window[3];
        window[3] = b;
        if (memcmp(window, SYNC_WORD, 4) == 0) return 0;
    }
    fprintf(stderr, "[serial] sync no encontrado tras %d bytes\n", max_tries);
    return -1;
}

// ─── Leer un paquete completo ─────────────────────────────────────────────────
int serial_read_packet(int fd, uint16_t *out_samples)
{
    if (fd < 0 || fd >= MAX_SERIAL_PORTS || serial_handles[fd] == INVALID_HANDLE_VALUE) return -1;
    HANDLE h = serial_handles[fd];

    if (find_sync(h) < 0) return -1;

    uint8_t raw[SERIAL_PACKET_SAMPLES * 2];
    if (read_exact(h, raw, (DWORD)sizeof(raw)) < 0) {
        fprintf(stderr, "[serial] timeout leyendo payload\n");
        return -1;
    }

    // Decodificar little-endian → uint16
    for (int i = 0; i < SERIAL_PACKET_SAMPLES; i++) {
        out_samples[i] = (uint16_t)(raw[i * 2]) | ((uint16_t)(raw[i * 2 + 1]) << 8);
    }
    return 0;
}
