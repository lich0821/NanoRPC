#include <conio.h> // _getch()
#include <map>
#include <stdio.h>
#include <string>

#include <nng/nng.h>
#include <nng/protocol/reqrep0/rep.h>

#include <pb_decode.h>
#include <pb_encode.h>

#include "wcferry.pb.h"

std::map<int, std::string> g_types = { { 0x01, "文字" }, { 0x03, "图片" }, { 0x22, "语音" } };

void print_buffer(uint8_t *buffer, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        printf("%02X ", buffer[i]);
    }
    printf("\n");
}

bool func_is_login(uint8_t *out, size_t *len)
{
    Response rsp   = Response_init_default;
    rsp.func       = Functions_FUNC_IS_LOGIN;
    rsp.which_msg  = Response_status_tag;
    rsp.msg.status = 1;
    if (!pb_get_encoded_size(len, Response_fields, &rsp)) {
        return false;
    }

    pb_ostream_t stream = pb_ostream_from_buffer(out, *len);
    if (!pb_encode(&stream, Response_fields, &rsp)) {
        printf("Encoding failed: %s\n", PB_GET_ERROR(&stream));
        return false;
    }

    return true;
}

bool func_get_msg_types(uint8_t *out, size_t *len)
{
    Response rsp              = Response_init_default;
    rsp.func                  = Functions_FUNC_GET_SELF_WXID;
    rsp.which_msg             = Response_types_tag;
    rsp.msg.types.types_count = g_types.size();

    size_t i = 0;
    for (auto it = g_types.begin(); it != g_types.end(); it++) {
        rsp.msg.types.types[i].key = it->first;
        sprintf(rsp.msg.types.types[i].value, it->second.c_str());
        i++;
    }

    if (!pb_get_encoded_size(len, Response_fields, &rsp)) {
        return false;
    }

    pb_ostream_t stream = pb_ostream_from_buffer(out, *len);
    if (!pb_encode(&stream, Response_fields, &rsp)) {
        printf("Encoding failed: %s\n", PB_GET_ERROR(&stream));
        return false;
    }

    return true;
}

bool func_send_txt(TextMsg txt, uint8_t *out, size_t *len)
{
    Response rsp   = Response_init_default;
    rsp.func       = Functions_FUNC_SEND_TXT;
    rsp.which_msg  = Response_status_tag;
    rsp.msg.status = 0;
    printf("%s, %s, %s\n", txt.msg, txt.receiver, txt.aters);

    if (!pb_get_encoded_size(len, Response_fields, &rsp)) {
        return false;
    }

    pb_ostream_t stream = pb_ostream_from_buffer(out, *len);
    if (!pb_encode(&stream, Response_fields, &rsp)) {
        printf("Encoding failed: %s\n", PB_GET_ERROR(&stream));
        return false;
    }

    return true;
}

bool dispatcher(uint8_t *in, size_t in_len, uint8_t *out, size_t *out_len)
{
    bool ret            = false;
    Request req         = Request_init_default;
    pb_istream_t stream = pb_istream_from_buffer(in, in_len);
    if (!pb_decode(&stream, Request_fields, &req)) {
        printf("Decoding failed: %s\n", PB_GET_ERROR(&stream));
        pb_release(Request_fields, &req);
        return false;
    }

    printf("Func: %02X", (uint8_t)req.func);
    switch (req.func) {
        case Functions_FUNC_IS_LOGIN: {
            printf("[Functions_FUNC_IS_LOGIN]\n");
            ret = func_is_login(out, out_len);
            break;
        }
        case Functions_FUNC_GET_MSG_TYPES: {
            printf("[Functions_FUNC_GET_MSG_TYPES]\n");
            ret = func_get_msg_types(out, out_len);
            break;
        }
        case Functions_FUNC_SEND_TXT: {
            printf("[FUNC_SEND_TXT]\n");
            ret = func_send_txt(req.msg.txt, out, out_len);
            break;
        }

        default: {
            printf("[UNKNOW FUNCTION]\n");
            break;
        }
    }
    pb_release(Request_fields, &req);
    return ret;
}

int main(int argc, char **argv)
{
    char *url = (char *)"tcp://127.0.0.1:5555";
    if (argc == 2) {
        url = argv[1];
    }
    nng_socket sock;
    nng_listener listener;
    int rv;

    if ((rv = nng_rep0_open(&sock)) != 0) {
        printf("nng_rep0_open: %d\n", rv);
    }

    if ((rv = nng_listener_create(&listener, sock, url)) != 0) {
        printf("nng_listener_create: %d\n", rv);
    }
    nng_listener_start(listener, 0);
    printf("listening on %s\n", url);

    static uint8_t out[1024 * 1024] = { 0 };
    while (true) {
        uint8_t *in = NULL;
        size_t in_len, out_len;
        if ((rv = nng_recv(sock, &in, &in_len, NNG_FLAG_ALLOC)) != 0) {
            printf("nng_recv: %d\n", rv);
        }
        printf("Get message[%ld]: ", in_len);
        print_buffer(in, in_len);
        if (dispatcher(in, in_len, out, &out_len)) {
            printf("Response[%ld]: ", out_len);
            print_buffer(out, out_len);
            rv = nng_send(sock, out, out_len, 0);
            if (rv != 0) {
                printf("nng_send: %d\n", rv);
            }

        } else {
            // Error
            printf("Error...\n");
            rv = nng_send(sock, out, 0, 0);
        }
        nng_free(in, in_len);
    }
    nng_close(sock);
    return _getch();
}
