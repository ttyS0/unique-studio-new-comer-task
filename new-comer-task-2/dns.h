#include <cstdint>
#include <cstddef>
#include <cstring>
#include <arpa/inet.h>
#include <netinet/in.h>

const int MAX_QUERIES = 20;

const size_t LABEL_MAX_LENGTH = 63;
const size_t NAME_MAX_LENGTH = 255;
const size_t TTL_MAX_LENGTH = 32;
const size_t UDP_MAX_LENGTH = 512;

struct dns_header
{
    uint16_t id;

    uint8_t rd : 1;
    uint8_t tc : 1;
    uint8_t aa : 1;
    uint8_t opcode : 4;
    uint8_t qr : 1;
    
    uint8_t rcode : 4;
    uint8_t cd : 1;
    uint8_t ad : 1;
    uint8_t z : 1;
    uint8_t ra : 1;

    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
};

#pragma pack(push, 1)
struct dns_record_inner
{
    uint16_t rtype;
    uint16_t rclass;
    uint32_t rttl;
    uint16_t rdlength;
};

struct dns_record
{
    unsigned char* rname;
    uint16_t rtype;
    uint16_t rclass;
    uint32_t rttl;
    uint16_t rdlength;
    uint8_t* rdata;
};

struct dns_question_inner
{
    uint16_t qtype;
    uint16_t qclass;
};
#pragma pack(pop)

struct dns_question
{
    uint8_t* qname;
    uint16_t qtype;
    uint16_t qclass;
};

struct dig_record
{
    char* dig_name;
    unsigned short dig_type;
    unsigned short dig_class;
    unsigned int dig_ttl;
    unsigned char* dig_data;
};

struct dig_cname_data
{
    char* cname;
};

struct dig_ptr_data
{
    char* ptrdname;
};

struct dig_hinfo_data
{
    char* cpu;
    char* os;
};

struct dig_mx_data
{
    unsigned short preference;
    char* exchange;
};

struct dig_ns_data
{
    char* nsdname;
};

struct dig_soa_data
{
    char* mname;
    char* rname;
    unsigned int serial;
    unsigned int refresh;
    unsigned int retry;
    unsigned int expire;
    unsigned int minimum;
};

struct dig_txt_data
{
    unsigned char txtlength;
    char* txtdata;
};

struct dig_a_data
{
    in_addr_t address;
};

struct dig_aaaa_data
{
    in6_addr address;
};

const char* dns_type_name[29] = { NULL, "A", "NS", "MD", "MF", "CNAME", "SOA", "MB", "MG", "MR", "NUL", "WKS", "PTR", "HINFO", "MINFO", "MX", "TXT", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "AAAA" };
enum dns_type
{
    DNS_TYPE_PADDING,
    DNS_TYPE_A,
    DNS_TYPE_NS,
    DNS_TYPE_MD,
    DNS_TYPE_MF,
    DNS_TYPE_CNAME,
    DNS_TYPE_SOA,
    DNS_TYPE_MB,
    DNS_TYPE_MG,
    DNS_TYPE_MR,
    DNS_TYPE_NUL,
    DNS_TYPE_WKS,
    DNS_TYPE_PTR,
    DNS_TYPE_HINFO,
    DNS_TYPE_MINFO,
    DNS_TYPE_MX,
    DNS_TYPE_TXT,
    DNS_TYPE_AAAA = 28
};
dns_type type_name_to_enum(const char* type_name)
{
    for(int i = 1; i < 29; i++)
    {
        if(strcmp(type_name, dns_type_name[i]) == 0)
        {
            return dns_type(i);
        }
    }
    return DNS_TYPE_NUL;
}

const char* dns_class_name[] = { NULL, "IN", "CS", "CH", "HS" };
enum dns_class
{
    DNS_CLASS_PADDING,
    DNS_CLASS_IN,
    DNS_CLASS_CS,
    DNS_CLASS_CH,
    DNS_CLASS_HS
};

char* domain_to_field(const char * domain)
{
    int l = strlen(domain);
    int prev = 0;
    char* field = new char[l + 2];
    char* p = field;
    for(int i = 0; i <= l; i++)
    {
        if(domain[i] == '.' || domain[i] == '\0')
        {
            *p++ = i - prev;
            for(; prev < i; prev++)
            {
                *p++ = domain[prev];
            }
            prev++;
        }
    }
    *p++ = '\0';
    return field;
}

char* pointer_to_string(const char * context, const char * raw, size_t length, size_t* raw_length)
{
    char* str = new char[NAME_MAX_LENGTH];
    const char* s = raw;
    char* p = str;
    unsigned short offset;
    while(*s != '\0')
    {
        if(length > 0)
        {
            if((s - raw) >= length)
            {
                break;
            }
        }
        if((unsigned char) *s >= (unsigned char) 0xC0) // 0b11000000
        {
            offset = ntohs(*(unsigned short *) s) & (unsigned short)(~0xC000);
            const char* pointed = &context[offset];
            pointed = pointer_to_string(context, pointed, 0, nullptr);
            while(*pointed != '\0')
            {
                *p++ = *pointed++;
            }
            s++;
            break;
        }
        else
        {
            *p++ = *s;
        }
        s++;
    }
    if(raw_length != nullptr) *raw_length = s - raw + 1;
    *p++ = '\0';
    return str;
}

char* field_to_domain(const char * field)
{
    int l = strlen(field);
    int prev = 1;
    char* domain = new char[l + 2];
    char* p = domain;
    for(int i = 0; i <= l; i++)
    {
        if(i != 0)
        {
            if(prev != 1) *p++ = '.';
            for(; prev < i; prev++)
            {
                *p++ = field[prev];
            }
            prev++;
        }
        i += field[i];
    }
    *p++ = '\0';
    return domain;
}

char* field_pointer_to_domain(const char * context, const char * raw, size_t length, size_t* raw_length)
{
    size_t len;
    char* unpointed = pointer_to_string(context, raw, length, &len);
    *raw_length = len;
    return field_to_domain(unpointed);
}

char* address_to_domain(const char * address)
{
    in_addr_t rev_address = htonl(inet_addr(address));
    char* rev = inet_ntoa(*(in_addr *)&rev_address);
    char* domain = new char[strlen(rev) + 13 + 1];
    strcpy(domain, rev);
    strcat(domain, ".in-addr.arpa");
    return domain;
}