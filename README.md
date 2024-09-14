# Distributed File System (DFS)

Distributed file system implemented using C and socket programming on Linux. It features a three-server architecture that transparently distributes files across multiple specialized servers. The system efficiently manages and balances file operations based on file types, providing robust support for multiple concurrent clients.

## Table of Contents
- [Project Description](#project-description)
- [System Architecture](#system-architecture)
- [Features](#features)
- [How to Run the Project](#how-to-run-the-project)
- [How the System Works](#how-the-system-works)
- [Supported Operations](#supported-operations)
- [Notes](#notes)

## Project Description

DFS consists of a **main server (Smain)** and two specialized servers, **Spdf** and **Stext**, that handle PDF and text files respectively. Users interact solely with the **Smain** server, which internally routes requests to the specialized servers based on file types, making the system transparent and efficient for end-users.

The system supports various file operations, including file upload, download, removal, and archive creation. Additionally, it can handle multiple client connections simultaneously, ensuring optimal performance and scalability.

## System Architecture

The architecture of the DFS is as follows:

- **Smain**: The main server that acts as the interface for all user operations and distributes requests to the specialized servers.
- **Spdf**: The server responsible for managing `.pdf` files.
- **Stext**: The server responsible for managing `.txt` files.

![Distributed File System Architecture](https://github.com/user-attachments/assets/0a534513-f11c-4941-a11e-aad7eab3d9ae)


## Features

- **Transparent file distribution**: Files are distributed based on type without the user’s knowledge.
- **Concurrent client handling**: Supports multiple simultaneous clients for better response times.
- **File operations**: Upload, download, remove, and archive (tar) files.
- **Efficient load balancing**: Requests are directed to specialized servers for `.pdf` and `.txt` files, reducing load on the main server.
- **Scalability**: The system is designed to scale with an increasing number of clients and servers.
- **Data integrity**: Ensures the consistency of file operations across servers.

## How to Run the Project

### Prerequisites
- Linux environment
- GCC (GNU Compiler Collection)

### Steps

1. **Clone the repository**:
    ```bash
    git clone https://github.com/harshsabhaya/distributed-file-system
    ```

2. **Compile the source code**:
    ```bash
    gcc -o spdf Spdf.c
    gcc -o stext Stext.c
    gcc -o smain Smain.c
    gcc -o client client.c
    ```

3. **Start the specialized servers**:
    Open two separate terminal windows and run the following:
    ```bash
    ./spdf
    ./stext
    ```

4. **Start the main server**:
    Open another terminal window and run:
    ```bash
    ./smain
    ```

5. **Run the client**:
    In a separate terminal window, execute the client program:
    ```bash
    ./client
    ```

## How the System Works

1. **Client-Server Communication**: The client interacts exclusively with the **Smain** server, sending requests for various file operations.
2. **Concurrent Client Handling**: The **Smain** server forks a new process for each incoming client request, ensuring concurrent processing.
3. **File Type-based Distribution**:
    - Requests involving `.pdf` files are forwarded to the **Spdf** server.
    - Requests involving `.txt` files are forwarded to the **Stext** server.
    - `.c` files are handled directly by the **Smain** server.
4. **On-Demand Connections**: The **Smain** server establishes connections with **Spdf** and **Stext** only when necessary, optimizing resource usage.
5. **Request Queueing**: While processing requests, **Smain** continues to listen for and queue new client requests.

## Supported Operations

| Command   | Description                                                       |
|-----------|-------------------------------------------------------------------|
| `ufile`   | Uploads a file to the system based on its type (`.c`, `.pdf`, `.txt`) |
| `dfile`   | Downloads a file from the system                                   |
| `rmfile`  | Removes a file from the system                                     |
| `dtar`    | Creates and downloads a tar archive of specified file types        |
| `display` | Lists files in a specified directory                               |

### Example Commands

- To upload a file:
    ```bash
    ufile <file-path>
    ```

- To download a file:
    ```bash
    dfile <file-name>
    ```

- To remove a file:
    ```bash
    rmfile <file-name>
    ```

- To create and download a tar archive of all `.pdf` files:
    ```bash
    dtar pdf
    ```

- To display the files in a directory:
    ```bash
    display <directory-path>
    ```

## Notes

- This project was developed as part of the COMP-8567 course, showcasing concepts of distributed systems, socket programming, and efficient file management.
- The system is designed for educational purposes and can be expanded for more complex file operations and additional server types.
