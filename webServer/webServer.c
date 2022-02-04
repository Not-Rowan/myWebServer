//            C Web Server
//     ==========================
//       By:Rowan Theodor Rothe
//     ==========================
//
//               _____
//              / | | \
//             |       |
//             | \___/ |
//              \_____/
//




//this is where you enter the default file directories
char *defaultDir1 = "/directory/to/siteDirs.txt";
char *defaultDir2 = "/directory/to/this/web/server/folder/";

//this is so i can use strcasestr
#define _GNU_SOURCE


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <ctype.h> //for strcasestr
#include <time.h> //logging current time

#include "c_db_r.h"



struct sockaddr_in sockaddr;
struct sockaddr_in clientaddr;
char webFileDir[9000];
char servFileDir[9000];
#define THREAD_POOL_SIZE 200
#define BACKLOG 200
int checkError;
//this will be set to 10,000 because that is the amount of bytes that have been initialized for the variable fullResp
int fullRespHeapSize = 10000;
int httpResponseSize = 10000;
int fileDirLen = 9000;
#define MAX_RESP_SIZE 10000

pthread_t thread_pool[THREAD_POOL_SIZE];




//creates a lock and unlock system for enqueue and dequeue
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
//creates a sleep timer for the threads/does something until given a signal or condition.
pthread_cond_t condition_var = PTHREAD_COND_INITIALIZER;

struct node {
    int client_socket;
    struct node *next;
};
typedef struct node node_t;

node_t *head = NULL;
node_t *tail = NULL;

void enqueue(int client_socket) {
    node_t *newnode = malloc(sizeof(node_t)); //create node
    newnode->client_socket = client_socket; //assign client socket
    newnode->next = NULL; //add node to the end of the list
    if (tail == NULL) { //checks if there is nothing on the tail and if so adds a head
        head = newnode;
    } else {
        //if there is a tail then add a new node onto the tail
        tail->next = newnode;
    }
    tail = newnode;
    //this all basically creates a new node then moves it over over and over again until stopped
}

int dequeue() {
    if (head == NULL) return -1;//if the head is empty or nothing has been enqueued then return null


    int result = head->client_socket;
    node_t *temp = head;
    head = head->next;
    if (head == NULL) {tail = NULL;}
    free(temp);
    return result;
    //this function removes everything off of the queue and returns the socket
    return 0;
}



//fullResp is the buffer that will store the request data
int *handleClient(int pclientSock, char *fullResp) {
    //if anything is initialized for no apparent reason, its bc gcc wants me to silence it and wont stfu
    FILE *fpointer;
    char *findExtention;
    char *clientIP;
    char *endDir;
    int freadErr;
    int fsize;
    int bin;
    int readSock = 0;
    char fileDir[fileDirLen];
    //char recvLineGET1[70];
    char httpResponseReport[httpResponseSize];
    char httpResponse[httpResponseSize];
    char *fileLine = 0;
    char *endOfPost;
    //char *splitPost;
    //char *postContentLength;
    //char *finalPostLength;
    //char tempPost[2000];
    //int contentLen;
    //char *refindBody;
    //char finalBody[100000];
    //char fullResp[100000];
    //char *sendFileDirectory;
    int checkSend;
    char *startDir;
    char *getEnd;
    int responseCode = 200;
    int nullOffset = 0;
    char *recvBufferTemp;
    int sizeToRead = 0;
    char *findHost;
    char *endHost;
    char *host = NULL;
    char *siteInfo, *siteInfoToken1, *siteInfoToken2, *siteInfoDomain, *siteInfoDir, *siteInfoBuffer;
    //i had to initialize this because if i do anything to an uninitialized variable, ill get a segfault
    char *siteInfoRoot = NULL;
    char *saveToken1, *saveToken2;
    char logdir[fileDirLen];




    //get client ip
    int acceptSock = pclientSock;
    clientIP = inet_ntoa(clientaddr.sin_addr);



    //strcpy(recvLineGET1, "~");

    //handles reading client http request
    do {
        //first check if fullResp is going to overflow. if it does then i will realloc the sizeof fullResp times 2. if there isnt anymore data to read, the while loop will stop.
        if (fullRespHeapSize < sizeToRead + nullOffset + 1 && sizeToRead+nullOffset+1 < MAX_RESP_SIZE) {
            fullRespHeapSize *= 2;
            if ((fullResp = realloc(fullResp, fullRespHeapSize)) == NULL) {
                perror("cant allocate enough memory on the heap");

                *fullResp = '\0';
                *recvBufferTemp = '\0';

                close(acceptSock);
                return 0;
            }
        } else if (sizeToRead+nullOffset+1 >= MAX_RESP_SIZE) {
            printf("allocating too much memory. allocation cancelled\n");

            *fullResp = '\0';
            *recvBufferTemp = '\0';

            close(acceptSock);
            return 0;
        }


        //set temporary buffer to read from. if it doesnt make sense why this copies recvBufferTemp to fullResp, it is because recvBufferTemp is equal to the memory address of fullResp so everything that goes to recvBufferTemp goes to fullResp
        recvBufferTemp = fullResp+nullOffset;
        //set size to read from
        sizeToRead = fullRespHeapSize - nullOffset -1;

        if (sizeToRead == 0) {
            printf("error getting read size\n");

            send(acceptSock, "HTTP/1.0 500\r\n\r\n", 16, MSG_NOSIGNAL);

            *fullResp = '\0';
            *recvBufferTemp = '\0';

            close(acceptSock);
            return 0;
        }

        //read to recvBufferTemp and make sure i cant read more than 1 megabyte
        if ((readSock = read(acceptSock, recvBufferTemp, sizeToRead)) <= 0 ) {
            //perror("error reading from socket");

            //error 500: internal server error
            send(acceptSock, "HTTP/1.0 500\r\n\r\n", 16, MSG_NOSIGNAL);

            *fullResp = '\0';
            *recvBufferTemp = '\0';

            close(acceptSock);
            return 0;
        }

        //increase the nullOffset so the program knows where to put the null terminator after
        nullOffset += readSock;

        //strcat(fullResp, recvLine);
        //once im done debugging the main GET request stuff, make sure to make a copy of fullResp so i can use that

        //fprintf(stdout, "typeofReq: %s\n", typeofReq);
    } while ((endOfPost = memchr(recvBufferTemp, '\n', readSock)) == NULL);
    //memset(fullResp, 0, 4000);
    //memset(tempPost, 0, 2000);
    fullResp[nullOffset] = '\0';
    
    
    
    
    
    //log time
    char currentTime[255];
    struct tm *sTm;
    
    time_t now = time(0);
    sTm = gmtime(&now);
    
    //format time to human readable form
    strftime(currentTime, sizeof(currentTime), "%Y-%m-%d %H:%M:%S", sTm);

    //logs client ip and request (includes text color changing for time (blue = \033[;34m))
    printf("\n\033[;34m[%s]", currentTime);
    printf("\033[0;00m");
    printf("%s\n\n%s.\n", clientIP, fullResp);


    strcpy(logdir, servFileDir);
    strcat(logdir, "serverlogs.txt");

    if ((fpointer = fopen(logdir, "r")) == NULL) {
        perror("error opening log file");

        *fullResp = '\0';
        send(acceptSock, "HTTP/1.0 500\r\n\r\n", 16, MSG_NOSIGNAL);

        close(acceptSock);
        fclose(fpointer);
        return 0;
    }

    fseek(fpointer, 0L, SEEK_END);
    fsize = ftell(fpointer);
    rewind(fpointer);
    fclose(fpointer);

    if (fsize >= 20000) {
        fpointer = fopen(logdir, "w");
    } else {
        fpointer = fopen(logdir, "a");
    }

    if (fpointer == NULL) {
        perror("error opening log file");

        *fullResp = '\0';
        send(acceptSock, "HTTP/1.0 500\r\n\r\n", 16, MSG_NOSIGNAL);

        close(acceptSock);
        fclose(fpointer);
        return 0;
    }

    fprintf(fpointer, "\n[%s] %s\n\n%s\n", currentTime, clientIP, fullResp);
    fclose(fpointer);
    



    //find which website has been requested and create a response based on that
    if ((findHost = strcasestr(fullResp, "Host:")) == NULL) {
            perror("error finding requested host");

            *fullResp = '\0';
            send(acceptSock, "HTTP/1.0 400\r\n\r\n", 16, MSG_NOSIGNAL);

            close(acceptSock);
            return 0;
    }

    if ((endHost = strchr(findHost, '\r')) == NULL) {
        perror("error finding end of requested host");

        *fullResp = '\0';
        send(acceptSock, "HTTP/1.0 400\r\n\r\n", 16, MSG_NOSIGNAL);

        close(acceptSock);
        return 0;
    }
    *endHost = '\0';
    host = findHost+6;
    


    if ((fpointer = fopen(defaultDir1, "r")) == NULL) {
        perror("error opening siteDirs.txt");

        *fullResp = '\0';
        send(acceptSock, "HTTP/1.0 500\r\n\r\n", 16, MSG_NOSIGNAL);

        close(acceptSock);
        return 0;
    }

    fseek(fpointer, 0L, SEEK_END);
    fsize = ftell(fpointer);
    rewind(fpointer);

    siteInfoBuffer = malloc(fsize*sizeof(char)+1);
    fread(siteInfoBuffer, fsize, 1, fpointer);


    siteInfoToken1 = strtok_r(siteInfoBuffer, "<", &saveToken1);

    //parse siteDirs.txt
    while (siteInfoToken1 != NULL) {
        siteInfoToken2 = strtok_r(siteInfoToken1, "\n", &saveToken2);
        
        //make sure i dont read more than i need to
        if (siteInfoDir != NULL && siteInfoRoot != NULL) {
            break;
        }

        while (siteInfoToken2 != NULL) {
            if ((siteInfo = strstr(siteInfoToken2, "sitedomain")) != NULL) {
                siteInfoDomain = siteInfo+11;
                
                if (strcmp(siteInfoDomain, host) == 0) {
                    siteInfoToken2 = strtok_r(NULL, "\n", &saveToken2);
                    continue;
                } else {
                    siteInfoDomain = NULL;
                    break;
                }
            }
            
            if (siteInfoDomain != NULL) {
                if ((siteInfo = strstr(siteInfoToken2, "sitedirectory")) != NULL) {
                    siteInfoDir = siteInfo+14;
                }

                if ((siteInfo = strstr(siteInfoToken2, "directoryroot")) != NULL) {
                    siteInfoRoot = siteInfo+14;
                }
            } else {
                printf("siteDirs.txt has been formatted improperly. Go fix that idiot\n");
                
                *fullResp = '\0';
                send(acceptSock, "HTTP/1.0 500\r\n\r\n", 16, MSG_NOSIGNAL);
        
                close(acceptSock);
                return 0;
            }

            siteInfoToken2 = strtok_r(NULL, "\n", &saveToken2);
        }

        siteInfoToken1 = strtok_r(NULL, "<", &saveToken1);
    }

    if (siteInfoDir != NULL) {
        strcpy(webFileDir, siteInfoDir);
        strcpy(fileDir, siteInfoDir);
    } else {
        printf("requested invalid webpage\n");
        strcpy(webFileDir, servFileDir);
        strcpy(fileDir, servFileDir);
    }

    //makes sure the http request is valid and logs the requested directory
    if ((getEnd = strstr(fullResp, "HTTP")) == NULL) {
        //perror("error reading from socket");

        *fullResp = '\0';
        //error 400: bad request
        send(acceptSock, "HTTP/1.0 400\r\n\r\n", 16, MSG_NOSIGNAL);
        
        close(acceptSock);
        return 0;
    }

    *getEnd = '\0';

    if ((startDir = strchr(fullResp, '/')) == NULL) {
        perror("this shouldnt happen .-.");

        //error 400: bad request
        send(acceptSock, "HTTP/1.0 400\r\n\r\n", 16, MSG_NOSIGNAL);

        *fullResp = '\0';

        close(acceptSock);
        return 0;
    }



    //handles the file retrieving
    //char *typeofReq = strchr(recvLineGET1, '~');
    //char *endReq = strchr(recvLineGET1, ' ');




    if ((endDir = strchr(startDir, ' ')) == NULL) {
        perror("endDir error");

        //error 500: internal server error
        send(acceptSock, "HTTP/1.0 500\r\n\r\n", 16, MSG_NOSIGNAL);

        *fileDir = '\0';
        *fullResp = '\0';

        close(acceptSock);
        return 0;
    }

    if (strchr(startDir, '?')) {
        endDir = strchr(startDir, '?');
    }


    *endDir = '\0';
    if (siteInfoDir != NULL) {
        strcat(fileDir, startDir);
    } else {
        strcat(fileDir, "err404.html");
    }

    //strcpy(endReq, "");


    //checks for requested directory
    if (strcmp(startDir, "/") == 0) {

        if (strlen(webFileDir) >= fileDirLen) {
            printf("\n\nbuffer overflow warning, please change the size of fileDirLen or make the size of webFileDir shorter\n\n");

            exit(1);
        }

        if (siteInfoRoot != NULL) {
            strcpy(fileDir, webFileDir);
            strcat(fileDir, siteInfoRoot);
            findExtention = ".html";
        } else {
            strcpy(fileDir, servFileDir);
            strcat(fileDir, "err404.html");
            findExtention = ".html";
        }
        responseCode = 200;
    } else {
        if ((findExtention = strrchr(startDir, '.')) == NULL) {
            perror("invalid webpage");

            findExtention = ".html";

            if (strlen(servFileDir) >= fileDirLen) {
                printf("\n\nbuffer overflow warning, please change the size of fileDirLen or make the size of servFileDir shorter\n\n");

                exit(1);
            }

            strcpy(fileDir, servFileDir);
            strcat(fileDir, "err404.html");
            responseCode = 404;
        }
    }

    //tries to open directory to check if it exists
    if ((fpointer = fopen(fileDir, "rb")) != NULL) {
        fseek(fpointer, 0L, SEEK_END);
        fsize = ftell(fpointer);
    } else if (strcmp(startDir-1, "/favicon.ico") == 0 && access(fileDir, F_OK) != 0) {
        if (strlen(servFileDir) >= fileDirLen) {
            printf("\n\nbuffer overflow warning, please change the size of fileDirLen or make the size of servFileDir shorter\n\n");

            exit(1);
        }

        strcpy(fileDir, servFileDir);
        strcat(fileDir, "favicon.ico");

        if (access(fileDir, F_OK) != 0) {
            printf("\n\n\nERROR: please do not delete the default favicon.ico file. the program will not work properly if it is deleted\n\n\n");

            //error 500: internal server error
            send(acceptSock, "HTTP/1.0 500\r\n\r\n", 16, MSG_NOSIGNAL);

            exit(1);
        }
        
        if ((fpointer = fopen(fileDir, "rb")) == NULL) {
            perror("fopen error1");

            //error 500: internal server error
            send(acceptSock, "HTTP/1.0 500\r\n\r\n", 16, MSG_NOSIGNAL);

            *fullResp = '\0';
            
            fclose(fpointer);
            close(acceptSock);
            return 0;
        }

        fseek(fpointer, 0L, SEEK_END);
        fsize = ftell(fpointer);
    } else if (access(fileDir, F_OK) != 0) {
        perror("webpage doesnt exist");
        findExtention = ".html";
        

        if (strlen(servFileDir) >= fileDirLen) {
            printf("\n\nbuffer overflow warning, please change the size of fileDirLen or make the size of servFileDir shorter\n\n");

            exit(1);
        }

        strcpy(fileDir, servFileDir);
        strcat(fileDir, "err404.html");
        responseCode = 404;
        if ((fpointer = fopen(fileDir, "rb")) == NULL) {
            perror("fopen error2");

            //error 500: internal server error
            send(acceptSock, "HTTP/1.0 500\r\n\r\n", 16, MSG_NOSIGNAL);

            *fullResp = '\0';
            
            fclose(fpointer);
            close(acceptSock);
            return 0;
        }
        fseek(fpointer, 0L, SEEK_END);
        fsize = ftell(fpointer);
    }


    fclose(fpointer);



    //sets the server http response
    if ((strcmp(findExtention, ".jpeg")) == 0 || (strcmp(findExtention, ".jpg")) == 0) {
        bin = 1;
        if (sprintf(httpResponse, "HTTP/1.0 %d\r\nConnection: close\r\nContent-Type: image/jpeg\r\nContent-Length: %d\r\nContent-Transfer-Encoding: binary\r\n\r\n", responseCode, fsize) >= httpResponseSize) {
            //perror("too big of a response");

            *fullResp = '\0';

            close(acceptSock);
            return 0;
        }
    } else if ((strcmp(findExtention, ".png")) == 0) {
        bin = 1;
        if (sprintf(httpResponse, "HTTP/1.0 %d\r\nConnection: close\r\nContent-Type: image/png\r\nContent-Length: %d\r\nContent-Transfer-Encoding: binary\r\n\r\n", responseCode, fsize) >= httpResponseSize) {
            //perror("too big of a response");

            *fullResp = '\0';

            close(acceptSock);
            return 0;
        }
    } else if ((strcmp(findExtention, ".ico")) == 0) {
        bin = 1;
        if (sprintf(httpResponse, "HTTP/1.0 %d\r\nConnection: close\r\nContent-Type: image/x-icon\r\nContent-Length: %d\r\nContent-Transfer-Encoding: binary\r\n\r\n", responseCode, fsize) >= httpResponseSize) {
            //perror("too big of a response");

            *fullResp = '\0';

            close(acceptSock);
            return 0;
        }
    } else if ((strcmp(findExtention, ".mp3")) == 0) {
        bin = 1;
        if (sprintf(httpResponse, "HTTP/1.0 %d\r\nConnection: close\r\nContent-Type: audio/mpeg\r\nContent-length: %d\r\nContent-Transfer-Encoding: binary\r\n\r\n", responseCode, fsize) >= httpResponseSize) {
            //perror("too big of a response");

            *fullResp = '\0';

            close(acceptSock);
            return 0;
        }
    } else if ((strcmp(findExtention, ".html")) == 0) {
        bin = 0;
        if (sprintf(httpResponse, "HTTP/1.0 %d\r\nConnection: close\r\nContent-Type: text/html\r\nContent-Length: %d\r\n\r\n", responseCode, fsize) >= httpResponseSize) {
            //perror("too big of a response");

            *fullResp = '\0';

            close(acceptSock);
            return 0;
        }
    } else if ((strcmp(findExtention, ".css")) == 0) {
        bin = 0;
        if (sprintf(httpResponse, "HTTP/1.0 %d\r\nConnection: close\r\nContent-Type: text/css\r\nContent-Length: %d\r\n\r\n", responseCode, fsize) >= httpResponseSize) {
            //perror("too big of a response");

            *fullResp = '\0';

            close(acceptSock);
            return 0;
        }
    } else if ((strcmp(findExtention, ".js")) == 0) {
    	bin = 0;
    	if (sprintf(httpResponse, "HTTP/1.0 %d/r/nConnection: close\r\nContent-Type: text/javascript\r\nContent-Length: %d\r\n\r\n", responseCode, fsize) >= httpResponseSize) {
    		//perror("too big of a response");
    		
    		*fullResp = '\0';
    		
    		close(acceptSock);
    		return 0;
    	}
    } else {
        if ((fpointer = fopen(fileDir, "r")) == NULL) {
            strcat(fileDir, "err404.html");
            
            fseek(fpointer, 0L, SEEK_END);
        	fsize = ftell(fpointer);
        	fclose(fpointer);
        	bin = 0;
        	if (sprintf(httpResponse, "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nContent-Length: %d\r\n\r\n", fsize) >= httpResponseSize) {
            	//perror("too big of a response");

            	*fullResp = '\0';

            	close(acceptSock);
            	return 0;
        	}
        
            perror("fopen error3");

            *fullResp = '\0';

            close(acceptSock);
            return 0;
        }
        
        bin = 0;
        if (sprintf(httpResponse, "HTTP/1.0 %d/r/nContent-Type: text\r\nContent-Length: %d\r\n\r\n", responseCode, fsize) >= httpResponseSize) {
        	//perror("too big of a response");
        	
        	*fullResp = '\0';
        	
        	close(acceptSock);
        	return 0;
        }
    }

    strcpy(httpResponseReport, httpResponse);



    //check for rsql
    //int testrsql = findRSQL(fileDir);
    //printf("testrsql: %d\n", testrsql);



    if (readSock < 0) {

        //error 500: internal server error
        send(acceptSock, "HTTP/1.0 500\r\n\r\n", 16, MSG_NOSIGNAL);

        *fullResp = '\0';
        *fileDir = '\0';
        *httpResponseReport = '\0';
        *httpResponse = '\0';

        close(acceptSock);
        return 0;
    }


    //checks if i need to read plaintext or binary
    if (bin == 0) {
        fpointer = fopen(fileDir, "r");
    } else if (bin == 1) {
        fpointer = fopen(fileDir, "rb");
    }


    if (fpointer == NULL) {
        perror("file open error");

        //error 500: internal server error
        send(acceptSock, "HTTP/1.0 500\r\n\r\n", 16, MSG_NOSIGNAL);

        *fullResp = '\0';
        *fileDir = '\0';
        *httpResponseReport = '\0';
        *httpResponse = '\0';

        fclose(fpointer);
        close(acceptSock);
        return 0;
    }

    



    //sends server http response to client
    fseek(fpointer, 0L, SEEK_END);
    fsize = ftell(fpointer);
    rewind(fpointer);




    if (fpointer == NULL) {
        perror("fopen error");

        //error 500: internal server error
        send(acceptSock, "HTTP/1.0 500\r\n\r\n", 16, MSG_NOSIGNAL);

        *fullResp = '\0';
        *fileDir = '\0';
        *httpResponse = '\0';
        *httpResponseReport = '\0';

        fclose(fpointer);
        close(acceptSock);
        return 0;
    }

    fileLine = malloc(fsize * sizeof(char));

    

    freadErr = fread(fileLine, fsize, 1, fpointer);
    
    
    if (freadErr != 1) {
        perror("fread error");

        //error 500: internal server error
        send(acceptSock, "HTTP/1.0 500\r\n\r\n", 16, MSG_NOSIGNAL);

        *fullResp = '\0';
        *fileDir = '\0';
        *httpResponseReport = '\0';
        *httpResponse = '\0';

        fclose(fpointer);
        close(acceptSock);
        return 0;
    }




    //sends response
    int httpLen = strlen(httpResponse);
    //usually i would use write but send includes some error handlers like MSG_NOSIGNAL which helps ignore quit signals sent to the kernel like "err: broken pipe" which was VERY ANNOYING (right, past Rowan?)
    if ((checkSend = send(acceptSock, httpResponse, httpLen, MSG_NOSIGNAL)) != httpLen) {
        perror("httpResponse error");

        *fullResp = '\0';
        *fileDir = '\0';
        *httpResponseReport = '\0';
        *httpResponse = '\0';

        fclose(fpointer);
        close(acceptSock);
        return 0;
    }

    //send full response
    if ((checkSend = send(acceptSock, fileLine, fsize, MSG_NOSIGNAL)) != fsize) {
        perror("checkSend error");

        *fullResp = '\0';
        *fileDir = '\0';
        *httpResponseReport = '\0';
        *httpResponse = '\0';
        
        fclose(fpointer);
        close(acceptSock);
        return 0;
    }

    

    *httpResponse = '\0';
    *fullResp = '\0';


    fclose(fpointer);
    close(acceptSock);
    free(fileLine);



    *fileDir = '\0';
    *httpResponseReport = '\0';
    return 0;
}



//assign work to each thread
void *giveThreadWork() {
    //allocate memory onto the heap to store the client request and so I can add more memory later (buffer overflow problems)
    char *respBuffer = malloc(fullRespHeapSize*sizeof(char));
    if (respBuffer == NULL) {
        perror("giveThreadWork malloc");
    }
    while (1) {
        int pclient;

        if (pthread_mutex_lock(&mutex) != 0) {
            printf("something very wrong has happened with pthread_mutex_lock in giveThreadWork()\n");
            exit(1);
        }

        //makes thread wait until signaled
        while ((pclient = dequeue()) == -1) {
            if (pthread_cond_wait(&condition_var, &mutex) != 0) {
                printf("something very wrong has happened with pthread_cont_wait in giveThreadWork()\n");
                exit(1);
            }
        }
        if (pthread_mutex_unlock(&mutex) != 0) {
            printf("something very wrong has happened with pthread_mutex_unlock in giveThreadWork()\n");
            exit(1);
        }
        
        handleClient(pclient, respBuffer);
    }
}


int main() {
    int clientSock;
    int sock;
    int portNum;
    int defaultOrNo;
    socklen_t clientLenMain;


    printf("\n\n\nWeb Server\nBy: Rowan Rothe\n\n\n");

    printf("would you like to use default directories (1 = yes, 0 = no): ");
    scanf("%d", &defaultOrNo);

    if (defaultOrNo == 0) {
        printf("enter the directory of the siteDirs.txt file (with '/' at the end): ");
        scanf("%s", webFileDir);

        printf("enter the directory of the web server folder (with '/' at the end): ");
        scanf("%s", servFileDir);
    } else if (defaultOrNo == 1) {
        strcpy(webFileDir, defaultDir1);
        strcpy(servFileDir, defaultDir2);
    }

    printf("what port would you like to host the site on?: ");
    scanf("%d", &portNum);

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0) {
        perror("sock error");
        exit(1);
    }



    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    sockaddr.sin_port = htons(portNum);

    if ((bind(sock, (struct sockaddr *) &sockaddr, sizeof(sockaddr))) < 0) {
        perror("bind error");
        exit(1);
    }

    printf("socket bind success\n");

    //create all the threads for the thread pool
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        if (pthread_create(&thread_pool[i], NULL, giveThreadWork, NULL) != 0) {
            printf("something very wrong has happened with pthread_create in main()\n");
            exit(1);
        }
    }

    printf("created thread pool of size %d successfully\n", THREAD_POOL_SIZE);

    if ((listen(sock, BACKLOG)) < 0) {
        perror("listen error");
        exit(1);
    }

    printf("listen success\n");




    while (1) {
        //checks for client connection
        clientLenMain = sizeof(clientaddr);
        if ((clientSock = accept(sock, (struct sockaddr *)&clientaddr, &clientLenMain)) < 0) {
            perror("accept error");
            close(clientSock);
        } else {
            if (pthread_mutex_lock(&mutex) != 0) {
                printf("something very wrong has happened with pthread_mutex_lock in main()\n");
                exit(1);
            }
            enqueue(clientSock);
            //sends a signal to wait until needed
            if (pthread_cond_signal(&condition_var) != 0) {
                printf("something very wrong has happened with pthread_cond_signal in main()\n");
                exit(1);
            }

            if (pthread_mutex_unlock(&mutex) != 0) {
                printf("something very wrong has happened with pthread_mutex_unlock in main()\n");
                exit(1);
            }
        }
    }

    return 0;
}



