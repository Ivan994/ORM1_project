/* 
    ********************************************************************
    Odsek:          Elektrotehnika i racunarstvo
    Departman:      Racunarstvo i automatika
    Katedra:        Racunarska tehnika i racunarske komunikacije (RT-RK)
    Predmet:        Osnovi Racunarskih Mreza 1
    Godina studija: Treca (III)
    Skolska godina: 2017/2018
    Semestar:       Zimski (V)
    Ime fajla:      client.c
    Opis:           TCP/IP klijent - Elektronski registar vozila
    Platforma:      Raspberry Pi 2 - Model B
    OS:             Raspbian
	Izrada: 		Aleksa Arsic (RA119/2015)
					Ivan Mitrovic (RA39/2013)
    ********************************************************************
*/

#include <stdlib.h>
#include<stdio.h>      //printf
#include<string.h>     //strlen
#include<sys/socket.h> //socket
#include<arpa/inet.h>  //inet_addr
#include <fcntl.h>     //for open
#include <unistd.h>    //for close

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT   27015
#define welcome_message "Welcome to Ivan, Aleksa & Sinovi rent-a-car"
#define LOGIN "Login Succesfull."
#define LOGIN_e "User data incorrect. Try another combination of username and password." 
#define REGISTER "Register Succesfull."
#define REGISTER_e "User with same username already exists." 
#define SIGN_out "Signed out."
#define DEFAULT_BUFLEN_C 1024
#define BUFLEN 16
#define CAR_ID_LEN 8
#define YEAR 4

int start_menu();
int login_menu();
int register_menu();
int logged_menu();
void search_menu(char id[], char man[], char cn[], char year[]);

int main(int argc , char *argv[]){

    int sock, read_size;
    struct sockaddr_in server, client;
    char message[1];
	int option = 0;
	char username[DEFAULT_BUFLEN];
	char password[DEFAULT_BUFLEN];
	char *us_pass;
	int port = 20716;
	char log_opt[3] = "00\0";
	char cars[DEFAULT_BUFLEN];
	char recv_car[DEFAULT_BUFLEN];
	char reserve_id[CAR_ID_LEN], reserved[DEFAULT_BUFLEN];
	char *search;
	char id[BUFLEN], manufacturer[DEFAULT_BUFLEN];
	char carname[DEFAULT_BUFLEN], year[BUFLEN]; 
	char search_r[DEFAULT_BUFLEN_C];

	//clean memory blocks
	memset(message, '\0', 1);
	memset(username, '\0', DEFAULT_BUFLEN);
	memset(password, '\0', DEFAULT_BUFLEN);
	memset(cars, '\0', DEFAULT_BUFLEN);

    //Create socket

    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1)
    {
        printf("Could not create socket");

    }

    puts("Socket created");

    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons(DEFAULT_PORT);

    //Connect to remote server

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("connect failed. Error");
        return 1;
    }

    puts("Connected\n");

    // Create socket to receive data
    int socket_desc , client_sock;
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1) {
        puts("Could not create socket");
    }

    // Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);

    // Bind
    if (bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0) {
        perror("bind failed. Error");
        return 1;
    }

    // Send port to server
    if (send(sock, &port, sizeof(int), 0) < 0) {
        perror("send failed. Error");
        return 1;
    }

    // Listen
    listen(socket_desc, 3);

    // Accept connection from server
    int c = sizeof(struct sockaddr_in);
    client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);

    if (client_sock < 0) {
        perror("accept failed. Error");
        return 1;
    }

	printf("%s\n", welcome_message);

	//Login or Register
	do{
		option = start_menu();
		if(option < 1 || option > 2) puts("Error. Choose again.");	
	}while(option < 1 || option > 2);

	message[0] = option + '0';
	message[1] = '\0';

	//Send start menu option
	if(send(sock, message, strlen(message), 0) < 0) {
	    puts("Send failed");
	    return 1;
	}

	// login screen with while not logged in
	while(option == 1){
		login_menu(username, password);
		//making one string from username and password
		//format for sending username|password
		//corresponding messages from server
		us_pass = (char *)malloc(strlen(username)+strlen(password)+2);
		strcat(us_pass, username);
		strcat(us_pass, "|");
		strcat(us_pass, password);

		if(send(sock, us_pass, strlen(us_pass), 0) < 0) {
			puts("Send failed");
			return 1;
		}

		// feedback from server about login
		// if login is succesfull server will send 1L and if not 0L
		if((read_size = recv(client_sock, log_opt, 3, 0)) == -1) {
			puts("Error 333");
		}
		if(!strcmp(log_opt, "0L\0")){
			puts(LOGIN_e);
			free(us_pass);
		}else if(!strcmp(log_opt, "1L\0")){
			puts(LOGIN);
			break;
		}else{
			puts("error");
			free(us_pass);
			close(sock);
		}
	}

	//Register screen with while not registered
	while(option == 2){
		register_menu(username, password);
		us_pass = (char *)malloc(strlen(username)+strlen(password)+2);
		strcat(us_pass, username);
		strcat(us_pass, "|");
		strcat(us_pass, password);

		if(send(sock, us_pass, strlen(us_pass), 0) < 0) {
			puts("Send failed");
			return 1;
		}

		// feedback from server about register
		// if login succesfull server will send 1R and if not 0R
		if((read_size = recv(client_sock, log_opt, 3, 0)) == -1) {
			puts("Error 333");
		}
		if(!strcmp(log_opt, "0R\0")){
			puts(REGISTER_e);
			free(us_pass);
		}else if(!strcmp(log_opt, "1R\0")){
			puts(REGISTER);
			break;
		}else{
			puts("error");
			free(us_pass);
			close(sock);
		}
	}

	// when logged in choose what to do next
	do{	
		// asking user what will he do when logged in (or registered)
		option = logged_menu(username);
		memset(message, '\0', 1);
		message[0] = option + '0';
		message[1] = '\0';

		if(send(sock, message, strlen(message), 0) < 0) {
			puts("Send failed");
			return 1;
		}

		// Switching between what user choose as option 
		// 1 : SEARCH all
		// 2 : SEARCH with parameters
		// 3 : Checking status of reserved vehicles by logged user
		// 4 : Reserve vehicle by vehicle ID
		// 5 : Log out/Disconnect from server
		// default : error
		switch(option){
			case 1:
				do{
					if((read_size = recv(client_sock, cars, DEFAULT_BUFLEN_C, 0)) == -1) {
							puts("Error 334");
						}
						puts(cars);
						memset(cars, '\0', DEFAULT_BUFLEN);							
				}while(!strcmp(cars, "EOF\0"));
				break;
			case 2:
				do{
					search_menu(id, manufacturer, carname, year);
					if((strlen(id) >  CAR_ID_LEN) || (strlen(year) > YEAR))
						puts("Error. Some of the parameters are not vaild.");
				}while((strlen(id) >  CAR_ID_LEN) || (strlen(year) > YEAR));

				int s_len = strlen(id)+strlen(manufacturer)+strlen(carname)+strlen(year) + 5;
				search = (char *)malloc(s_len);

				memset(search, '\0', s_len);
				strcat(search, id);
				strcat(search, "|");
				strcat(search, manufacturer);
				strcat(search, "|");
				strcat(search, carname);
				strcat(search, "|");
				strcat(search, year);
				strcat(search, "\0");


				if(send(sock, search, strlen(search), 0) < 0) {
					puts("Send failed");
					return 1;
				}	

				memset(search_r, '\0', DEFAULT_BUFLEN);
				if((read_size = recv(client_sock, search_r, DEFAULT_BUFLEN, 0)) == -1) {
					puts("Error 334");
				}
				puts(search_r);

				memset(search, '\0', s_len);
				free(search);

				break;

			case 3:
				memset(recv_car, '\0', DEFAULT_BUFLEN);

				if((read_size = recv(client_sock, recv_car, DEFAULT_BUFLEN_C, 0)) == -1) {
					puts("Error 334");
				}

				puts("Reserved vehicles: ");
				puts(recv_car);

				break;
			case 4:
				memset(reserve_id, '\0', CAR_ID_LEN);

				do{
					printf("Reserve car [ID]: ");
					scanf("%s", reserve_id);
					if(strlen(reserve_id) != 8) puts("Vehicle ID must have 8-digits.");
				}while(strlen(reserve_id) != 8);

				if(send(sock, reserve_id, CAR_ID_LEN, 0) < 0) {
					puts("Send failed");
					return 1;
				}
				memset(reserved, '\0', DEFAULT_BUFLEN);
				if((read_size = recv(client_sock, reserved, DEFAULT_BUFLEN, 0)) == -1) {
					puts("Error 335");
				}
				puts(reserved);

				break;
			case 5:
				puts("**************************************");	
				printf("Thanks for visiting us!\nLogged off.\n");
				puts("**************************************");	
				if(send(sock, username, strlen(username), 0) < 0) {
					puts("Send failed");
					return 1;
				}
				if(send(sock, SIGN_out, strlen(SIGN_out), 0) < 0) {
					puts("Send failed");
					return 1;
				}

				free(us_pass);
				free(search);
				close(sock);			
				break;
			default:
				puts("Error. Choose again.");
				break;
		}

	}while(option != 5);

    return 0;

}



int start_menu(){

	int retVal = 0;

	puts("**************************************");
	puts("1. Login");
	puts("2. Register");
	puts("**************************************");	
	printf("Choose option: ");
	scanf("%d", &retVal);

	return retVal;

}



int login_menu(char username[], char password[]){

	puts("**************************************");
	printf("Username: ");
	scanf("%s", username);	
	printf("Password: ");
	scanf("%s", password);

	return 1;

}



int register_menu(char username[], char password[]){

	puts("**************************************");
	printf("Enter new username: ");
	scanf("%s", username);
	printf("Enter new password: ");
	scanf("%s", password);
	return 1;	

}



int logged_menu(const char username[]){

	int retVal = 0;

	puts("**************************************");
	printf("You are logged in as %s\n", username);
	puts("1. Search all");
	puts("2. Search [ID][MANUFACTURER][CARNAME][YEAR]");
	puts("3. Check status of reserved vehicles");
	puts("4. Reserve vehicle");
	puts("5. Logout");
	puts("**************************************");	
	printf("Choose option: ");
	scanf("%d", &retVal);

	return retVal;

}


// Search with parameters menu
void search_menu(char id[], char man[], char cn[], char year[]){

	puts("Search by parameters:");
	printf("Vehicle ID: ");
	scanf("%s", id);
	printf("Manufacturer: ");
	scanf("%s", man);
	printf("Car name: ");
	scanf("%s", cn);
	printf("Year: ");
	scanf("%s", year);

	year[4] = '\0';

}
