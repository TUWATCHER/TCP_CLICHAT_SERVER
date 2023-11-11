#pragma once

#include <stdio.h> 
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>    //close  
#include <sys/types.h>  
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros  

#define PORT 7777
#define MESSAGE_LENGTH 1025
#define LISTEN_BUFFER 5
#define ERROR -1

void Run();

struct sockaddr_in address;
socklen_t addrlen;
int master_socket_fd, connection, bind_status, connection_status,
client_socket[5], max_clients = 5, client_sd, activity, max_sd, new_socket, requestCode;

const char* message = "Welcome to CLICHAT Server!";
char clientRequest[MESSAGE_LENGTH];
char serverResponse[MESSAGE_LENGTH];
int options = true;

fd_set readfds;

void Run()
{
    // All client sockets to 0
    for (int i = 0; i < max_clients; i++)
    {
        client_socket[i] = 0;
    }

    // Create Master socket
    master_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (master_socket_fd == 0)
    {
        perror("Socket creation failed!\n");
        exit(EXIT_FAILURE);
    }

    // Set master socket to allow multiple connections
    if (setsockopt(master_socket_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&options, sizeof(options)) < 0)
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Set address
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(PORT);
    address.sin_family = AF_INET;

    // Bind socket
    bind_status = bind(master_socket_fd, (struct sockaddr*)& address, sizeof(address));
    if (bind_status == ERROR)
    {
        perror("Failed to bind socket!\n");
        exit(EXIT_FAILURE);
    }

    // Listen for new connections
    connection_status = listen(master_socket_fd, LISTEN_BUFFER);
    if (connection_status == ERROR)
    {
        perror("Failed to listen for new connections!\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        std::cout << "Server is listening for new connections on PORT " << PORT << std::endl;
    }

    addrlen = sizeof(address);
    std::cout << "Wating for connections..." << std::endl;

    while (true)
    {
        //clear the socket set  
        FD_ZERO(&readfds);   
     
        //add master socket to set  
        FD_SET(master_socket_fd, &readfds);   
        max_sd = master_socket_fd;   
             
        //add child sockets to set  
        for (int i = 0; i < max_clients; i++)   
        {   
            //socket descriptor  
            client_sd = client_socket[i];   
                 
            //if valid socket descriptor then add to read list  
            if(client_sd > 0)   
                FD_SET(client_sd , &readfds);   
                 
            //highest file descriptor number, need it for the select function  
            if(client_sd > max_sd)   
                max_sd = client_sd;   
        }   

        //wait for an activity on one of the sockets , timeout is NULL ,  
        //so wait indefinitely  
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);   
       
        if ((activity < 0) && (errno!=EINTR))   
        {   
            perror("Select Error!");
            exit(EXIT_FAILURE);   
        }   

        if (FD_ISSET(master_socket_fd, &readfds))   
        {   
            if ((new_socket = accept(master_socket_fd,  
                    (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)   
            {   
                perror("accept");   
                exit(EXIT_FAILURE);   
            }   
             
            //inform user of socket number - used in send and receive commands
            std::cout << "New connection! Socket FD: " << new_socket
                        << " IP: " << inet_ntoa(address.sin_addr)
                        << " PORT: " << ntohs(address.sin_port)
                        << std::endl;            
           
            //send new connection greeting message  
            if(send(new_socket, message, strlen(message), 0) != strlen(message))   
            {   
                perror("SEND ERROR");   
            }   
                 
            std::cout << "Welcome message sent successfully!\n";  
                 
            //add new socket to array of sockets  
            for (int i = 0; i < max_clients; i++)   
            {   
                //if position is empty  
                if(client_socket[i] == 0 )   
                {   
                    client_socket[i] = new_socket;
                    std::cout << "Adding to list of client sockets as " << i << std::endl;             
                    break;   
                }   
            }   
        }   
             
        //else its some IO operation on some other socket 
        for (int i = 0; i < max_clients; i++)   
        {   
            client_sd = client_socket[i];   
                 
            if (FD_ISSET(client_sd , &readfds))   
            {   
                bzero(clientRequest, MESSAGE_LENGTH);
                //Check if it was for closing , and also read the  
                //incoming message  
                if ((requestCode = recv(client_sd , clientRequest, sizeof(clientRequest), 0)) == 0)   
                {   
                    //Somebody disconnected , get his details and print  
                    getpeername(client_sd , (struct sockaddr*)&address, (socklen_t*)&addrlen);

                    std::cout << "Host disconnected! IP: " <<  inet_ntoa(address.sin_addr)
                                << " PORT: " << ntohs(address.sin_port)
                                << std::endl;  
                         
                    //Close the socket and mark as 0 in list for reuse  
                    close(client_sd);   
                    client_socket[i] = 0;   
                }   
                     
                // Send messages back to clients
                else 
                {
                    getpeername(client_sd , (struct sockaddr*)&address, (socklen_t*)&addrlen);
                    
                    ssize_t bytes;

                    std::cout << "Receiving packets from client " << inet_ntoa(address.sin_addr) << std::endl;             
                    std::cout << clientRequest;                   

                    for ( int j = 0; j < max_clients; ++j)
                    {
                        // Skip original sender of message
                        if (client_sd == client_socket[j] || j == i)
                        {
                            continue;
                        }

                        if (client_socket[j] != 0)
                        {
                            bytes = send(client_socket[j], clientRequest, sizeof(clientRequest), 0);
                        }
                        
                    }                   
                    
                }   
            }   
        } 
    }
    
    close(master_socket_fd);
}