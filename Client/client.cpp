#include <stdio.h>
#include <vector>

#include <nng/nng.h>
#include <nng/protocol/reqrep0/req.h>
#include <pb_decode.h>
#include <pb_encode.h>

#include "wcferry.pb.h"

void print_buffer(uint8_t *buffer, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        printf("%02X ", buffer[i]);
    }
    printf("\n");
}

bool req_is_login(uint8_t *out, size_t *out_len)
{
    Request req   = Request_init_default;
    req.func      = Functions_FUNC_IS_LOGIN;
    // req.which_msg = Request_empty_tag;

    if (!pb_get_encoded_size(out_len, Request_fields, &req)) {
        return false;
    }

    pb_ostream_t stream = pb_ostream_from_buffer(out, *out_len);
    if (!pb_encode(&stream, Request_fields, &req)) {
        printf("Encoding failed: %s\n", PB_GET_ERROR(&stream));
        return false;
    }

    return true;
}

bool make_req(uint8_t func, uint8_t *out, size_t *out_len)
{
    bool ret = false;
    printf("make_req for %02X\n", func);
    switch (func) {
        case Functions_FUNC_IS_LOGIN: {
            ret = req_is_login(out, out_len);
            break;
        }

        default: {
            break;
        }
    }
    return ret;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("Usage: ./client.exe func [url]\nfunc: 0~5\turl: tcp://127.0.0.1:5555");
        return -1;
    }

    uint8_t func = 0;
    char *url    = (char *)"tcp://127.0.0.1:5555";
    func         = atoi(argv[1]);
    if (argc > 2) {
        url = argv[2];
    }

    printf("url: %s, func: %02X\n", url, func);
    nng_socket sock;
    nng_dialer dialer;
    int rv;

    if ((rv = nng_req0_open(&sock)) != 0) {
        printf("nng_req0_open: %d\n", rv);
        return -1;
    }
    if ((rv = nng_dialer_create(&dialer, sock, url)) != 0) {
        printf("nng_dialer_create: %d\n", rv);
        return -2;
    }

    nng_dialer_start(dialer, 0);

    uint8_t *in   = NULL;
    size_t in_len = 0, out_len = 0;
    static uint8_t out[1024] = { 0 };
    if (make_req(func, out, &out_len)) {
        printf("Send message[%ld]: ", out_len);
        nng_send(sock, out, out_len, 0);
        print_buffer(out, out_len);

        if ((rv = nng_recv(sock, &in, &in_len, NNG_FLAG_ALLOC)) != 0) {
            printf("nng_recv: %d\n", rv);
        }
        printf("Get message[%ld]: ", in_len);
        print_buffer(in, in_len);

        nng_free(in, in_len);
        nng_close(sock);
        return 0;
    }

    nng_close(sock);
    printf("ERROR");
    return -1;
}
