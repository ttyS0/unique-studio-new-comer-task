#include "cli.h"
#include "dig.h"
#define VERSION "0.1"

void print_records(const char* section, dig_record* collection, size_t length)
{
    dig_record* cur;
    printf("# %s Section: %d\n", section, length);
    if(length == 0) return;
    const char* empty = "(empty)";
    printf("%-40s%-10s%-10s%-10s%s\n", "NAME", "TYPE", "CLASS", "TTL", "DATA");
    for(int i = 0; i < length; i++)
    {
        char* data;
        cur = &collection[i];
        if(cur->dig_type == DNS_TYPE_A)
        {
            data = inet_ntoa(*(in_addr*)(cur->dig_data));
        }
        else if(cur->dig_type == DNS_TYPE_AAAA)
        {
            data = new char[INET6_ADDRSTRLEN + 1];
            inet_ntop(AF_INET6, &((dig_aaaa_data*)(cur->dig_data))->address, data, (socklen_t)INET6_ADDRSTRLEN);
        }
        else if(cur->dig_type == DNS_TYPE_NS)
        {
            data = ((dig_ns_data*)(cur->dig_data))->nsdname;
        }
        else if(cur->dig_type == DNS_TYPE_CNAME)
        {
            // printf("%02x", ((dig_cname_data*)(an[i].dig_data))->cname);
            data = ((dig_cname_data*)(cur->dig_data))->cname;
        }
        else if(cur->dig_type == DNS_TYPE_SOA)
        {
            char * m = ((dig_soa_data*)(cur->dig_data))->mname;
            char * r = ((dig_soa_data*)(cur->dig_data))->rname;
            data = new char[strlen(m) + strlen(r) + 2];
            strcpy(data, m);
            strcpy(data + strlen(m), ",");
            strcpy(data + strlen(m) + 1, r);
        }
        else if(cur->dig_type == DNS_TYPE_PTR)
        {
            data = ((dig_ptr_data*)(cur->dig_data))->ptrdname;
        }
        else if(cur->dig_type == DNS_TYPE_MX)
        {
            data = ((dig_mx_data*)(cur->dig_data))->exchange;
        }
        else if(cur->dig_type == DNS_TYPE_TXT)
        {
            data = ((dig_txt_data*)(cur->dig_data))->txtdata;
        }
        else
        {
            data = (char *) empty;
        }
        printf("%-40s%-10s%-10s%-10d%s\n", cur->dig_name , dns_type_name[cur->dig_type], dns_class_name[cur->dig_class], cur->dig_ttl, data);
    }
}

void query_recursive(const char * dns, const char * domain)
{
    in_addr_t auth = INADDR_NONE;
    in_addr_t master;
    char* current_name = nullptr;
    dig d(dns);
    master = d.current_dns();
    auth = d.current_dns();
    dig_record* an;
    size_t an_len;
    dig_record* ns;
    size_t ns_len;
    int queries = 0;
    bool first = true;
    while(current_name == nullptr || strcmp(current_name, domain) != 0)
    {
        bool found = false;
        if(queries >= MAX_QUERIES)
        {
            printf("Max queries exceed!");
            exit(0);
        }
        printf("Querying %s NS servers from %s...\n", (first ? "root" : "next level"), inet_ntoa(*(in_addr*)&auth));
        if(!d.query(first ? "" : domain, DNS_TYPE_NS, DNS_CLASS_IN, false, &an, &an_len, &ns, &ns_len, nullptr, nullptr))
        {
            printf("Retry...\n");
            queries++;
            continue;
        }
        print_records("Answers", an, an_len);
        for(int i = 0; i < an_len; i++)
        {
            if(an[i].dig_type == DNS_TYPE_A || an[i].dig_type == DNS_TYPE_CNAME)
            {
                current_name = an[i].dig_name;
                found = true;
                if(first) first = false;
                break;
            }
            else if(an[i].dig_type == DNS_TYPE_NS)
            {
                d.set_dns(master);
                auth = d.get_a_record(((dig_ns_data*)(an[i].dig_data))->nsdname);
                printf("\nSelecting %s (%s) as the next DNS server\n", ((dig_ns_data*)(an[i].dig_data))->nsdname, inet_ntoa(*(in_addr*) &auth));
                d.set_dns(auth);
                found = true;
                if(first) first = false;
                current_name = an[i].dig_name;
                break;
            }
        }
        print_records("Nameservers", ns, ns_len);
        for(int i = 0; (i < ns_len) && !found; i++)
        {
            if(ns[i].dig_type == DNS_TYPE_A || ns[i].dig_type == DNS_TYPE_CNAME)
            {
                current_name = ns[i].dig_name;
                break;
            }
            else if(ns[i].dig_type == DNS_TYPE_NS)
            {
                d.set_dns(master);
                auth = d.get_a_record(((dig_ns_data*)(ns[i].dig_data))->nsdname);
                printf("\nSelecting %s (%s) as the next DNS server\n", ((dig_ns_data*)(ns[i].dig_data))->nsdname, inet_ntoa(*(in_addr*) &auth));
                d.set_dns(auth);
                current_name = ns[i].dig_name;
                break;
            }
        }
        queries++;
    }

}

void query_normal(const char * dns, const char * domain, dns_type type)
{
    dig d(dns);
    dig_record* an;
    size_t an_len;
    dig_record* ns;
    size_t ns_len;
    dig_record* ar;
    size_t ar_len;
    dig_record* cur;
    int queries = 0;
    while(!d.query(domain, type, DNS_CLASS_IN, true, &an, &an_len, &ns, &ns_len, &ar, &ar_len))
    {
        if(queries >= MAX_QUERIES)
        {
            printf("Max queries exceed!");
            exit(0);
        }
    }
    print_records("Answers", an, an_len);
    print_records("Nameservers", ns, ns_len);
    print_records("Additional", ar, ar_len);
}

void query_reverse(const char * dns, const char * reverse)
{
    dig d(dns);
    dig_record* an;
    size_t an_len;
    int queries = 0;
    while(!d.query(address_to_domain(reverse), DNS_TYPE_PTR, DNS_CLASS_IN, true, &an, &an_len, nullptr, nullptr, nullptr, nullptr))
    {
        if(queries >= MAX_QUERIES)
        {
            printf("Max queries exceed!");
            exit(0);
        }
    }
    print_records("Answers", an, an_len);
}

int main(int argc, char** argv)
{
    cli c(argc, argv);
    c.add_switch("+trace", "enable tracing");
    c.add_switch("-h", "see help");
    c.add_option("@", "specify DNS server", "");
    c.add_option("-x", " do reverse lookups", "");
    c.add_argument("NAME", "the target domain", "");
    c.add_argument("TYPE", "query type", "A");
    c.parse();

    if(c.get_switch("-h"))
    {
        c.help();
        return 0;
    }
    bool trace = c.get_switch("+trace");
    const char * user_dns = c.get_option("@");
    const char * user_domain = c.get_argument("NAME");
    const char * type = c.get_argument("TYPE");
    const char * reverse = c.get_argument("-x");
    std::cout << "YDiG " << VERSION << std::endl;
    std::cout << "User DNS: " << user_dns << std::endl;
    std::cout << "User Domain: " << user_domain << std::endl;
    std::cout << "User Type: " << type << std::endl;
    dns_type user_type = type_name_to_enum(type);
    std::cout << "Tracing: " << (trace ? "On" : "Off") << std::endl;
    dig s(user_dns);
    in_addr_t dns = s.current_dns();
    std::cout << "DNS Server: " << inet_ntoa(*(in_addr*)&dns) << std::endl;
    if(strlen(c.get_argument("-x")) > 0)
    {
        std::cout << "Reverse: " << reverse << std::endl;
        query_reverse(user_dns, reverse);
    }
    else if(trace)
    {
        std::cout << "Domain: " << (strlen(user_domain) ? user_domain : "(root)") << std::endl;
        query_recursive(user_dns, user_domain);
    }
    else
    {
        std::cout << "Domain: " << (strlen(user_domain) ? user_domain : "(root)") << std::endl;
        query_normal(user_dns, user_domain, user_type);
    }

    return 0;
}