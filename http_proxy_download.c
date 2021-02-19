/* f20170842@hyderabad.bits-pilani.ac.in VISHNU VARDHAN REDDY IREDDY */

/* 
    This is a programs which uses sockets to get the data from web something like a web-scraper. 
    First, it creates a socket and connects with the Proxy server and then uses send() function
    to send the HTTP requests for the data to be retrieved.
    recv() function gets the response from the server and puts it on a string.
    Then we are writing the response to the files excluding the HTTP response header.
    If there are any redirects to http sites, then that is taken care by 'Location: '.
    source for b64encoder: github handle of  'elzoughby'
*/



#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<netinet/in.h>

char img_src[1024], *res;
int flag = 0;

char* base64encoder(char* text){
    res = (char *)malloc(strlen(text)*4/3+4);
    char* search_string = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    char pads = 0, str_buf[3];
    
    int iterator = 0;

    for(int i = 0;text[i] != '\0';i++){
        str_buf[pads++] = text[i];
        if(pads == 3){
            res[iterator] = search_string[str_buf[0]>>2];
            iterator++;
            res[iterator] = search_string[((str_buf[0]&0x03)<<4) + (str_buf[1]>>4)];
            iterator++;
            res[iterator] = search_string[((str_buf[1]&0x0f)<<2) + (str_buf[2]>>6)];
            iterator++;
            res[iterator] = search_string[str_buf[2]&0x3f];
            iterator++;
            pads = 0;
        }
    }

    if(pads>0){
        res[iterator] = search_string[str_buf[0]>>2];
        iterator++;
        if(pads == 1){
            res[iterator] = search_string[(str_buf[0]&0x03)<<4];
            iterator++;
            res[iterator] = '=';
            iterator++;
        }else{                   
            res[iterator] = search_string[((str_buf[0]&0x03)<<4) + (str_buf[1]>>4)];
            iterator++;
            res[iterator] = search_string[(str_buf[1]&0x0f)<<2];
            iterator++;
        }
        res[iterator] = '=';
        iterator++;
    }

    res[iterator] = '\0';  
    return res;
}

int get_len(char* ch, char* res){
    int sz = 0;
    while(ch != (res+sz))
        sz++;

    return sz;
}



void get_file(char* img_src, char* hourl, char* url, char* encreds, int pport, char* pip, char* file){
    if(!flag)
        img_src[0]='\0';
    
    char* msg = "GET http://%s/%s HTTP/1.1\r\nHost: %s\r\nAccept: */*\r\nConnection: close\r\nProxy-Authorization: Basic %s\r\n\r\n";
    char conn_msg[3*1024];
    sprintf(conn_msg, msg, url, img_src, hourl, encreds);

    int sockfd;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd<0){
        printf("Socket initiation failed!!");
        return;
    }
    printf("\n-->  Socket intiation successful\n");

    struct sockaddr_in srvr_addr;
    srvr_addr.sin_family = AF_INET;
    srvr_addr.sin_port = htons(pport);
    srvr_addr.sin_addr.s_addr = inet_addr(pip);

    int ssd = connect(sockfd, (struct sockaddr *)&srvr_addr, sizeof(srvr_addr));
    if(ssd<0){
        printf("error while connecting to server!!");
        return;
    }
    printf("\n-->  Connected to proxy server\n");

    char *srvr_res;
    srvr_res = (char *) malloc(32*1024);
    send(sockfd, conn_msg, strlen(conn_msg),0);
    printf("\n-->  Sending HTTP request to %s\n", url);
    int bytes_read;
    FILE *fptr;
    fptr = fopen(file, "wb");

    do{
        bytes_read = recv(sockfd, srvr_res, 32*1024, 0);
        if(bytes_read == 0)
            break;
        if(bytes_read == -1){
            printf("recv error!");
            return;
        }
        else{
            
            char loc[] = "Location: ";
            char* loch = strstr(srvr_res, loc);
            int ls = strlen(loc);
            

            char temp[] = "Connection: close\r\n\r\n";
            char* ch = strstr(srvr_res, temp);
            int ts = strlen(temp);


            if(loch != NULL && loch < ch){
                char* wch = strstr(loch, "://");
                if(wch == NULL){
                    char* sl = strstr(loch, "/");
                    char resurl[strlen(hourl)+strlen(sl)];
                    strcat(resurl, hourl);
                    strcat(resurl, sl);
                    close(sockfd);
                    fclose(fptr);
                    printf("\n-->  Closing the socket and redirecting to %s\n", resurl);
                    get_file(img_src, hourl, resurl, encreds, pport, pip, file);
                }
                wch += 3;
                char* nl = strstr(wch, "\n");
                char* com = strstr(wch, "/");
                int sz = (int)(strlen(wch)-strlen(nl));
                int hsz = (int)(strlen(wch)-strlen(com));
                if(*(nl-2) == '/')
                    *(nl-2) = 0;
                char reurl[sz], hurl[hsz];
                sprintf(reurl,"%.*s", sz, wch);
                sprintf(hurl, "%.*s", hsz, wch);
                reurl[sz-1]=0;

                close(sockfd);
                fclose(fptr);
                printf("\n-->  Closing the socket and redirecting to %s\n", reurl);
                get_file(img_src, hurl, reurl, encreds, pport, pip, file);
                return;
            }

            
            if(ch != NULL){
                int ch_len = get_len(ch+ts, srvr_res);
                fwrite(ch+ts, bytes_read - ch_len, 1, fptr);
            }else
                fwrite(srvr_res, bytes_read, 1, fptr);


            if(!flag && !strcmp(url, "info.in2p3.fr")){
                char temp1[] = "IMG SRC=\"";
                ch = strstr(srvr_res, temp1);
                if(ch != NULL){
                    flag = 1;
                    int it = strlen(temp1), jt = 0;
                    while(*(ch+it) != '\"'){
                        img_src[jt] = *(ch+it);
                        it++;
                        jt++;
                    }
                    img_src[jt]='\0';
                }
            }
            
        }
    }while(bytes_read > 0);
    printf("\n-->  Received data and written to file successfully\n");

    printf("\n-->  Closing the socket\n");
    close(sockfd);
    fclose(fptr);
    return;
}

int main(int argc, char *argv[]){
    
    if(argc != 8){
        printf("Invalid number of arguments!!\nCorrect order:: ./<BIN_O/P_FILE> <URL> <PROXY_IP> <PROXY_PORT> <LOGIN_ID> <PASSWORD> <HTML_FILE_NAME> <LOGO_FILE_NAME>");
        return 1;
    }
    
    char *url = argv[1], *pip = argv[2], *lid = argv[4], *pswrd = argv[5], *htmlf = argv[6], *logo = argv[7];
    int pport = atoi(argv[3]);
    
    char* creds_for = "%s:%s";
    char creds[1024];
    sprintf(creds, creds_for, lid, pswrd);
    int clen = strlen(creds);
    char* encreds = base64encoder(creds);

    char* hc = strstr(url, "/");
    int hl = 0;
    char* hurl;
    if(hc != NULL){
        hl = (int)(strlen(url)-strlen(hc));
        hurl = (char *) malloc(hl);
        sprintf(hurl, "%.*s", hl, url);
    }else{
        hurl = (char *) malloc(strlen(url));
        strcpy(hurl, url);
    }
    
    get_file(img_src, hurl, url, encreds, pport, pip, htmlf);

    if(!strcmp(url, "info.in2p3.fr")){
        printf("\n-->  Downloading logo image\n");
        get_file(img_src, hurl, url, encreds, pport, pip, logo);
        printf("\n-->  Image downloaded successfully\n");
    }
    return 0;
}