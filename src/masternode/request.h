/**************************************************
 * Created: 2013/3/24
 * Author: Leo_xy
 * Email: xy198781@sina.com
 * Last modified: 2013/3/24 9:24
 * Version: 0.1
 * File: src/masternode/request.h
 * Breif: Header for request handling mechanism.
 **************************************************/
#include <mpi.h>

#ifndef REQUEST_HEADER

typedef struct Request
{
    enum request_type {
        ADD,
        SUB
    };
    int arg1;
    int arg2;
    int rank;
    int tag;
} Request;

int
handle_request(Request* req)
{
    if(!req) return -1;
    switch (req->request_type)
    case ADD:
        
}


#endif
