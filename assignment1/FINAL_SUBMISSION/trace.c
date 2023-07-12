#include "trace.h"

uint8_t ip_length(const uint8_t *pkt_data) {
    int num = 0;

    const uint8_t x = pkt_data[14] & 0xF;
    num = x * 4;
    return num;
}

void ether_info(const uint8_t *pkt_data, int in_byte_count, int packet_num) {
    struct ether_addr ether_macs;

    printf("Packet number: %d  Frame Len: %d\n\n", packet_num, in_byte_count);
    printf("\tEthernet Header\n");

    memcpy(&ether_macs, &pkt_data[0], 6);
    printf("\t\tDest MAC: %s\n", ether_ntoa(&ether_macs));
    memcpy(&ether_macs, &pkt_data[6], 6);
    printf("\t\tSource MAC: %s\n", ether_ntoa(&ether_macs));      
}

uint16_t ether_next(const uint8_t *pkt_data) {
    uint16_t val;

    /* EXTRACT TYPE SO WE KNOW WHAT HEADER COMES NEXT */
    memcpy(&val, &pkt_data[12], sizeof val);
    val = ntohs(val);
    return val;
}

uint8_t ip_next(const uint8_t *pkt_data) {
    uint8_t val;

    /* EXTRACT TYPE SO WE KNOW WHAT HEADER COMES NEXT */
    memcpy(&val, &pkt_data[23], sizeof val);
    return val;
}

void tcp_flag(int ip_len, const uint8_t *pkt_data) {
    uint8_t MASK_ACK = 0x10;
    uint8_t MASK_SYN = 0x2;
    uint8_t MASK_RST = 0x4;
    uint8_t MASK_FIN = 0x1;
    uint16_t DATA;
    uint8_t VALUE;
    
    memcpy(&DATA, &pkt_data[13 + ip_len], sizeof DATA);
    DATA = ntohs(DATA);

    /* BITMASKING FLAG BYTE TO EXTRACT CORRECT FLAG STATUS */
    /* ACKNOWLEGE FLAG */
    VALUE = DATA & MASK_ACK;
    if (VALUE == MASK_ACK) {
        printf("\t\tACK Flag: Yes\n");
    }
    if (VALUE == 0x0) {
        printf("\t\tACK Flag: No\n");
    }
    /* SYNC FLAG */
    VALUE = DATA & MASK_SYN;
    if (VALUE == MASK_SYN ) {
        printf("\t\tSYN Flag: Yes\n");
    }
    if (VALUE == 0x0) {
        printf("\t\tSYN Flag: No\n");
    }
    /* RESET FLAG */
    VALUE = DATA & MASK_RST;
    if (VALUE == MASK_RST ) {
        printf("\t\tRST Flag: Yes\n");
    }
    if (VALUE == 0 ) {
        printf("\t\tRST Flag: No\n");
    }
    /* FIN FLAG */
    VALUE = DATA & MASK_FIN;
    if (VALUE == MASK_FIN ) {
        printf("\t\tFIN Flag: Yes\n");
    }
    if (VALUE == 0 ) {
        printf("\t\tFIN Flag: No\n");
    } 
}

void tcp_info(uint16_t tcp_len, uint8_t chksum_buff[1512], int ip_len, const uint8_t *pkt_data) {
    uint16_t SRC_PORT;
    uint16_t DST_PORT;
    uint32_t SEQUENCE_NUM;
    uint32_t ACK_NUM;
    uint16_t WINDOW_SIZE;
    uint16_t cksum1; 
    uint16_t result;
    uint16_t DATA;
    uint8_t VALUE;
    uint8_t MASK_ACK = 0x10;

    /* SINCE IP HEADER CAN VARY LENGTH, ADD THE ETHER (13 BYTES) TO IP LEN TO GET CORRECT OFFSETT */
    ip_len = ip_len + 13;

    memcpy(&SRC_PORT, &pkt_data[1 + ip_len], sizeof SRC_PORT);
    SRC_PORT = ntohs(SRC_PORT);
    memcpy(&DST_PORT, &pkt_data[3 + ip_len], sizeof DST_PORT);
    DST_PORT = ntohs(DST_PORT);
    memcpy(&SEQUENCE_NUM, &pkt_data[5 + ip_len], sizeof SEQUENCE_NUM);
    SEQUENCE_NUM = htonl(SEQUENCE_NUM);
    memcpy(&ACK_NUM, &pkt_data[9 + ip_len], sizeof ACK_NUM);
    ACK_NUM = htonl(ACK_NUM);

    printf("\tTCP Header\n");

    if (SRC_PORT == 80) {
        printf("\t\tSource Port:  HTTP\n");
    }else {
        printf("\t\tSource Port: : %d\n", SRC_PORT);
    }
    if (DST_PORT == 80) {
        printf("\t\tDest Port:  HTTP\n");
    }else {
        printf("\t\tDest Port: : %d\n", DST_PORT);
    }
    printf("\t\tSequence Number: %" PRIu32 "\n", SEQUENCE_NUM);

    memcpy(&DATA, &pkt_data[13 + ip_len], sizeof DATA);
    DATA = ntohs(DATA);
    VALUE = DATA & MASK_ACK;

    if (ACK_NUM != 0 && VALUE == MASK_ACK) {
        
        printf("\t\tACK Number: %" PRIu32 "\n", ACK_NUM);
    }else {
        printf("\t\tACK Number: <not valid>\n");
    }

    tcp_flag(ip_len, pkt_data);

    memcpy(&WINDOW_SIZE, &pkt_data[15 + ip_len], sizeof WINDOW_SIZE);
    WINDOW_SIZE = ntohs(WINDOW_SIZE);
    printf("\t\tWindow Size: %d\n", WINDOW_SIZE);

    /* MEMCPY THE TCP SEGMENT INTO THE BIG BUFFER AFTER PSEUDO HEADER */
    memcpy(&chksum_buff[12], &pkt_data[ip_len + 1], tcp_len);

    /* EXTRACT CHECKSUM BYTES IN HOST ORDER */
    memcpy(&cksum1, &pkt_data[ip_len + 17], 2);
    cksum1 = ntohs(cksum1);

    result = in_cksum((unsigned short *)(chksum_buff), tcp_len + 12);
    if (result == 0) {
        printf("\t\tChecksum: Correct (0x%x)\n\n", cksum1);
    }else {
        printf("\t\tChecksum: Incorrect (0x%x)\n\n", cksum1);
    }
}

void udp_info(int len, const uint8_t *pkt_data) {
    uint16_t SRC_PORT;
    uint16_t DST_PORT;
    len = len + 13;

    memcpy(&SRC_PORT, &pkt_data[1 + len], sizeof SRC_PORT);
    SRC_PORT = ntohs(SRC_PORT);

    memcpy(&DST_PORT, &pkt_data[3 + len], sizeof DST_PORT);
    DST_PORT = ntohs(DST_PORT);

    printf("\tUDP Header\n");
    printf("\t\tSource Port: : %d\n", SRC_PORT);
    printf("\t\tDest Port: : %d\n", DST_PORT);
}

void icmp_info(int ip_len, const uint8_t *pkt_data) {
    
    printf("\tICMP Header\n");
     
    if (pkt_data[13 + ip_len + 1] == bad_cksum_ICMP) {
        printf("\t\tType: 109\n\n");
    }
    if (pkt_data[1 + ip_len + 13] == REPLY) {
        printf("\t\tType: Reply\n\n");
    }
    else if (pkt_data[1 + ip_len + 13] == REQUEST) {
        printf("\t\tType: Request\n\n");
    }
}

void ip_info(const uint8_t *pkt_data) {
    int ip_len = 0;
    uint16_t IP_PUD_LEN;
    uint8_t ip_flag;
    uint32_t sender_ip;
    uint32_t dst_ip;
    struct in_addr sender_ip_addr;
    struct in_addr dst_ip_addr;
    unsigned short checksum;
    uint16_t checksum_print;
    const uint8_t * test = &pkt_data[14];
    uint16_t tcp_len;
    uint8_t chksum_buff[1512];
    uint8_t reserved = 0;
    uint16_t newtcplen;


    ip_len = ip_length(pkt_data);
    
    printf("\tIP Header\n");
    printf("\t\tHeader Len: %d (bytes)\n", ip_len);
    printf("\t\tTOS: 0x%01x\n", pkt_data[15]);
    printf("\t\tTTL: %d\n", pkt_data[22]);

    memcpy(&IP_PUD_LEN, &pkt_data[16], sizeof IP_PUD_LEN);
    IP_PUD_LEN = ntohs(IP_PUD_LEN);
    printf("\t\tIP PDU Len: %d (bytes)\n", IP_PUD_LEN);

    ip_flag = ip_next(pkt_data);

    switch (ip_flag) {

    case UDP:
        printf("\t\tProtocol: UDP\n");
        break;
    case TCP:
        printf("\t\tProtocol: TCP\n");
        break;
    case ICMP:
        printf("\t\tProtocol: ICMP\n");
        break;
    
    default:
        printf("\t\tProtocol: Unknown\n");
        break;
    }

    memcpy(&checksum_print, &pkt_data[24], sizeof checksum_print);
    
    checksum = in_cksum((unsigned short *) test, ip_len);
    if (checksum == 0) {
        printf("\t\tChecksum: Correct (0x%1x)\n", checksum_print);
    }else {
        printf("\t\tChecksum: Incorrect (0x%1x)\n", checksum_print);
    }
    
    memcpy(&sender_ip, &pkt_data[26], sizeof sender_ip);
    sender_ip_addr.s_addr = sender_ip;

    printf("\t\tSender IP: %s\n", inet_ntoa(sender_ip_addr));

    /*TCP PSEUDO HEADER */
    /* SOURCE ADDRESS */
    memcpy(&chksum_buff[0], &pkt_data[26], 1);
    memcpy(&chksum_buff[1], &pkt_data[27], 1);
    memcpy(&chksum_buff[2], &pkt_data[28], 1);
    memcpy(&chksum_buff[3], &pkt_data[29], 1);

    /* DESTINATION ADDRESS */
    memcpy(&chksum_buff[4], &pkt_data[30], 1);
    memcpy(&chksum_buff[5], &pkt_data[31], 1);
    memcpy(&chksum_buff[6], &pkt_data[32], 1);
    memcpy(&chksum_buff[7], &pkt_data[33], 1);

    /* 8 BITS OF ZEROS and PROTOCOL */
    memcpy(&chksum_buff[8], &reserved,     1);
    memcpy(&chksum_buff[9], &ip_flag,      1);

    /* TECP SEGMENT LEN */
    tcp_len = IP_PUD_LEN - ip_len;
    newtcplen = htons(tcp_len);
    memcpy(&chksum_buff[10], &newtcplen,   2);

    memcpy(&dst_ip, &pkt_data[30], sizeof dst_ip);
    dst_ip_addr.s_addr = dst_ip;
    printf("\t\tDest IP: %s\n", inet_ntoa(dst_ip_addr));

    printf("\n");

    switch (ip_flag) {

    case UDP:
        udp_info(ip_len, pkt_data);
        break;
    case TCP:
        tcp_info(tcp_len, chksum_buff, ip_len, pkt_data);
        break;
    case ICMP:
        icmp_info(ip_len, pkt_data);
        break;
    
    default:
        break;
    }
}

void arp_info(const uint8_t *pkt_data) {
    uint32_t sender_ip;
    uint32_t target_ip;
    struct in_addr sender_ip_addr;
    struct in_addr target_ip_addr;
    struct ether_addr arp_macs;

    printf("\tARP header\n");

    if (pkt_data[21] == 0x01 ) {
        printf("\t\tOpcode: Request\n");
    } else {
        printf("\t\tOpcode: Reply\n");
    }

    memcpy(&arp_macs, &pkt_data[22], 6);
    printf("\t\tSender MAC: %s\n", ether_ntoa(&arp_macs));

    memcpy(&sender_ip, &pkt_data[28], sizeof sender_ip);
    sender_ip_addr.s_addr = sender_ip;
    printf("\t\tSender IP: %s\n", inet_ntoa(sender_ip_addr));

    memcpy(&arp_macs, &pkt_data[32], 6);
    printf("\t\tTarget MAC: %s\n", ether_ntoa(&arp_macs));

    memcpy(&target_ip, &pkt_data[38], sizeof target_ip);
    target_ip_addr.s_addr = target_ip;
    printf("\t\tTarget IP: %s\n\n\n", inet_ntoa(target_ip_addr));
}

int main(int argc, char *argv[]) {
    pcap_t *fp;
    char errbuf[PCAP_ERRBUF_SIZE];
    struct pcap_pkthdr *pkthdr;
    const uint8_t *pkt_data;
    int in_byte_count = 0;
    int err_check = 0;
    int packet_num = 0;
    uint16_t ether_flag;

    if (argc != 2) {
        fprintf(stderr, "%s", "Usage: ./trace [pcapfile] \n");
        exit(1);
    }

    if ((fp = pcap_open_offline(argv[1], errbuf)) == NULL) {
        fprintf(stderr, "Error in opening savefile, %s, for reading: %s\n", argv[1], errbuf);
        exit(2);
    }
    
    while ((err_check = pcap_next_ex(fp, &pkthdr, &pkt_data) >= 0)) {
        packet_num++;
        in_byte_count = pkthdr->len;

        ether_info(pkt_data, in_byte_count, packet_num);
        ether_flag = ether_next(pkt_data);
        
        switch (ether_flag) {

        case ARP:
            printf("\t\tType: ARP\n\n");
            arp_info(pkt_data);
            break;

        case IP:
            printf("\t\tType: IP\n\n");
            ip_info(pkt_data);
            break;
        
        default:
            printf("\t\tType: Uknown\n\n");
            break;
        }
    }
    pcap_close(fp);
    return 0;
}

