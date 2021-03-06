//
//  MPUtoISOBMFFProcessor.cpp
//  Bento4
//
//  Created by Jason Justman on 2/17/19.
//


/*****************************************************************
|
    AP4 - MP4 File Dumper
|
|    Copyright 2002-2008 Axiomatic Systems, LLC
|
|
|    This file is derived from Bento4/AP4 (MP4 Atom Processing Library).
|
|    Unless you have obtained Bento4 under a difference license,
|    this version of Bento4 is Bento4|GPL.
|    Bento4|GPL is free software; you can redistribute it and/or modify
|    it under the terms of the GNU General Public License as published by
|    the Free Software Foundation; either version 2, or (at your option)
|    any later version.
|
|    Bento4|GPL is distributed in the hope that it will be useful,
|    but WITHOUT ANY WARRANTY; without even the implied warranty of
|    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
|    GNU General Public License for more details.
|
|    You should have received a copy of the GNU General Public License
|    along with Bento4|GPL; see the file COPYING.  If not, write to the
|    Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
|    02111-1307, USA.
|
 ****************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "Ap4.h"
#include <list>
using namespace std;

#ifdef __EMSCRIPTEN__
#include "emscripten.h"
#endif

/*----------------------------------------------------------------------
 |   constants
 +---------------------------------------------------------------------*/
#define BANNER "MPUProcessor - jjustman\n"

int sequenceNumber = 1;
AP4_TrunAtom* trunToComputeDataOffset = NULL;

void traverseChildren(AP4_Atom* toCheckAtom, list<AP4_Atom*> atomList) {
    AP4_AtomParent* parent = AP4_DYNAMIC_CAST(AP4_ContainerAtom, toCheckAtom);
    
    if(!parent) {
        return;
    }
    
    AP4_List<AP4_Atom>::Item* child = parent->GetChildren().FirstItem();
    while (child) {
        if(child->GetData()) {
            AP4_Atom* atom = child->GetData();
            AP4_UI32 m_Type2 = atom->GetType();

            atomList.push_back(atom);

            char name[5];
            AP4_FormatFourCharsPrintable(name, m_Type2);
            name[4] = '\0';
            
            // printf("atom is: %s\n", name);
            const char* toFindMfhd = "mfhd";
            if(strncmp(toFindMfhd, name, 4) == 0) {
                AP4_MfhdAtom* mfhdAtom = AP4_DYNAMIC_CAST(AP4_MfhdAtom, atom);
                mfhdAtom->SetSequenceNumber(sequenceNumber++);
            }
            
            const char* toFindTrun = "trun";
            const char* toRemoveTrak = "trak";
            const char* toRemoveTrex = "trex";
            const char* toRemoveTfdt = "tfdt";
            const char* toRemoveEdts = "edts";
            //remove tfhd?
            const char* toRemoveTraf = "traf";

            if(strncmp(toRemoveEdts, name, 4) == 0) {
                printf("removing %s\n", name);
                atom->Detach();
                //set atom to null so we don't traverse to this boxes children
                atom = NULL;
                
            } else if (strncmp(toRemoveTfdt, name, 4) == 0) {
                printf("removing %s\n", name);
                atom->Detach();
                atom = NULL; //no atom->children to traverse after the Tfdt..
            } else if(strncmp(toFindTrun, name, 4) == 0) {
                AP4_TrunAtom* trunAtom = AP4_DYNAMIC_CAST(AP4_TrunAtom, atom);
                //todo check with parent/tfhd.track id==1
                trunToComputeDataOffset = trunAtom;
                printf("setting trunToComputeDataOffset to: %p\n", trunToComputeDataOffset);
               // child = NULL;
                
            } else if(strncmp(toRemoveTrex, name, 4) == 0 ) {
                AP4_TrexAtom* trexAtom = AP4_DYNAMIC_CAST(AP4_TrexAtom, atom);
                if(trexAtom->GetTrackId() == 2) {
                    printf("removing %s\n", name);
                    atom->Detach();
                    atom = NULL;

                    //seems to throw up a segfault? delete atom;

                   // child = NULL;
                    
                 //   return;
                }
            } else if(strncmp(toRemoveTrak, name, 4) == 0 ) {
                //dynamic cast to figure out our track id
                AP4_TrakAtom* trakAtom = AP4_DYNAMIC_CAST(AP4_TrakAtom, atom);
                AP4_TkhdAtom* tkhdAtom = AP4_DYNAMIC_CAST(AP4_TkhdAtom, trakAtom->GetChild(AP4_ATOM_TYPE_TKHD));
                
                if(tkhdAtom->GetTrackId() == 2) {
                    printf("removing %s\n", name);
                 
                    trakAtom->Detach();
                    //don't iterate over our children...
                    atom = NULL;
                    child = NULL;
                  
                }
            } else if(strncmp(toRemoveTraf, name, 4) == 0 ) {
                AP4_ContainerAtom* traf = AP4_DYNAMIC_CAST(AP4_ContainerAtom, atom);
                AP4_TfhdAtom* tfhdAtom = AP4_DYNAMIC_CAST(AP4_TfhdAtom, traf->GetChild(AP4_ATOM_TYPE_TFHD));
                
                if(tfhdAtom->GetTrackId() == 2) {
                    printf("toRemoveTraf: removing %s\n", name);
                    if(trunToComputeDataOffset) {
                        
                        //20 is the size of the tfdt box removed earlier
                        trunToComputeDataOffset->SetDataOffset(trunToComputeDataOffset->GetDataOffset() - traf->GetSize() - 20);
                        printf("setting trunToComputeDataOffset to: %d\n", trunToComputeDataOffset->GetDataOffset());
                        
                        trunToComputeDataOffset = NULL;
                    }
                    //adjust the other trun offset
                    atom->Detach();
                    atom = NULL;
//                    child = NULL;
                   // child = NULL;
                    //seems to throw up a segfault? delete atom;

                 //   return;
                }
            }
            if(atom) {
                traverseChildren(atom, atomList);
            }
        }
        if(child) {
            child = child->GetNext();
        }
    }
}

int mpu_dump_count=0;

AP4_DataBuffer* mpuToISOBMFFProcessBoxes(uint8_t* full_mpu_payload, uint32_t full_mpu_payload_size, int mdat_size) {
	printf("in mpuToISOBMFFProcessBoxes");

	//AP4_Result result = AP4_FileByteStream::Create(filename, AP4_FileByteStream::STREAM_MODE_READ, input);
  //  AP4_MemoryByteStream
	//    AP4_MemoryByteStream(const AP4_UI08* buffer, AP4_Size size);
//
//	char* filename = (char*)calloc(20, sizeof(char));
//	snprintf(filename, 20, "mpudump-%d", mpu_dump_count++);
//	FILE* f = fopen(filename, "w");
//	for(int i=0; i < full_mpu_payload_size; i++) {
//		fwrite(&full_mpu_payload[i], 1, 1, f);
//
//	}
//	fclose(f);

	AP4_ByteStream* boxDumpConsoleOutput = NULL;
	AP4_FileByteStream::Create("-stderr", AP4_FileByteStream::STREAM_MODE_WRITE, boxDumpConsoleOutput);

	AP4_AtomInspector* inspector = new AP4_PrintInspector(*boxDumpConsoleOutput);
    inspector->SetVerbosity(3);

    AP4_MemoryByteStream* memoryInputByteStream = new AP4_MemoryByteStream(full_mpu_payload, full_mpu_payload_size);
    
    AP4_DataBuffer* dataBuffer = new AP4_DataBuffer(4096);

    // open the output memory buffer, assume we won't be bigger than our ingest payload size for now
    AP4_MemoryByteStream* memoryOutputByteStream = new AP4_MemoryByteStream(dataBuffer);
    
    // inspect the atoms one by one
    AP4_Atom* atom;
    list<AP4_Atom*> atomList;
    
    AP4_DefaultAtomFactory atom_factory;
    while (atom_factory.CreateAtomFromStream(*memoryInputByteStream, atom) == AP4_SUCCESS) {
        // remember the current stream position because the Inspect method
        // may read from the stream (there may be stream references in some
        // of the atoms
        AP4_Position position;
        memoryInputByteStream->Tell(position);

        AP4_UI32 m_Type = atom->GetType();
        char name[5];
        AP4_FormatFourCharsPrintable(name, m_Type);
        name[4] = '\0';
        printf("mpuToISOBMFFProcessBoxes - atom is: %s\n", name);

        traverseChildren(atom, atomList);
        atom->Write(*memoryOutputByteStream);
        
        memoryInputByteStream->Seek(position);
        atom->Inspect(*inspector);
        atomList.push_back(atom);
        
        memoryInputByteStream->Seek(position);

    }

    if(mdat_size > 0) {
		memoryOutputByteStream->WriteUI32(mdat_size+AP4_ATOM_HEADER_SIZE);
		memoryOutputByteStream->WriteUI32(AP4_ATOM_TYPE_MDAT);
    }
    
    //clean up our atom list
    while(atomList.size()) {
        
        AP4_Atom* toClear = atomList.front();
        atomList.pop_front();
    //    delete toClear;
    }
    
    if (boxDumpConsoleOutput) boxDumpConsoleOutput->Release();
    if (memoryInputByteStream) memoryInputByteStream->Release();

    delete inspector;
    return dataBuffer;
}

AP4_DataBuffer* mpuToDumpISOBMFFBoxes(uint8_t* full_mpu_payload, uint32_t full_mpu_payload_size, int mdat_size) {
    printf("in mpuToDumpISOBMFFBoxes");
    
    //AP4_Result result = AP4_FileByteStream::Create(filename, AP4_FileByteStream::STREAM_MODE_READ, input);
    //  AP4_MemoryByteStream
    //    AP4_MemoryByteStream(const AP4_UI08* buffer, AP4_Size size);
    //
    //    char* filename = (char*)calloc(20, sizeof(char));
    //    snprintf(filename, 20, "mpudump-%d", mpu_dump_count++);
    //    FILE* f = fopen(filename, "w");
    //    for(int i=0; i < full_mpu_payload_size; i++) {
    //        fwrite(&full_mpu_payload[i], 1, 1, f);
    //
    //    }
    //    fclose(f);
    
    AP4_ByteStream* boxDumpConsoleOutput = NULL;
    AP4_FileByteStream::Create("-stderr", AP4_FileByteStream::STREAM_MODE_WRITE, boxDumpConsoleOutput);
    
    AP4_AtomInspector* inspector = new AP4_PrintInspector(*boxDumpConsoleOutput);
    inspector->SetVerbosity(3);
    
    AP4_MemoryByteStream* memoryInputByteStream = new AP4_MemoryByteStream(full_mpu_payload, full_mpu_payload_size);
    
    AP4_DataBuffer* dataBuffer = new AP4_DataBuffer(4096);
    
    // open the output memory buffer, assume we won't be bigger than our ingest payload size for now
    AP4_MemoryByteStream* memoryOutputByteStream = new AP4_MemoryByteStream(dataBuffer);
    
    // inspect the atoms one by one
    AP4_Atom* atom;
    list<AP4_Atom*> atomList;
    
    AP4_DefaultAtomFactory atom_factory;
    while (atom_factory.CreateAtomFromStream(*memoryInputByteStream, atom) == AP4_SUCCESS) {
        // remember the current stream position because the Inspect method
        // may read from the stream (there may be stream references in some
        // of the atoms
        AP4_Position position;
        memoryInputByteStream->Tell(position);
        
       
        atom->Inspect(*inspector);
        
        memoryInputByteStream->Seek(position);
        
    }
    
    if (boxDumpConsoleOutput) boxDumpConsoleOutput->Release();
    if (memoryInputByteStream) memoryInputByteStream->Release();
    
    delete inspector;
    
    
    
    return dataBuffer;
}
