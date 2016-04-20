/*
  tcpsocket.cc
  A collection of functions for dealing with sockets.

  FishGrid
  Copyright (C) 2009 Jan Benda <benda@bio.lmu.de> & Joerg Henninger <henninger@bio.lmu.de>

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

#include "tcpsocket.h"


int TCPSocket::socket( void )
{	// Starte Server-Socket
  int sock = 0;
  if ((sock = ::socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("Socket");
  }
  cerr << "Socket created: " << sock << endl;
  return sock;
}


int TCPSocket::bind( int sock, int port )
{
  struct sockaddr_in server_addr;
  // Definiere Socket - Parameter // ACHTUNG: die Parameter werden erst fuer "bind" gebraucht
  server_addr.sin_family = AF_INET; // Adress-Bereich
  server_addr.sin_port = htons(port); // Port zum Horchen
  server_addr.sin_addr.s_addr = INADDR_ANY; // Alle Adressen kommen als Verbindung in Frage	

  // Binde Socket an den Port
  if ( ::bind(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) < 0 ) {
    perror("Unable to bind");
    exit(1);
    //~ return -1;
  }
  cerr << "Binding." << endl;
  return 0;
}


int TCPSocket::listen( int sock, int n )
{
  // Setze den Port in Listen-Modus: maximal 3 Verbindungen in der Warteschlange erlaubt
  if ( ::listen(sock, n) < 0 ) {
    perror("Listen failed");
    return -1;
  }

  return 0;
}


int TCPSocket::accept( int sock )
{
  // Akzeptiere die Verbindung und mache einen neuen Client auf, der mit dem Remote-Client kommuniziert
  socklen_t sin_size;
  sin_size = sizeof(struct sockaddr_in);
  struct sockaddr_in client_addr;
  int client = ::accept(sock, (struct sockaddr *)&client_addr,&sin_size);

  cerr << "I got a connection from (" << inet_ntoa(client_addr.sin_addr) << ", " << ntohs(client_addr.sin_port) << '\n';
	
  return client;
}


int TCPSocket::connect( int sock, char * h, int port )
{
  struct sockaddr_in server_addr;
  struct hostent *host;
  host = gethostbyname( h );
  // Definiere Socket - Parameter
  server_addr.sin_family = AF_INET; // Adress-Bereich
  server_addr.sin_port = htons(port); // Port des Servers
  server_addr.sin_addr = *((struct in_addr *)host->h_addr);
	
  if ( ::connect(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1 ) 
    {
      perror("Connect");
      return -1;
    }
  return 0;
}


int TCPSocket::recv( int sock, struct MetaData &metaData, float **vec )
{
  *vec = 0;
  // RECEIVE A STRUCTURE, and associated data
  int recvd = ::recv( sock, &metaData,sizeof(metaData), 0 );
  if (recvd < 0 ) {
    perror("recv() failed");
    return 0;
  }
  if (recvd == 0 ) {
    cerr << "Socket " << sock << " closed. Removing client from list." << endl;
    return 0;
  }
  cerr << "Datasize - Bytes received: " << recvd << endl;
  cerr << "Datasize - Command: " << metaData.Command << endl;

  // IF THERE IS DATA, RECEIVE DATA
  if ( metaData.SizeData > 0 ) {
    *vec = (float *)malloc( metaData.SizeData );
    char * dataPointer = (char *)(*vec);
    recvd = 0; // number of bytes that have been sent already
    int recvdlast = 0; // number of bytes that have been sent with last call
		
    while ( recvd < metaData.SizeData ) {
      recvdlast = ::recv(sock, dataPointer, metaData.SizeData-recvd, 0);
      // check, if recv worked alright
      if (recvdlast < 0 ) {
	perror("recv() failed");
	return -1;
      }
      else {
	//~ cerr << "received: " << recvd << endl;
	recvd += recvdlast;
	dataPointer += recvdlast;
      }
    }
    cerr << "Bytes received: " << recvd << endl;
  }
  return recvd;
}


int TCPSocket::send( int s, struct MetaData &metaData, float *vec )
{
  //	metaData.sizeData *= sizeof(float);
	
  // SEND A STRUCTURE, and associated data
  // send size of data-block as int
  int sentlast = 0; // number of bytes that have been sent with last call
  sentlast = ::send(s, &metaData, sizeof(metaData), 0);
  if ( sentlast == -1)
    {
      perror("send() failed");
      return 1;
    }
  cerr << "Sending size worked fine." << endl;
  cerr << "Sent data: " << sentlast << endl;

  // send data block
  int sent = 0; // number of bytes that have been sent already
  if ( metaData.SizeData > 0 ) 
    {
      char * dataPointer = (char *) &vec;
      sentlast = 0; // number of bytes that have been sent with last call
      while (sent < metaData.SizeData ) {
	// if end of data-block is far away
	if ( sent < metaData.SizeData - BUFFERsize ) 
	  {
	    sentlast = ::send(s, dataPointer, BUFFERsize, 0);
	    // check, if send worked alright
	    if (sentlast == -1 ) 
	      {
		perror("send() failed");
		return 1;
	      }
	    else {
	      sent += sentlast;
	      dataPointer += sentlast;
	    }
	  }
			
	// if end of data-block is close
	else 
	  {
	    sentlast = ::send(s, dataPointer, metaData.SizeData-sent, 0);
	    // check, if send worked alright
	    if (sentlast == -1 )
	      {
		perror("send() failed");
		return 1;
	      }
	    else 
	      {
		sent += sentlast;
		dataPointer += sentlast;
	      }
	  }
      }
    }
  cerr << "Sent data: " << sent << endl;
  cerr << endl;
	
  return sent;
}


int TCPSocket::close( int s )
{
  int cl = close(s);
  if (cl < 0 )
    {
      perror("close() failed");
      return -1;
    }
  return 0;
}


// ###############################
// ###############################
// Select

void TCPSocket::init_list(struct list_type *list)
{
  list->data = NULL;
  list->count = 0;
}


int TCPSocket::add_client(struct list_type *list, int sock)
{
  // adds a client to the top of the list // the server socket is the last entry of the list
  struct list_entry *n;

  n = (struct list_entry *) malloc(sizeof(*n));
  if (!n)
    {
      perror("malloc() failed");
      return 1;
    }

  n->sock = sock;
  n -> next = list->data;

  list->data = n;
  list->count++;

  return 0;
}


int TCPSocket::remove_client(struct list_type *list, int sock)
{
  // sock is the socket to be removed from the list
  struct list_entry *lz, *lst = NULL;

  if (!list->count)
    return 1;

  // search the list for sock; if sock is found, break; lst is the last entry before the socket, I guess
  for (lz = list->data; lz; lz = lz->next)
    {
      if (lz->sock == sock)
	break;
      lst = lz;
    }

	 
  if (!lz)
    return 1;

  // if lst is set (which happens to be when sock is not the first socket of the list), hand over the link to the next entry
  if (lst)
    lst->next = lz->next;
  // otherwise, it's the first entry we want to remove
  else
    list->data = lz->next;

  free(lz); // free space (opposite of malloc)
  list->count--; // reduce the cerr of list items

  return 0;
}

// die liste ins set fuellen
int TCPSocket::fill_set(fd_set *fds, struct list_type *list)
{
  int max = 0;
  struct list_entry *lz;

  for (lz = list->data; lz; lz = lz->next)
    {
      if (lz->sock > max)
	max = lz->sock;
      FD_SET(lz->sock, fds);
    }

  return max;
}

// find socket-event
int TCPSocket::get_sender(fd_set *fds)
{
  int i = 0;

  while(!FD_ISSET(i, fds))
    i++;

  return i;
}


int TCPSocket::clear_list( struct list_type &list )
{ // clear all list entries, except the server socket ( which is the last in the list )
  while ( list.count > 1 )
    { // remove all entries
      cerr << "Closing socket: " << list.data->sock << endl;
      close(list.data->sock);
      int r = remove_client( &list, list.data->sock );
      if (r > 0)
	return 1;
    }
  return 0;
}

