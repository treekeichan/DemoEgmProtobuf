/////////////////////////////////////////////////////////////////////////
// Sample using Google protocol buffers C++
//
//#include "stdafx.h"
#include <WinSock2.h>
#include <iostream>
#include <fstream>

#ifndef _TCHAR
#define _TCHAR char
#endif

#include "./egm/egm.pb.h" // generated by Google protoc.exe

#pragma comment(lib, "Ws2_32.lib")      // socket lib
#pragma comment(lib, "libprotobuf.lib") // protobuf lib

static int portNumber = 6510;
static unsigned int sequenceNumber = 0;

using namespace std;
using namespace abb::egm;


// Protobuf-C++ is supported by Google and no other third party libraries needed for the protobuf part. 
// It can be a bit tricky to build the Google tools in Windows but here is a guide on how to build 
// protobuf for Windows (http://eli.thegreenplace.net/2011/03/04/building-protobuf-examples-on-windows-with-msvc).
//
// When you have built libprotobuf.lib and protoc.exe:
//	 Run Google protoc to generate access classes, protoc --cpp_out=. egm.proto
//	 Create a win32 console application
//	 Add protobuf source as include directory
//	 Add the generated egm.pb.cc to the project (exclude the file from precompile headers)
//	 Copy the file below
//	 Compile and run
//
//
// Copyright (c) 2014, ABB
// All rights reserved.
//
// Redistribution and use in source and binary forms, with
// or without modification, are permitted provided that 
// the following conditions are met:
//
//    * Redistributions of source code must retain the 
//      above copyright notice, this list of conditions 
//      and the following disclaimer.
//    * Redistributions in binary form must reproduce the 
//      above copyright notice, this list of conditions 
//      and the following disclaimer in the documentation 
//      and/or other materials provided with the 
//      distribution.
//    * Neither the name of ABB nor the names of its 
//      contributors may be used to endorse or promote 
//      products derived from this software without 
//      specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF 
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//////////////////////////////////////////////////////////////////////////
// Create a simple sensor message
void CreateSensorMessage(EgmSensor* pSensorMessage)
{
    EgmHeader* header = new EgmHeader();
    header->set_mtype(EgmHeader_MessageType_MSGTYPE_CORRECTION);
    header->set_seqno(sequenceNumber++);
    header->set_tm(GetTickCount());

    pSensorMessage->set_allocated_header(header);

    EgmCartesian *pc = new EgmCartesian();
    pc->set_x(1.1);
    pc->set_y(2.2);
    pc->set_z(3.3);

    EgmQuaternion *pq = new EgmQuaternion();
    pq->set_u0(1.0);
    pq->set_u1(0.0);
    pq->set_u2(0.0);
    pq->set_u3(0.0);

    EgmPose *pcartesian = new EgmPose();
    pcartesian->set_allocated_orient(pq);
    pcartesian->set_allocated_pos(pc);

    EgmPlanned *planned = new EgmPlanned();
    planned->set_allocated_cartesian(pcartesian);

    pSensorMessage->set_allocated_planned(planned);
}

//////////////////////////////////////////////////////////////////////////
// Display inbound robot message
void DisplayRobotMessage(EgmRobot *pRobotMessage)
{
    if (pRobotMessage->has_header() && pRobotMessage->header().has_seqno() && pRobotMessage->header().has_tm() && pRobotMessage->header().has_mtype())
    {
        printf("SeqNo=%d Tm=%u Type=%d\n", pRobotMessage->header().seqno(), pRobotMessage->header().tm(), pRobotMessage->header().mtype());
    }
    else
    {
        printf("No header\n");
    }
}

int _tmain(int argc, _TCHAR* argv[])
{
    SOCKET sockfd;
    int n;
    struct sockaddr_in serverAddr, clientAddr;
    int len;
    char protoMessage[1400];

    /* Init winsock */
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0)
    {
        fprintf(stderr, "Could not open Windows connection.\n");
        exit(0);
    }

    // create socket to listen on
    sockfd = ::socket(AF_INET,SOCK_DGRAM,0);

    memset(&serverAddr, sizeof(serverAddr), 0);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(portNumber);

    // listen on all interfaces
    bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));

    string messageBuffer;
    for (int messages = 0; messages < 100; messages++)
    {
        // receive and display message from robot
        len = sizeof(clientAddr);
        n = recvfrom(sockfd, protoMessage, 1400, 0, (struct sockaddr *)&clientAddr, &len);
        if (n < 0)
        {
            printf("Error receive message\n");
            continue;
        }

        // deserialize inbound message
        EgmRobot *pRobotMessage = new EgmRobot();
        pRobotMessage->ParseFromArray(protoMessage, n);
        DisplayRobotMessage(pRobotMessage);
        delete pRobotMessage;

        // create and send a sensor message
        EgmSensor *pSensorMessage = new EgmSensor();
        CreateSensorMessage(pSensorMessage);
        pSensorMessage->SerializeToString(&messageBuffer);

        // send a message to the robot
        n = sendto(sockfd, messageBuffer.c_str(), messageBuffer.length(), 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
        if (n < 0)
        {
            printf("Error send message\n");
        }
        delete pSensorMessage;
    }
    return 0;
}

