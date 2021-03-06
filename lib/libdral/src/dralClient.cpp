/**************************************************************************
 * Copyright (c) 2014, Intel Corporation
 *
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are 
 * met:
 * 
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright 
 *   notice, this list of conditions and the following disclaimer in the 
 *   documentation and/or other materials provided with the distribution.
 * - Neither the name of the Intel Corporation nor the names of its 
 *   contributors may be used to endorse or promote products derived from 
 *   this software without specific prior written permission.
 *  
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A 
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY 
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file dralClient.cpp
 * @author Pau Cabre
 * @brief dral client
 */

#include <sstream>
#include <unistd.h>
#include "asim/dralClient.h"
#include "asim/dralClientDefines.h"
using namespace std;



/**
 * constructor with the file descriptor from where the dral client
 * will get data, the listener, and the read buffer size
 */
DRAL_CLIENT_CLASS::DRAL_CLIENT_CLASS (
    int fd, DRAL_LISTENER listener, UINT32 buffer_size)
{
    UINT16 type;
    UINT16 ver;
    ostringstream sout;

    error=false;

    dralRead = new DRAL_BUFFERED_READ_CLASS (fd,listener,buffer_size);
    if (dralRead == NULL)
    {
        type=DRAL_ERROR;
    }
    else
    {
        type=CheckFileType(&ver);
    }
    switch (type)
    {
      case DRAL_BINARY_0:
        implementation = new DRAL_CLIENT_BINARY_0_IMPLEMENTATION_CLASS (
            dralRead,listener);
        listener->Version(0);
        break;
      case DRAL_BINARY_1:
        implementation = new DRAL_CLIENT_BINARY_1_IMPLEMENTATION_CLASS (
            dralRead,listener);
        listener->Version(1);
        break;
      case DRAL_BINARY_2:
        implementation = new DRAL_CLIENT_BINARY_2_IMPLEMENTATION_CLASS (
            dralRead,listener);
        listener->Version(2);
        break;
      case DRAL_BINARY_3:
        implementation = new DRAL_CLIENT_BINARY_3_IMPLEMENTATION_CLASS (
            dralRead,listener);
        listener->Version(3);
        break;
      case DRAL_BINARY_4:
        implementation = new DRAL_CLIENT_BINARY_4_IMPLEMENTATION_CLASS (
            dralRead,listener);
        listener->Version(4);
        break;
      case DRAL_ASCII_0_1:
        implementation = new DRAL_CLIENT_ASCII_IMPLEMENTATION_CLASS (
            dralRead,listener);
        listener->Version(1);
        break;
      case DRAL_UNSUPORTED_ASCII_VERSION:
        sout << "The file you are trying to read has ascii format version "
        << ver << ". " << endl
        << "There is only ascii support for versions lower or equal to "
        << DRAL_CLIENT_ASCII_VERSION_MAJOR << endl
        << "Please generate your trace using the binary format." << endl;
        listener->Error((char *)sout.str().c_str());
        listener->EndSimulation();
        error=true;
        implementation=NULL;
        break;
      case DRAL_UNSUPORTED_VERSION:
        sout << "The file you are tyring to read is version "
        << ver << " while the Dral Client library you are using is still "
        << DRAL_CLIENT_VERSION_MAJOR << endl
        << "Please update your client and recomplie it." << ends;

        listener->Error((char *)sout.str().c_str());
        listener->EndSimulation();
        error=true;
        implementation=NULL;
        break;
      case DRAL_UNSUPORTED_FILE:
        sout << "The file you are trying to read does not appear to be a dral "
        << "file or it uses a very old dral format. If you are sure it is a "
        << "dral file, you will need to generate it again after updating "
        << "the core" << ends;
        listener->Error((char *)sout.str().data());
        listener->EndSimulation();
        error=true;
        implementation=NULL;
        break;
      case DRAL_ERROR_READING_FILE:
        listener->Error("Error reading the file");
        listener->EndSimulation();
        error=true;
        implementation=NULL;
        break;
      case DRAL_EMPTY_FILE:
        listener->Error("Reading from an empty file");
        listener->EndSimulation();
        error=true;
        implementation=NULL;
        break;
      case DRAL_ERROR:
        listener->Error("Error creating the DRAL_BUFFERED_READ_CLASS");
        listener->EndSimulation();
        error=true;
        implementation=NULL;
    }
}


/*
 * void destructor
 */
DRAL_CLIENT_CLASS::~DRAL_CLIENT_CLASS (void)
{
    if (implementation != NULL)
    {
        delete implementation;
    }
    
    if (dralRead != NULL)
    {
        delete dralRead;
    }
}


UINT16
DRAL_CLIENT_CLASS::CheckFileType (UINT16 * ver)
{
    INT64 r;

    struct versionFormat
    {
        UINT64 commandCode      : 6;
        UINT64 reserved         : 26;
        UINT64 major_version    : 16;
        UINT64 minor_version    : 16;
    } * command;

    command = (versionFormat *)dralRead->Read(sizeof(*command),&r);

    if (r == -1)
    {
        return DRAL_ERROR_READING_FILE;
    }
    if (command == NULL)
    {
        return DRAL_EMPTY_FILE;
    }

    if (!strncmp("version ",(char *)command,sizeof(*command)))
    {
        stringstream sout;
        char * c;
        UINT16 major;

        c = (char *)dralRead->Read(1,&r);
        while (r!=0 && r!=-1 && (*c)!='\n')
        {
            sout << *c;
            c = (char *)dralRead->Read(1,&r);
        }

        if (r != 1)
        {
            return DRAL_ERROR_READING_FILE;
        }
        
        sout.ignore(6); // ignore 'major '
        
        sout >> major;
        
        if (major > DRAL_CLIENT_ASCII_VERSION_MAJOR)
        {
            *ver=major;
            return DRAL_UNSUPORTED_ASCII_VERSION;
        }
        else if (major <= DRAL_CLIENT_VERSION_MAJOR)
        {
            return DRAL_ASCII_0_1;
        }
        else
        {
            *ver=major;
            return DRAL_UNSUPORTED_VERSION;
        }
    }
    else if (command->commandCode == DRAL_VERSION)
    {
        switch (command->major_version)
        {
          case 0:
            return DRAL_BINARY_0;
            break;
          case 1:
            return DRAL_BINARY_1;
            break;
          case 2:
            return DRAL_BINARY_2;
            break;
          case 3:
            return DRAL_BINARY_3;
            break;
          case 4:
            return DRAL_BINARY_4;
            break;
          default:
            *ver=command->major_version;
            return DRAL_UNSUPORTED_VERSION;
            break;
        }
    }
    else
    {
        return DRAL_UNSUPORTED_FILE;
    }
}

