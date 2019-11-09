#include <fstream>
#include <cstring>
#include <cstdlib>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include "dns.h"

class dig
{
private:
    const char* root = "(root)";
    in_addr_t dns_server = INADDR_NONE;
    in_addr_t resolv_dns_server()
    {
        in_addr_t dns;
        char buf[64];
        char field[32];
        char server[16];
        std::ifstream fin("/etc/resolv.conf");
        while(fin.getline(buf, 63))
        {
            if(buf[0] == '#')
            {
                continue;
            }
            int t = 0;
            char* p = buf;
            while(*p == ' ') p++;
            strncpy(field, p, 10);
            field[10] = '\0';
            if(strcmp(field, "nameserver") == 0)
            {
                while(*p != ' ') p++;
                while(*p == ' ') p++;
                do
                {
                    server[t++] = *p;
                    p++;
                } while (*p != '\0' && *p != ' ');
                server[t] = '\0';
                dns = inet_addr(server);
                break;
            }
        }
        fin.close();
        return dns;
    }
    unsigned char* to_dig_data(dns_type dns_type, const char* context, const char* raw, size_t length)
    {
        size_t name_len;
        if(dns_type == DNS_TYPE_A)
        {
            dig_a_data* r = new dig_a_data();
            r->address = *(unsigned int*) raw;
            return (unsigned char*) r;
        }
        else if(dns_type == DNS_TYPE_CNAME)
        {
            dig_cname_data* r = new dig_cname_data();
            r->cname = field_pointer_to_domain(context, raw, length, &name_len);
            return (unsigned char*) r;
        }
        else if(dns_type == DNS_TYPE_PTR)
        {
            dig_ptr_data* r = new dig_ptr_data();
            r->ptrdname = field_pointer_to_domain(context, raw, length, &name_len);
            return (unsigned char*) r;
        }
        else if(dns_type == DNS_TYPE_NS)
        {
            dig_ns_data* r = new dig_ns_data();
            r->nsdname = field_pointer_to_domain(context, raw, length, &name_len);
            return (unsigned char*) r;
        }
        else if(dns_type == DNS_TYPE_SOA)
        {
            dig_soa_data* r = new dig_soa_data();
            r->mname = field_pointer_to_domain(context, raw, length, &name_len);
            raw += name_len;
            r->rname = field_pointer_to_domain(context, raw, length, &name_len);;
            raw += name_len;
            r->serial = *(unsigned int*) raw;
            raw += sizeof(unsigned int);
            r->refresh = *(unsigned int*) raw;
            raw += sizeof(unsigned int);
            r->retry = *(unsigned int*) raw;
            raw += sizeof(unsigned int);
            r->expire = *(unsigned int*) raw;
            raw += sizeof(unsigned int);
            r->minimum = *(unsigned int*) raw;
            return (unsigned char*) r;
        }
        else if(dns_type == DNS_TYPE_MX)
        {
            dig_mx_data* r = new dig_mx_data();
            r->preference = *(unsigned short*) raw;
            raw += sizeof(unsigned short);
            r->exchange =  field_pointer_to_domain(context, raw, length, &name_len);
            return (unsigned char*) r;
        }
        else if(dns_type == DNS_TYPE_TXT)
        {
            dig_txt_data* r = new dig_txt_data();
            r->txtlength = *(unsigned char*) raw;
            raw += sizeof(unsigned char);
            r->txtdata = new char [r->txtlength + 1];
            strncpy(r->txtdata, raw, r->txtlength);
            r->txtdata[r->txtlength] = '\0';
            return (unsigned char*) r;
        }
        else if(dns_type == DNS_TYPE_AAAA)
        {
            dig_aaaa_data* r = new dig_aaaa_data();
            memcpy(&r->address.__in6_u, raw, 16);
            return (unsigned char*) r;
        }
        else
        {
            return nullptr;
        }
        
    }
public:
    dig()
    {
        dns_server = resolv_dns_server();
    }
    dig(const char* dns_str)
    {
        in_addr_t dns;
        if((dns = inet_addr(dns_str)) != INADDR_NONE)
        {
            dns_server = dns;
        }
        else if((dns_server = resolv_dns_server()) && ((dns = get_a_record(dns_str)) != INADDR_NONE))
        {
            dns_server = dns;
        }
        else
        {
            dns_server = resolv_dns_server();
        }       
    }
    dig(in_addr_t dns)
    {
        if(dns != INADDR_NONE)
        {
            dns_server = dns;
        }
        else
        {
            dns_server = resolv_dns_server();
        }
        
    }
    void set_dns(const char* dns_str)
    {
        in_addr_t dns = inet_addr(dns_str);
        if(dns == INADDR_NONE)
        {
            dns_server = resolv_dns_server();
        }
        else
        {
            dns_server = dns;
        }       
    }
    void set_dns(in_addr_t dns)
    {
        if(dns == INADDR_NONE)
        {
            dns_server = resolv_dns_server();
        }
        else
        {
            dns_server = dns;
        }
        
    }
    in_addr_t current_dns()
    {
        return dns_server;
    }
    bool query(const char* query_domain, dns_type query_type, dns_class query_class, bool query_recursive, dig_record** an_records, size_t* an_length, dig_record** ns_records, size_t* ns_length, dig_record** ar_records, size_t* ar_length)
    {
        // std::cout << dns_server;
        char send_buffer[UDP_MAX_LENGTH];
        memset(send_buffer, 0, sizeof(send_buffer));
        char recv_buffer[UDP_MAX_LENGTH * UDP_MAX_LENGTH];
        memset(recv_buffer, 0, sizeof(recv_buffer));
        const char* dns_domain = domain_to_field(query_domain);
        char* send_p = send_buffer;
        char* recv_p = recv_buffer;
        
        size_t name_len;

        int s = socket(AF_INET , SOCK_DGRAM , IPPROTO_UDP);
        timeval tv_out;
        tv_out.tv_sec = MAX_QUERY_TIME;
        tv_out.tv_usec = 0;
        setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv_out, sizeof(tv_out));

        sockaddr_in dest;
        dest.sin_family = AF_INET;
        dest.sin_port = htons(53);
        dest.sin_addr.s_addr = dns_server;

        dns_header* send_dh = (dns_header*) send_p;
        send_dh->qdcount = htons(1);
        send_dh->rd = query_recursive ? 1 : 0;
        send_dh->id = htons(getpid());
        // printf("PID: %x\n", getpid());
        send_p += sizeof(dns_header);
        strcpy(send_p, dns_domain);
        send_p += strlen(dns_domain) + 1;
        dns_question_inner* send_dqi = (dns_question_inner*) (send_p);
        send_dqi->qclass = htons(query_class);
        send_dqi->qtype = htons(query_type);
        printf("Query type: %d\n", query_type);
        send_p += sizeof(dns_question_inner);

        ssize_t send_len;
        // printf("Start sending...\n");
        if((send_len = sendto(s, send_buffer, (size_t)(send_p - send_buffer), 0, (const sockaddr*) &dest, sizeof(dest))) < 0)
        {
            // printf("Sending failed\n");
            return false;
        }
        // printf("End sending...\n");
        int orig_len;
        struct sockaddr_in orig;
        ssize_t recv_len;
        // printf("Start receiving...\n");
        if((recv_len = recvfrom(s, recv_buffer, UDP_MAX_LENGTH * UDP_MAX_LENGTH, 0, (sockaddr*) &orig, (socklen_t *) &orig_len)) <= 0)
        {
            // printf("Receiving failed\n");
            return false;
        }
        // printf("Recv Length: %d\n", recv_len);
        // printf("End receiving...\n");
        dns_header* recv_dh = (dns_header*) recv_p;
        recv_p += sizeof(dns_header);
        uint16_t an_count = ntohs(recv_dh->ancount);
        uint16_t ns_count = ntohs(recv_dh->nscount);
        uint16_t ar_count = ntohs(recv_dh->arcount);
        if(an_length != nullptr) *an_length = an_count;
        if(ns_length != nullptr) *ns_length = ns_count;
        if(ar_length != nullptr) *ar_length = ar_count;
        if(an_records != nullptr) *an_records = new dig_record[an_count];
        if(ns_records != nullptr) *ns_records = new dig_record[ns_count];
        if(ar_records != nullptr) *ar_records = new dig_record[ar_count];
        recv_p += (size_t)(send_p - send_buffer) - sizeof(dns_header);
        char *rname;
        
        if(an_records != nullptr)
        {
            for(int i = 0; i < an_count; i++)
            {
                rname = field_pointer_to_domain(recv_buffer, recv_p, 0, &name_len);
                recv_p += name_len;
                if(strlen(rname) == 0) rname = (char*) root;
                (*an_records)[i].dig_name = rname;

                dns_record_inner* recv_dri = (dns_record_inner*) recv_p;
                (*an_records)[i].dig_type = ntohs(recv_dri->rtype);
                (*an_records)[i].dig_class = ntohs(recv_dri->rclass);
                (*an_records)[i].dig_ttl = ntohl(recv_dri->rttl);
                recv_p += sizeof(dns_record_inner);

                unsigned char* rdata = to_dig_data(dns_type(ntohs(recv_dri->rtype)), recv_buffer, recv_p, ntohs(recv_dri->rdlength));
                (*an_records)[i].dig_data = rdata;
                recv_p += ntohs(recv_dri->rdlength);
            }
        }
        if(ns_records != nullptr)
        {
            for(int i = 0; i < ns_count; i++)
            {
                rname = field_pointer_to_domain(recv_buffer, recv_p, 0, &name_len);
                recv_p += name_len;
                if(strlen(rname) == 0) rname = (char*) root;
                (*ns_records)[i].dig_name = rname;

                dns_record_inner* recv_dri = (dns_record_inner*) recv_p;
                (*ns_records)[i].dig_type = ntohs(recv_dri->rtype);
                (*ns_records)[i].dig_class = ntohs(recv_dri->rclass);
                (*ns_records)[i].dig_ttl = ntohl(recv_dri->rttl);
                recv_p += sizeof(dns_record_inner);

                unsigned char* rdata = to_dig_data(dns_type(ntohs(recv_dri->rtype)), recv_buffer, recv_p, ntohs(recv_dri->rdlength));
                (*ns_records)[i].dig_data = rdata;
                recv_p += ntohs(recv_dri->rdlength);
            }
        }
        if(ar_records != nullptr)
        {
            for(int i = 0; i < ar_count; i++)
            {
                rname = field_pointer_to_domain(recv_buffer, recv_p, 0, &name_len);
                recv_p += name_len;
                if(strlen(rname) == 0) rname = (char*) root;
                (*ar_records)[i].dig_name = rname;

                dns_record_inner* recv_dri = (dns_record_inner*) recv_p;
                (*ar_records)[i].dig_type = ntohs(recv_dri->rtype);
                (*ar_records)[i].dig_class = ntohs(recv_dri->rclass);
                (*ar_records)[i].dig_ttl = ntohl(recv_dri->rttl);
                recv_p += sizeof(dns_record_inner);

                unsigned char* rdata = to_dig_data(dns_type(ntohs(recv_dri->rtype)), recv_buffer, recv_p, ntohs(recv_dri->rdlength));
                (*ar_records)[i].dig_data = rdata;
                recv_p += ntohs(recv_dri->rdlength);
            }
        }
        close(s);
        return true;
    }
    in_addr_t get_a_record(const char * domain)
    {
        dig_record* an;
        size_t an_len;
        int queries;
        while(!query(domain, DNS_TYPE_A, DNS_CLASS_IN, true, &an, &an_len, nullptr, nullptr, nullptr, nullptr))
        {
            if(queries >= MAX_QUERIES)
            {
                printf("Max queries exceed!");
                exit(0);
            }
            queries++;
        }
        for(int i = 0; i < an_len; i++)
        {
            if(an[i].dig_type == DNS_TYPE_A)
                return ((dig_a_data*)(an[i].dig_data))->address;
        }
        return INADDR_NONE;
    }
};