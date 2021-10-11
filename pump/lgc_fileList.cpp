#include <sys/types.h>
#include <dirent.h>
#include <string.h>


#include "lgc_fileList.h"
#include "lgc_param.h"
#include "lgc_ckpt.h"
#include "lgc_api.h"

static bool isFinish(const LGC_MediaFileInfo& fileInfo)
{
	const char* fileName = fileInfo.fileName;
	const char* p = strrchr(fileName,'.');
	p--;
	return *p == 'E';
}

static bool isValidOfMediaFileName(const char* mediaFileName)
{
	const char* pE = strstr(mediaFileName, "_E.lgc");
	const char* pS = strstr(mediaFileName, "_S.lgc");
	const char* p = NULL;
	const char* end = NULL;
	if(pE == NULL && pS == NULL)
		return false;
	end = pE == NULL ? pS:pE;
	
	for(p=mediaFileName; p < end; p++){
		if( !(*p >= '0' && *p <= '9') )
			return false;
	}
	return true;
}

bool operator<(const LGC_MediaFileInfo& src, const LGC_MediaFileInfo& dest)
{
	DWORD seq1 = src.getSequence();
	DWORD seq2 = dest.getSequence();

	return seq1 < seq2;
}

DWORD LGC_MediaFileInfo::getSequence() const
{
	const char* fileName = this->fileName;
	if(isValidOfMediaFileName(fileName) == false){
		lgc_errMsg("fileName ivalid: %s \n", fileName);
		exit(1);
	}

	char sequenceStr[126] = {0};
	unsigned int sequence = 0;
	const char* pE = strstr(fileName, "_E.lgc");
	const char* pS = strstr(fileName, "_S.lgc");
	const char* p = pE == NULL? pS:pE;
	strncpy(sequenceStr, fileName, p - fileName);

	sequence = (unsigned int)atoi(sequenceStr);

	return sequence;
}



/////////////////////////////////////////////////////////////////////
//LGC_FileList
LGC_FileList::LGC_FileList()
{
	return;
}
	
LGC_FileList::~LGC_FileList()
{
	return;
}

bool LGC_FileList::loadAllMediaFile()
{
	DIR* pDir = NULL;
    struct dirent* pDirent = NULL;

	const char* mediaDir = LGC_Param::getInstance()->getMediaFileDir();
	fprintf(stdout, "mediaDir=%s\n", mediaDir);

	LGC_MediaFileInfo fileInfo;

    pDir = opendir(mediaDir);
    if(pDir == NULL){
                lgc_errMsg("open dir failed:%s\n", mediaDir);
                return false;
    }

    while( (pDirent = readdir(pDir) ) != NULL){
		if( strcmp(pDirent->d_name,".") == 0 || strcmp(pDirent->d_name,"..") == 0)
			continue;
        
		//skip mediaFile that mediaFileName invalid
		if( isValidOfMediaFileName(pDirent->d_name) == false)
			continue;

		//push_back valid mediaFile to list of mediaFile
		strcpy(fileInfo.fileName,pDirent->d_name);
		this->push_back(fileInfo);
   }
    //sort list of mediaFile according to sequence of mediaFile
    this->sort();

	//update statistical member viarables
	m_current_it = m_file_container.begin();

	//closedir
	closedir(pDir);

	return true;

}

LGC_MediaFileInfo* LGC_FileList::getNextMediaFile()
{
	return m_current_it == m_file_container.end()? NULL: &(*m_current_it++);
}

bool LGC_FileList::removeAllSendedFile()
{
	list<LGC_MediaFileInfo>::iterator it;

	for(it=m_file_container.begin(); it != m_current_it; it++){
		//m_file_container.remove(it);
	}

	m_file_container.clear();
	return true;
}

void LGC_FileList::view()
{
	list<LGC_MediaFileInfo>::iterator it;

	for(it=m_file_container.begin(); it != m_file_container.end(); it++){
		fprintf(stdout, "%s\n", (*it).fileName);
	}
	return;
}

bool LGC_FileList::push_back(const LGC_MediaFileInfo& fileInfo)
{
	if( isFinish(fileInfo) == false){
		return true;//ignore
	}
	
//	BYTE8 ckpt_scn = LGC_CKPT::getInstance()->getLastCommitSCN();
//	BYTE8 scn = getSCN(fileInfo);
//	if(scn <= ckpt_scn ){//the media file have sended don't send again
//		return true;
//	}

	m_file_container.push_back(fileInfo);
	return true;
}

void LGC_FileList::sort()
{
	m_file_container.sort();
	return;
}




