# MySMTP — Simplified Email Service

A lightweight **SMTP-like email service implemented in C using TCP sockets**. The project consists of a multi-threaded server and a command-line client that communicate using a simplified SMTP protocol.

The server accepts email messages from clients and stores them locally in mailbox files. Clients can send, list, and retrieve emails for recipients.

## Features

* 📡 TCP socket-based client-server communication
* 🧵 Multi-threaded server supporting multiple clients
* 📬 Mailbox storage for individual recipients
* 💾 Persistent local email storage
* 🔄 SMTP-like command protocol
* 🖥️ Command-line client interface
* ✉️ Support for sending, listing, and retrieving emails

### Supported Commands

| Command     | Description                             |
| ----------- | --------------------------------------- |
| `HELO`      | Initiates communication with the server |
| `MAIL FROM` | Specifies the sender                    |
| `RCPT TO`   | Specifies the recipient                 |
| `DATA`      | Starts entering the email body          |
| `LIST`      | Lists emails in a recipient's mailbox   |
| `GET_MAIL`  | Retrieves a specific email              |
| `QUIT`      | Closes the client connection            |

---

## Project Structure

```text
.
├── Makefile
├── mysmtp_client.c
├── mysmtp_server.c
├── mysmtp_client
├── mysmtp_server
├── alice_b.com.txt
├── alice_b.com_counter.txt
├── bob_a.com.txt
└── bob_a.com_counter.txt
```

### File Description

* **`mysmtp_server.c`** — Implements the SMTP-like mail server.
* **`mysmtp_client.c`** — Implements the command-line client.
* **`Makefile`** — Compiles the client and server.
* **`mysmtp_server`** — Compiled server executable.
* **`mysmtp_client`** — Compiled client executable.
* **`*_*.txt`** — Local mailbox and message-counter files used for email storage.

---

## Requirements

To build and run the project, you need:

* Linux/Unix-based environment
* GCC compiler
* `make`
* POSIX-compliant socket and threading support

The project uses standard C networking APIs such as:

* TCP sockets
* `pthread`
* File I/O

---

## Compilation

Clone or download the project and navigate to its directory.

Run:

```bash
make
```

This compiles both the client and server programs.

After successful compilation, the following executables should be available:

```text
mysmtp_server
mysmtp_client
```

---

## Running the Server

Start the server by specifying a port number:

```bash
./mysmtp_server 2525
```

The server will start listening for incoming TCP connections on port `2525`.

Keep this terminal running.

---

## Running the Client

Open a **second terminal** and connect to the server using:

```bash
./mysmtp_client 127.0.0.1 2525
```

Here:

* `127.0.0.1` is the localhost IP address.
* `2525` is the port on which the server is listening.

---

## Example Session

Once the client is connected, you can interact with the server using the supported commands.

### Sending an Email

```text
HELO client1
MAIL FROM: alice@a.com
RCPT TO: bob@b.com
DATA
Hello Bob,

This is a test email.
.
```

The `DATA` command indicates that the email body is about to be entered.

A single period (`.`) on a line by itself indicates the **end of the email**.

The server then stores the message in the recipient's mailbox.

---

## Listing Emails

To view the emails stored for a recipient:

```text
LIST bob@b.com
```

This displays the messages currently available in Bob's mailbox.

Example:

```text
1
2
3
```

The exact output depends on the emails that have been stored.

---

## Retrieving an Email

To retrieve a specific email, use:

```text
GET_MAIL bob@b.com 1
```

where:

* `bob@b.com` is the recipient.
* `1` is the email number to retrieve.

The server returns the corresponding stored email.

---

## Closing the Connection

When finished, terminate the client session using:

```text
QUIT
```

This closes the TCP connection between the client and server.

---

## Example Complete Workflow

### Terminal 1 — Start Server

```bash
make
./mysmtp_server 2525
```

### Terminal 2 — Start Client

```bash
./mysmtp_client 127.0.0.1 2525
```

### Client Commands

```text
HELO client1
MAIL FROM: alice@a.com
RCPT TO: bob@b.com
DATA
Hello Bob,

This is a test email.
.
LIST bob@b.com
GET_MAIL bob@b.com 1
QUIT
```

---

## Mailbox Storage

Emails are stored locally by the server rather than being delivered to an external mail service.

Mailbox-related files contain the stored messages and counters used to keep track of emails.

For example:

```text
bob_a.com.txt
bob_a.com_counter.txt
```

The mailbox file stores Bob's received emails, while the counter file keeps track of the number of stored messages.

This provides a simple persistent storage mechanism without requiring an external database.

---

## Architecture

The project follows a simple client-server architecture:

```text
             TCP Connection
     ┌──────────────────────────┐
     │                          │
     ▼                          │
┌──────────┐              ┌──────────────┐
│  Client  │ ──────────── │    Server    │
│          │              │              │
└──────────┘              │  TCP Socket  │
                          │      │       │
                          │      ▼       │
                          │  Mailbox     │
                          │  Storage     │
                          └──────────────┘
```

The server uses threads to handle multiple client connections concurrently.

---

## Protocol Flow

A typical email transmission follows this sequence:

```text
HELO
  │
  ▼
MAIL FROM
  │
  ▼
RCPT TO
  │
  ▼
DATA
  │
  ▼
Email Body
  │
  ▼
.
  │
  ▼
Email Stored
```

The client can subsequently use:

```text
LIST
  │
  ▼
GET_MAIL
```

to access stored messages.

---


