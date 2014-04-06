
Networks Lab
============

This is a multi-part assignment done as part of the Computer Networks Course in Spring 2014, IITM. 

- Part 1: Use UDP to set up a server-client command and file transfer system.
- Part 2: Use Sliding Window Protocol to ensure reliability and flow control by setting it up on both the end hosts.
- Part 3: Check your implementation of Sliding Window Protocol by introducting a machine in the middle that drops packets probabilitistically between the two.
- Part 4: Use pthreads to implement the dropping concurrently. Also, use a concurrent TCP server. 


Commands
-

Use the following commands:

```
 ./server <window_size>
 ```
 
 

```
 ./mim <server> <p> <q>
 ```
 
 
```
 ./client <server> <window_size>
 ```
 
 
