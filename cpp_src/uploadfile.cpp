#include"uplaodfile.h"

using namespace std;

bool Upload::AcceptFile() {
	while (fastCGI.FcgiAccept()) {
		
		fastCGI.contentLen = getenv("CONTENT_LENGTH");
		printf("Content-type: text/html\r\n\r\n");
		
		if (fastCGI.contentLen != NULL) {
			buflen = strtol(fastCGI.contentLen, nullptr, 10);
		}

		if (buflen <= 0) {
			cout << "No data from standard input.<p>\n" << endl;
		}
		else {
			char tmpch;
			
			fileData = (char*)malloc(buflen);
			pbegin = ptemp = fileData;
			
			for (int i = 0; i < buflen; ++i) {
				if ((tmpch = getchar()) < 0) {
					cout << "Not enough bytes received on standard input<p>\n" << endl;
					break;
				}
				*ptemp = tmpch;
				++ptemp;
			}

			pend = ptemp;
		}
		cout << fileData << endl;
		ParseDataAndSave();
		UploadFile(fileName);
		SaveToMysql();
	}

	return true;
}

bool Upload::ParseDataAndSave() {
	ptemp = strstr(pbegin, "\r\n");
	strncpy(fastCGI.boundary, pbegin, ptemp - pbegin);

	ptemp += 2;
	buflen -= (ptemp - pbegin);
	pbegin = ptemp;
	char* pfileNameBegin = strstr(pbegin, "filename=");
	pfileNameBegin += strlen("filename=");  
	char* pfileNameEnd = strchr(++pfileNameBegin, '"');  
	strncpy(fileName, pfileNameBegin, pfileNameEnd - pfileNameBegin);
	cout << fileName << endl;
	printf("<br>filename: %s<br>\n", fileName);

	ptemp = strstr(pbegin, "\r\n");
	ptemp += 2;
	buflen -= (ptemp - pbegin);  
	pbegin = ptemp;
	ptemp = strstr(pbegin, "\r\n");
	ptemp += 4;
	buflen -= (ptemp - pbegin);  


	pbegin = ptemp;

	int boderLen = strlen(fastCGI.boundary);
	int FullDataLen = buflen - boderLen + 1;
	for (int i = 0; i < FullDataLen; ++i) {
		if (*pbegin == *fastCGI.boundary) {
			if (memcmp(pbegin, fastCGI.boundary, boderLen) == 0) {
				break;
			}
		}
		++pbegin;
	}
	swap(pbegin, ptemp);
	if (ptemp == nullptr) {
		ptemp = pend;
	}

	ptemp -= 2;  


	int fd = open(fileName, O_CREAT | O_WRONLY, 0664);
	write(fd, pbegin, ptemp - pbegin);
	close(fd);

	return true;
}

bool Upload::UploadFile(char *fileName) {
	if (!fastDFS.FdfsClientInit()) {
		cout << "fastdfs initial failed." << endl;
		return false;
	}

	if (!fastDFS.TrackerGetConnection()) {
		cout << "tracker initial failed." << endl;
		fastDFS.FdfsClientDestroy();
		return false;
	}

	if (!fastDFS.TrackerQueryStorageStore()) {
		cout << "storage initial failed." << endl;
		fastDFS.FdfsClientDestroy();
		return false;
	}

	if (!fastDFS.StorageUploadByFilename1(fileName, fileId)) {
		cout << "StorageUploadByFilename1 initial failed." << endl;
		fastDFS.FdfsClientDestroy();
		return false;
	}

	if (!fastDFS.TrackerCloseConnectionEx()) {
		cout << "TrackerCloseConnectionEx initial failed." << endl;
		fastDFS.FdfsClientDestroy();
		return false;
	}

	fastDFS.FdfsClientDestroy();
	printf("<br>fileid: %s\n<br>", fileId);
	
	return true;
}

bool Upload::SaveToMysql() {
	char sql[SQL_LEN] = { '\0' };
	snprintf(sql, SQL_LEN, "insert into %s values(NULL, '%s', '%s')", TABLE_NAME, fileName, fileId);
	cout << sql << endl;
	int flag = mysql.MysqlQuery(sql);
	if (flag == 0) {
		return true;
	}

	return false;
}
