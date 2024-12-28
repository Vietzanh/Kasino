#include "test.h"

TEST(test_decode_packet)
{
    char *data = "\x00\x0A\x01\x00\x00hello";

    Packet *packet = decode_packet(data);

    ASSERT(packet->header.packet_len == 10);
    ASSERT(packet->header.protocol_ver == 1);
    ASSERT(packet->header.packet_type == 0);
    ASSERT(compare_raw_bytes(packet->data, "hello", 5) == 1);
}

TEST(test_encode_packet)
{
    Header *header = malloc(sizeof(Header));
    header->packet_len = 10;
    header->protocol_ver = 1;
    header->packet_type = 0;

    char *data = "hello";
    Packet *packet = malloc(sizeof(Packet));
    packet->header = *header;

    memcpy(packet->data, data, header->packet_len - sizeof(Header));
    char *encoded = encode_packet(header, packet->data);
    ASSERT(compare_raw_bytes(encoded, "\x00\x0A\x01\x00\x00hello", 10) == 1);
    free(header);
    free(encoded);
    free(packet);
}

int main()
{
    RUN_TEST(test_decode_packet);
    RUN_TEST(test_encode_packet);
}