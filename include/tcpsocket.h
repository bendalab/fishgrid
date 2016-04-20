/*
  tcpsocket.h
  A collection of functions for dealing with sockets.

  FishGridRecorder
  Copyright (C) 2011 Jan Benda <benda@bio.lmu.de> & Joerg Henninger <henninger@bio.lmu.de>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.
  
  FishGrid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _TCPSOCKET_H_
#define _TCPSOCKET_H_ 1

#include <cstdlib>
#include <cstdio>
#include <ctype.h>
#include <cerrno>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <termios.h>
#include <sys/fcntl.h>
#include <iostream>

using namespace std;


/*! 
\class TCPSocket
\brief A collection of functions for dealing with sockets.
\author Joerg Henninger
*/


class TCPSocket
{

public:

protected:

    /*! Maximum package size. */
  static const int BUFFERsize = 4096;

  struct MetaData {
    int SizeData;
    char Command[20];
  };

  // list stuff 
  struct list_entry {
    int sock;
    struct list_entry *next;
  }; 
  
  struct list_type {
    struct list_entry *data;
    unsigned count;
  };
  
  static const int BUF_SIZ=4096;
  static const int PORT=7000;

    /*! Start socket. \return the socket handle. */
  int socket( void );
  int bind( int sock, int port );
  int listen( int sock, int n );
  int accept( int sock );
  int connect( int sock, char * h, int port );
  int recv( int sock, struct MetaData &metaData, float **vec );
  int send( int s, struct MetaData &metaData, float *vec );
  int close( int s );
  void init_list(struct list_type *list);
  int add_client(struct list_type *list, int sock);
  int remove_client(struct list_type *list, int sock);
  int fill_set(fd_set *fds, struct list_type *list);
  int get_sender(fd_set *fds);
  int clear_list( struct list_type &list );



};


#endif /* ! _TCPSOCKET_H_ */

