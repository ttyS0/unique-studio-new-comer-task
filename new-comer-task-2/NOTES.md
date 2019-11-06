# Prerequisite
## Big Endian & Little Endian
In common operating systems, bytes are usually stored in their corresponding order, that is the lower bytes are stored in lower address, vice versa.

But in networks stream and some embedded chips, the opposite idea is adopted that is the lower bytes are stored in higher address, vice versa.

The difference actually makes sense, because the efficiency in networks stream and embedded fields matters the most. Usually, the higher bytes contain the more significant information, and the address usually increases from the lower, so the higher bytes can be read earlier.

### Bytes in Little Endian
```
 HIGHER ADDRESS
+-+-+-+-+-+-+-+-+
|4 (HIGHER BYTE)|
+-+-+-+-+-+-+-+-+
|       3       |
+-+-+-+-+-+-+-+-+
|       2       |
+-+-+-+-+-+-+-+-+
|1 (LOWER  BYTE)|
+-+-+-+-+-+-+-+-+
 LOWER ADDRESS
```

### Bytes in Big Endian
```
 HIGHER ADDRESS
+-+-+-+-+-+-+-+-+
|1 (LOWER  BYTE)|
+-+-+-+-+-+-+-+-+
|       2       |
+-+-+-+-+-+-+-+-+
|       3       |
+-+-+-+-+-+-+-+-+
|4 (HIGHER BYTE)|
+-+-+-+-+-+-+-+-+
 LOWER ADDRESS
```

## Bit Fields
To implement diagram structures in a convenient way, bit fields are available and easy to use in C/C++ like the following.

```c++
struct
{
  type [name] : width;
}
```

It sets the member with certain width of bits. Anyhow, things become complicated when more and more bit fields are introduced into the structure, and how they are stored has something to do with big endian & little endian.

In little endian, bit fields are stored in the reversed way that is the lower bits are stored in lower address, vice versa.

In big endian, it's the opposite situation.
### Bits in Little Endian
```
 HIGHER ADDRESS
+-+
|8| HIGHER BIT
+-+
|7|
+-+
|6|
+-+
|5|
+-+
|4|
+-+
|3|
+-+
|2|
+-+
|1| LOWER BIT
+-+
 LOWER ADDRESS
```

### Bits in Big Endian
```
 HIGHER ADDRESS
+-+
|1| LOWER BIT
+-+
|2|
+-+
|3|
+-+
|4|
+-+
|5|
+-+
|6|
+-+
|7|
+-+
|8| HIGHER BIT
+-+
 LOWER ADDRESS
```

# DNS Diagram Structures
See details in [RFC 1035](https://www.ietf.org/rfc/rfc1035.txt).
## DNS Query / Response Header in RFC 1035
```
 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|                      ID                       |
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|QR|   Opcode  |AA|TC|RD|RA| Z|AD|CD|   RCODE   |
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|                QDCOUNT/ZOCOUNT                |
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|                ANCOUNT/PRCOUNT                |
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|                NSCOUNT/UPCOUNT                |
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|                    ARCOUNT                    |
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
```
## DNS Question in RFC 1035
```
  0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|                                               |
/                     QNAME                     /
/                                               /
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|                     QTYPE                     |
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|                     QCLASS                    |
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
```
## DNS Resource Record in RFC 1035
```
  0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|                                               |
/                                               /
/                      NAME                     /
|                                               |
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|                      TYPE                     |
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|                     CLASS                     |
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|                      TTL                      |
|                                               |
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|                   RDLENGTH                    |
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--|
/                     RDATA                     /
/                                               /
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
```

