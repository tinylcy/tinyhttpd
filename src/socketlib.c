/*
 * Copyright (C) Chenyang Li
 * Copyright (C) Vino
 *
 * version 2017/12/01
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "error.h"
#include "socketlib.h"

int open_listenfd(const char *port) {
    struct addrinfo hints, *listp, *p;
    int listenfd, optval = 1;

    /* Get a list of potential server addresses */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;                 /* Using TCP */
    hints.ai_flags    = AI_PASSIVE | AI_ADDRCONFIG;  /* On any IP address */
    hints.ai_flags   |= AI_NUMERICSERV;              /* Using a numeric port arg */

    if (getaddrinfo(NULL, port, &hints, &listp) != 0) {
        err_sys("[open_listenfd] getaddrinfo error");
    }

    /* Walk the list for one that we can bind to */
    for (p = listp; p; p = p->ai_next) {
        /* Create a listen socket descriptor, if failed, try the next */
        if ((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
            continue;
        }

        /* Eliminates "Address already in use" error from bind */
        if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int)) != 0) {
            err_sys("[open_listenfd] setsockopt error");
        }

        /* Bind the descriptor to the address */
        if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0) {
            break;
        }

        /* Bind failed, try the next */
        if (close(listenfd) < 0) {
            err_sys("[open_listenfd] close error");
        }
    }

    /* Clean up */
    freeaddrinfo(listp);

    /* No address worked */
    if (!p) {
        return -1;
    }

    /* Make it a listening socket ready to accept connection requests */
    if (listen(listenfd, BACKLOG) < 0) {
        if (close(listenfd) < 0) {
            err_sys("[open_listenfd] close error");
        }
        return -1;
    }

    return listenfd;
}

int open_clientfd(const char *hostname, const char *port) {
    struct addrinfo hints, *listp, *p;
    int clientfd;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;     /* Using TCP */
    hints.ai_flags    = AI_ADDRCONFIG;   /* Recommended for connection */
    hints.ai_flags   |= AI_NUMERICSERV;  /* Using a numeric port arg */

    if (getaddrinfo(hostname, port, &hints, &listp) != 0) {
        err_sys("[open_clientfd] getaddrinfo error");
    }

    /* Walk the list for one that we can successfully connect to */
    for (p = listp; p; p = p->ai_next) {
        /* Create a client socket descriptor */
        if ((clientfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
            err_sys("[open_clientfd] socket error");
        }

        /* Connect to the server */
        if (connect(clientfd, p->ai_addr, p->ai_addrlen) != -1) {
            break;
        }

        /* Connect failed, try another */
        if (close(clientfd) < 0) {
            err_sys("[open_clientfd] close error");
        }
    }

    /* Clean up */
    freeaddrinfo(listp);
    
    if (!p) {          /* All connects failed */
        return -1;
    } else {           /* The last connect succeeded */
        return clientfd;
    }
}
