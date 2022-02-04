#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define MAX_LEN 1000

char *removeChar(char *str, char charToRemove) {
	const size_t strSize = strlen(str)+1;
	char *temp = malloc(strSize * sizeof(char));

	int tempIndex = 0;
	for (int i = 0; i < strSize; i++) {
		if (str[i] == charToRemove) continue;

		temp[tempIndex] = str[i];
		tempIndex++;
	}

	//free(temp);
	return temp;
}



//handle database request commands
int getdb(char requestCmd[MAX_LEN]) {
	FILE *fpointer;
	char *commandBody;
	char *findCommandBody;
	char *requestTokens;
	int search;
	char fileLine[MAX_LEN];
	char *rowFind;
	char *rowFindEnd;
	char rowFound[MAX_LEN];
	char *columnFind;
	char columnFound[MAX_LEN];
	char *columnFindEnd;
	char requestTokensTemp[MAX_LEN];
	int match = 0;
	int returnSuccess = 0;
	char *beforeList;
	char *nowList;
	char *columnFoundFull;
	char *endOfColumn;
	char rowFoundBeg[MAX_LEN];
	int append = 0;
	char appendInsert[MAX_LEN];

	char *saveptr1, *saveptr2;

	requestTokens = strtok_r(requestCmd, ";", &saveptr1);

	while (requestTokens != NULL) {
		strcpy(requestTokensTemp, requestTokens);

		if (strstr(requestTokens, "OPENR") != NULL) {
			if ((commandBody = strchr(requestTokensTemp, '/')) != NULL) {
				if ((fpointer = fopen(commandBody, "r")) == NULL) {
					perror("openr");
					fclose(fpointer);
					return -6;
				}
			} else {
				fclose(fpointer);
				return -5;
			}
		} else if (strstr(requestTokens, "OPENA") != NULL) {
			if ((commandBody = strchr(requestTokensTemp, '/')) != NULL) {
				if ((fpointer = fopen(commandBody, "a")) == NULL) {
					perror("opena");
					fclose(fpointer);
					return -6;
				}
				append = 1;
			} else {
				fclose(fpointer);
				return -5;
			}
		} else if (strstr(requestTokens, "SEARCH") != NULL) {
			if ((findCommandBody = strrchr(requestTokensTemp, ' '))) {

				commandBody = removeChar(findCommandBody, ' ');

				if (strcmp(commandBody, "true") == 0) {
					search = 1;
				} else if (strcmp(commandBody, "false") == 0) {
					search = 0;
				} else {
					fclose(fpointer);
					return -7;
				}
			} else {
				fclose(fpointer);
				return -5;
			}
		} else if (strstr(requestTokens, "ROW") != NULL) {
			if ((findCommandBody = strrchr(requestTokensTemp, ' ')) != NULL) {

				commandBody = removeChar(findCommandBody, ' ');

				if (append == 1) {
					fprintf(fpointer, "\n");
					strcat(appendInsert, ";");
					strcat(appendInsert, commandBody);
				} else if (append == 0) {

					while (1) {
						fread(fileLine, sizeof(fileLine), MAX_LEN-1, fpointer);

						if ((rowFind = strstr(fileLine, commandBody)) != NULL) {
							strcpy(rowFound, rowFind);

							if ((rowFindEnd = strchr(rowFound, ';')) != NULL) {
								strcpy(rowFindEnd+1, "");

								strcpy(rowFoundBeg, rowFound);

								if ((endOfColumn = strchr(rowFoundBeg, ' ')) != NULL) {
									strcpy(endOfColumn, "");
								}
								if (strcmp(rowFoundBeg, commandBody) == 0) {
									printf("rowFound: %s\n", rowFound);
									returnSuccess = 4;
									break;
								} else {
									return -1;
									break;
								}
							} else {
								fclose(fpointer);
								return -2;
							}
						} else {
							fclose(fpointer);
							return -1;
						}
					}
				}
			} else {
				fclose(fpointer);
				return -5;
			}
		} else if (strstr(requestTokens, "COLUMN") != NULL) {
			if ((findCommandBody = strrchr(requestTokensTemp, ' ')) != NULL) {

				commandBody = removeChar(findCommandBody, ' ');
				
				if (append == 1) {
					strcat(appendInsert, " ");
					strcat(appendInsert, commandBody);
				} else if (append == 0) {
					if (search == 0) {
						if ((columnFind = strstr(rowFound, commandBody)) != NULL) {
							strcat(columnFound, columnFind);

							if ((columnFindEnd = strchr(columnFound, ' ')) != NULL || (columnFindEnd = strchr(columnFound, ';')) != NULL) {
								strcpy(columnFound, removeChar(columnFound, ';'));
								if (strcmp(columnFound, commandBody) == 0) {
									strcpy(columnFindEnd, "");
									printf("column found: %s\n", columnFound);
									returnSuccess = 5;
								} else {
									returnSuccess = -3;
								}
							} else {
								fclose(fpointer);
								return -2;
							}
						} else {
							fclose(fpointer);
							return -3;
						}

					} else if (search == 1) {
						strcpy(columnFound, commandBody);
						
						if (feof(fpointer) != 0) {
							fclose(fpointer);
							return -8;
						}

						int fileSize;

						fseek(fpointer, 0, SEEK_END);
						fileSize = ftell(fpointer);
						fseek(fpointer, 0, SEEK_SET);

						char fullFile[fileSize];

						while (feof(fpointer) == 0) {
							fread(fileLine, sizeof(fileLine), MAX_LEN-1, fpointer);
							strcat(fullFile, fileLine);
						}
						
						char *searchTokens = strtok_r(fullFile, ";", &saveptr2);

						
						while (searchTokens != NULL) {

							if ((columnFoundFull = strstr(searchTokens, columnFound)) != NULL) {
								if ((endOfColumn = strchr(columnFoundFull, ' ')) != NULL) {
									strcpy(endOfColumn, "");
								} else if ((endOfColumn = strchr(columnFoundFull, ';')) != NULL) {
									strcpy(endOfColumn, "");
								}
								
								if (strcmp(columnFoundFull, columnFound) == 0) {
									if (nowList == NULL) {
										nowList = searchTokens;
									} else {
										beforeList = nowList;

										nowList = searchTokens;


										if (strcmp(nowList, beforeList) == 0) {
											returnSuccess = 1;
										}
									}

									match++;
								}
							}

							searchTokens = strtok_r(NULL, ";", &saveptr2);
						}

						if (match > 0 && returnSuccess != 1) {
							returnSuccess = 2;
						}

						fseek(fpointer, 0, SEEK_SET);
					}
				}
			} else {
				fclose(fpointer);
				return -5;
			}
		} else {
			fclose(fpointer);
			return -4;
		}

		requestTokens = strtok_r(NULL, ";", &saveptr1);
	}

	

	if (append == 1) {
		strcat(appendInsert, ";");

		fprintf(fpointer, "%s", appendInsert);
	}



	fclose(fpointer);

	return returnSuccess;
}


void convertToDB(char *dbToken) {
    char *temp = strtok(dbToken, "&");
    char *tempput;
    char *tempput2;
    char finalPut[MAX_LEN];
    int doOnce = 1;

    //run through input token and convert into Rsql string
    while (temp != NULL) {
        tempput = strchr(temp, '=');
        tempput2 = removeChar(tempput, '=');
        
        if (doOnce == 1) {
            strcat(finalPut, "OPENA /Users/rowan/Desktop/webServer(Mac)/test.txt; ROW 1.");
            doOnce = 0;
        }
        strcat(finalPut, "; COLUMN ");
        strcat(finalPut, tempput2);

        temp = strtok(NULL, "&");
    }
    
    strcat(finalPut, ";");

    getdb(finalPut);
}


//rsqlString is the string that has been found after using findRSQL
//parses the rsql that has been found and enters/reads it from the database
void parseRSQL(char *rsqlString) {
	char *rsqlString2 = malloc(sizeof(rsqlString));
	char *fidStart;
	char *fidEnd;
	char *cmdStart;
	char *cmdEnd;
	char *trueCmdStartBefore;
	char *trueCmdEnd;
	char *trueCmdStart;
	char *trueFidStartBefore;
	char *trueFidEnd;
	char *trueFidStart;

	strcpy(rsqlString2, rsqlString);

	//form id start and end
	fidStart = strstr(rsqlString, "fid:");
	fidEnd = strstr(rsqlString, "cmd:");

	strcpy(fidEnd, "");

	//cmd start and end
	cmdStart = strstr(rsqlString2, "cmd:");
	cmdEnd = strrchr(rsqlString2, ';');
	strcpy(cmdEnd, "");


	//true cmd start and end
	trueCmdStartBefore = strchr(cmdStart, '"');
	trueCmdEnd = strrchr(cmdStart, '"');


	strcpy(trueCmdEnd, "");
	trueCmdStart = removeChar(trueCmdStartBefore, '"');


	//true form id start and end
	trueFidStartBefore = strchr(fidStart, '"');
	trueFidEnd = strrchr(fidStart, '"');


	strcpy(trueFidEnd, "");
	trueFidStart = removeChar(trueFidStartBefore, '"');

	printf("fid: %s\n", trueFidStart);
	printf("cmd: %s\n", trueCmdStart);
	//trueCmdStart is what stores the rsql command
	//trueFidStart is what stores the id of the form
	//when the post request is logged, take the variable names and organize them based on what has been specified by the variables in the cmd
}


//-1: file doesnt exist or could not be opened
//0: success
//1: no rsql found
//this reads from the html file being sent across and finds any special instructions for the server to execute
//sendFileDir is the file directory of the file being sent to the client
int findRSQL(char *sendFileDir) {
	char *startOfRsql;
	char *endOfRsql;
	FILE *filePointer;


    filePointer = fopen(sendFileDir, "r");
    if (feof(filePointer) || filePointer == NULL) return -1;




    int fileSize;

    fseek(filePointer, 0L, SEEK_END);
    fileSize = ftell(filePointer);

    char fullString[fileSize];

    fclose(filePointer);


    filePointer = fopen(sendFileDir, "r");

    fread(fullString, sizeof(fullString), sizeof(fullString), filePointer);

    fclose(filePointer);
    


    if ((startOfRsql = strstr(fullString, "<rsql>")) == NULL) return 1;
    if ((endOfRsql = strstr(fullString, "</rsql>")) == NULL) return 1;

    strcpy(endOfRsql+7, "");

    parseRSQL(startOfRsql);

    return 0;
}

