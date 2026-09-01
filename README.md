# CDC CN course project Proof
Implement a Simplified Email Service

# MySMTP Mail Server and Client

A simple SMTP-like mail system implemented in C using socket programming and multithreading. This project includes a server that accepts client connections, stores emails in local mailbox files, and supports basic mail retrieval commands.

## Features

- SMTP-like command interaction
- Multi-client threaded server
- Mailbox storage in local files
- Email listing and retrieval
- Simple validation for email addresses
- Basic message transfer flow

## Project Structure

```text
.
├── makefile
├── mysmtp_client.c
├── mysmtp_server.c
├── mysmtp_client
├── mysmtp_server
├── alice_b.com.txt
├── alice_b.com_counter.txt
├── bob_a.com.txt
└── bob_a.com_counter.txt
