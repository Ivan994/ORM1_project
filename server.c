/* 
    ********************************************************************
    Odsek:          Elektrotehnika i racunarstvo
    Departman:      Racunarstvo i automatika
    Katedra:        Racunarska tehnika i racunarske komunikacije (RT-RK)
    Predmet:        Osnovi Racunarskih Mreza 1
    Godina studija: Treca (III)
    Skolska godina: 2017/2018
    Semestar:       Zimski (V)
    Ime fajla:      server.c
    Opis:           TCP/IP klijent - Elektronski registar vozila
    Platforma:      Raspberry Pi 2 - Model B
    OS:             Raspbian
	Izrada: 		Aleksa Arsic (RA119/2015)
					Ivan Mitrovic (RA39/2013)
    ********************************************************************
*/

#include <stdlib.h>
#include<stdio.h>
#include<string.h>    //strlen
#include<sys/socket.h>
#include<arpa/inet.h> //inet_addr
#include<unistd.h>    //write

// defines
#define DEFAULT_BUFLEN 512
#define DEFAULT_BUFLEN_P 128
#define DEFAULT_PORT   27015
#define ID_LEN 4
#define LOGIN "1L"
#define LOGIN_e "0L"
#define REGISTER "1R"
#define REGISTER_e "0R"
#define RESERVED "User doesn't have any vehicles registered under his name"
#define CAR_ID_LEN 8
#define V_RSVD "Vehicle reserved."
#define V_ALREADY_RSVD "Vehicle already reserved."
#define YEAR 4
#define BUFLEN 16
#define SEARCH_e "No results found."

// super_user login information
char s_user[DEFAULT_BUFLEN] = "admin";
char s_user_pass[DEFAULT_BUFLEN] = "0000";
// global variables
char reserved[DEFAULT_BUFLEN];
char f_id[CAR_ID_LEN], f_manufacturer[BUFLEN];
char f_carname[BUFLEN], f_year[YEAR];

char id[CAR_ID_LEN], manufacturer[BUFLEN];
char carname[BUFLEN], year[YEAR];

// functions
int check_base_for_user(const char username[], const char password[]);
int give_user_pass(const char us_pass[], char username[], char password[]);
int register_new_user(char temp_user[], char temp_password[]);
void get_id(const char buffer[], int *id);
void make_us_pass(char us_pass[], const char username[], const char password[]);
int search_all(const int sock);
void check_reserved(const char username[], const int sock);
int give_us_car(const char us_car[], const char username[], char user_name[]);
int parse_car_id(const char cars[], char car[]);
void reserve_vehicle(const char username[], char car_id[], const int c_sock, const int sock);
int parse_reserved(const char file_line[], char id[], char user_name[]);
void make_us_car(char us_id[], const char user_name[], const char id[]);
void search(const int c_sock, const int sock, char parameters[]);
void get_f_line_params(const char f_line[]);
void get_line_params(const char params[]);
int cmp_y(char f_year[], char year[]);
int params_no(const char params[]);

int main(int argc , char *argv[])
{
	int sock, client_port;
    int socket_desc , client_sock , c , read_size;
    struct sockaddr_in server , client, server_c;
    char cm_start_menu_opt[DEFAULT_BUFLEN];
	char sw_start_opt = '0'; 
	char us_pass[DEFAULT_BUFLEN];
	char username[DEFAULT_BUFLEN], password[DEFAULT_BUFLEN];
	char logged_opt[1];
	int l_option, logged, registered;
	char car_id[CAR_ID_LEN];
	char parameters[DEFAULT_BUFLEN_P];

    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");

    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(DEFAULT_PORT);

    //Bind
    if(bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        perror("bind failed. Error");
        return 1;
    }
    puts("bind done");

	//Listen
	listen(socket_desc , 3);

	while(1){

		// Clean memory blocks - client_messages
		memset(cm_start_menu_opt, '\0', DEFAULT_BUFLEN);
		memset(us_pass, '\0', DEFAULT_BUFLEN);
		memset(logged_opt, '\0', 1);

		//Accept and incoming connection
		puts("Waiting for incoming connections...");
		c = sizeof(struct sockaddr_in);

		//accept connection from an incoming client
		client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
		if (client_sock < 0) {
			perror("accept failed");
			return 1;
		}
		puts("Connection accepted");

		//CONNECTING TO client TO SEND DATA
		//Create socket
		sock = socket(AF_INET , SOCK_STREAM , 0);
		if (sock == -1)
		{
		   close(socket_desc);
		   printf("Could not create socket");
		}
		puts("Socket created");

		if (recv(client_sock, &client_port, sizeof(int), 0) == -1)
		{
		    perror("recv failed. Error");
		    return 1;
		}

		server_c.sin_addr.s_addr = client.sin_addr.s_addr;
		server_c.sin_family = AF_INET;
		server_c.sin_port = htons(client_port);

		//Connect to remote client
		if (connect(sock, (struct sockaddr *)&server_c, sizeof(server_c)) < 0)
		{
		    perror("connect failed. Error");
		    return 1;
		}

		puts("Connected to client\n");

		//Receive start menu message from client
		if((read_size = recv(client_sock, cm_start_menu_opt, DEFAULT_BUFLEN, 0)) == -1) {
			puts("Error geting start menu option from client.");
		}else{
			sw_start_opt = cm_start_menu_opt[0];
			memset(cm_start_menu_opt, '\0', DEFAULT_BUFLEN);
			if(sw_start_opt == '1'){
				puts("Login.");
			}else if(sw_start_opt == '2'){
				puts("Try Register under:");
			}else{
				puts("Error getting valid start menu option.");
			}
		}

		// while not logged in or registered ask client for information
		while((logged == 0) || (registered == 0)){
			logged = 0;
			registered = 0;
			memset(username, '\0', DEFAULT_BUFLEN);
			memset(password, '\0', DEFAULT_BUFLEN);
			memset(us_pass, '\0', DEFAULT_BUFLEN);
			//Receive username and password client sent to server
			if((read_size = recv(client_sock, us_pass, DEFAULT_BUFLEN, 0)) == -1) {
				puts("Error geting username|password from client.");
			}else{
				puts(us_pass);	
			}

			if(read_size == 0) {
				puts("Client disconnected");
				fflush(stdout);
			}else if(read_size == -1) {
				perror("recv failed");
			}

			give_user_pass(us_pass, username, password);

			printf("Username: ");
			puts(username);
			printf("Password: ");
			puts(password);

			// Is it login or register?
			// If its login server will run trough the user_pass.txt file and try to find user under the username and password client sent to server. If found server will send to client 1L and if not server will send 0L
			// If its register server will run trough the user_pass.txt file and try to find user under the username and password client sent to server. If found server will send to client 0R and if not server will send 1R
			switch(sw_start_opt){
				case '1':
					if(check_base_for_user(username, password) == 1){
						puts("Login successful.");
						logged = 1;
						if(send(sock, LOGIN, strlen(LOGIN), 0) < 0) {
							puts("Send failed");
							return 1;
						}
					}
					else{
						puts("Username doesn't exist or password might be incorrect.");
						logged = 0;
						if(send(sock, LOGIN_e, strlen(LOGIN_e), 0) < 0) {
							puts("Send failed");
							return 1;
						}
					}
					break;
				case '2':
					if(register_new_user(username, password) == 1){
						puts("Register successful.");
						registered = 1;
						if(send(sock, REGISTER, strlen(REGISTER), 0) < 0) {
							puts("Send failed");
							return 1;
						}
					}
					else{
						puts("Making new super_user or user already exists.");
						registered = 0;
						if(send(sock, REGISTER_e, strlen(REGISTER_e), 0) < 0) {
							puts("Send failed");
							return 1;
						}
					}
					break;
				default:
					puts("Oops! Something went wrong.");
					break;
			}
			if(logged == 1) break;
			if(registered == 1) break;
		}

		do{
			//Reading signed in users options 
			if((read_size = recv(client_sock, logged_opt, sizeof(logged_opt), 0)) == -1) {
				puts("Error geting username|password from client.");
			}		
			l_option = logged_opt[0] - '0';

			printf("From logged menu, user chose: %d\n", l_option);


			// Switching between what user choose as option 
			// 1 : SEARCH all
			// 2 : SEARCH with parameters
			// 3 : Checking status of reserved vehicles by logged user
			// 4 : Reserve vehicle by vehicle ID
			// 5 : Log out/Disconnect from server
			// default : error
			switch(l_option){
				case 1: 
					search_all(sock);
					break;
				case 2:
					search(client_sock, sock, parameters);
					break;
				case 3:
					check_reserved(username, sock);
					break;
				case 4:
					puts("Trying to reserve vehicle with id: ");
					memset(car_id, '\0', CAR_ID_LEN);
					reserve_vehicle(username, car_id, client_sock, sock); 
					break;
				case 5:
					puts("Client disconnected");
					close(client_sock);			
					break;
				default:
					puts("Oops! Choose again."); 
					break;
			}

			if(read_size == 0) {
				puts("Client disconnected");
				fflush(stdout);
			} else if(read_size == -1) {
				perror("recv failed");
			}
		}while(l_option != 5);

	}

    return 0;
}

// this function opens user_pass.txt file and runs trough it trying to find username and password combo 
// if its found return 1 else return -1
int check_base_for_user(const char username[], const char password[]){
	// check super_user
	if((!strcmp(s_user, username)) && (!strcmp(s_user_pass, password))) return 1;
	else{
		char user_f[DEFAULT_BUFLEN], pass_f[DEFAULT_BUFLEN];
		char buffer[DEFAULT_BUFLEN];
		memset(buffer, '\0', DEFAULT_BUFLEN);
		memset(user_f, '\0', DEFAULT_BUFLEN);
		memset(pass_f, '\0', DEFAULT_BUFLEN);
		FILE *fp = fopen("user_pass.txt", "r");

		while (fgets(buffer, sizeof(buffer), fp) != NULL){
		    give_user_pass(buffer, user_f, pass_f);
			if((!strcmp(username, user_f)) && (!strcmp(password, pass_f))) {
				fclose(fp);
				return 1;
			}
		}
		if (feof(fp))
		   puts("\nEnd of file reached. User not in database.");

		fclose(fp);
	}
	return -1;
}

// from username|password combo making new char buffers with separated username
// and password for further use
int give_user_pass(const char us_pass[], char username[], char password[]){
	int i = 0;
	int j = 0;
	int k = 0;
	for(i = 0; i < strlen(us_pass); i++){
		if(us_pass[i] == '|'){
			username[i] = '\0';		
			break;
		}		
		j++;
		username[i] = us_pass[i]; 
	}

	for(i = j + 1; i < strlen(us_pass); i++){
		password[k] = us_pass[i];
		k++;
		if(us_pass[i] == '|'){
            password[k-1] = '\0';
			break;
		}
	}
	
	password[strlen(password)] = '\0';

	return 1;
}

// this function runs trough user_pass.txt file and tries to find username and client sent to server. If found return -1 and error message: User already in database
// else this function will append to user_pass new username|password combo
// also generating new uniqe user ID
int register_new_user(char username[], char password[]){

	if(!strcmp(s_user, username)){
		puts("User is trying to make new super user. Not possible.");
		return -1;
	}
	char user_f[DEFAULT_BUFLEN], pass_f[DEFAULT_BUFLEN];
	char buffer[DEFAULT_BUFLEN];
	int id;
	// id buffers for generating new unique users id
    char n_id[ID_LEN + 1];
    int o_id;
    char l_id[ID_LEN - 1];

    memset(l_id, '\0', ID_LEN - 1);
	memset(buffer, '\0', DEFAULT_BUFLEN);
	memset(user_f, '\0', DEFAULT_BUFLEN);
	memset(pass_f, '\0', DEFAULT_BUFLEN);
	FILE *fp = fopen("user_pass.txt", "r");

	while (fgets(buffer, sizeof(buffer), fp) != NULL){
	    give_user_pass(buffer, user_f, pass_f);
		get_id(buffer, &id);

		if(id < o_id) id = o_id;
		else o_id = id;

		printf("%d\n", id);
		if((!strcmp(username, user_f))) {
			fclose(fp);
			puts("Error. User already in database.");
			return -1;
		}
	}
	if (feof(fp))
	   puts("\nEnd of file reached. User not in database.");

	fclose(fp);

	FILE *fp_w = fopen("user_pass.txt", "a");
	memset(buffer, '\0', DEFAULT_BUFLEN);

	make_us_pass(buffer, username, password);
	
	// making new id  
	o_id = id + 1;   
	sprintf(l_id,"%d", o_id);
  	memset(n_id, '\0', ID_LEN + 1);
    strcat(n_id, "00");
    strcat(n_id, l_id);

	strcat(buffer, "|");
	strcat(buffer, n_id);
	buffer[strlen(buffer)] = '\n';

    fputs(buffer, fp_w);

	puts("\nNew user in database.");
	printf("Username: ");
	puts(username);
	printf("Password: ");
	puts(password);
	printf("User ID: ");
	puts(n_id);

	fclose(fp_w);

	return 1;
}

// parsing user ID from buffer (file_line)
void get_id(const char buffer[], int *id){
    int i;
    int j = 0;
    int k = 0;
    char id_s[ID_LEN];
    for(i = 0; i < strlen(buffer); i++){
        if(buffer[i] == '|'){
            j = i;
        }
    }
    for(i = j + 1; i < strlen(buffer); i++, k++){
        id_s[k] = buffer[i];
    }
    id_s[ID_LEN] = '\0';
    puts(id_s);
    *id = atoi(id_s);
}

// makes username|password combo
void make_us_pass(char us_pass[], const char username[], const char password[]){
    memset(us_pass, '\0', DEFAULT_BUFLEN);
    strcat(us_pass, username);
    strcat(us_pass, "|");
    strcat(us_pass, password);
    puts(us_pass);
}

// search_all lists all vehicles in cars.txt file 
int search_all(const int sock){
	char cars[DEFAULT_BUFLEN];
	FILE *fp = fopen("cars.txt", "r");

	memset(cars, '\0', DEFAULT_BUFLEN);

	while(fgets(cars, sizeof(cars), fp) != NULL){
		printf("%s", cars);
		if(send(sock, cars, strlen(cars), 0) < 0){
			puts("Send failed");
			return 1;
		} 
		memset(cars, '\0', DEFAULT_BUFLEN);
	}
	if(feof(fp)){
		memset(cars, '\0', DEFAULT_BUFLEN);
		strcpy(cars, "EOF\0");
		if(send(sock, cars, strlen(cars), 0) < 0){
			puts("Send failed");
			return 1;
		}
		puts(cars);
	}

	fclose(fp);

	return 1;
}

// this function opens reserved.txt in read mode and tries to find cars reserved under
// specific username by using give_us_car(). After finding one we increment counter. 
// When we get to End Of File, file closes and new matrix is made with as much colums as counter value and rows strlen(CAR_ID)
// car ids are stored in that matrix 
// Now the cars.txt file is opened in read mode and comparing between car ids in file and in matrix starts
// after finding car id in file that is identical to the one in the matrix file_line is 
// sent to client as hat represents the vehicle he reserved 
void check_reserved(const char username[], const int sock){
	char file_get[DEFAULT_BUFLEN];
	char user_name[DEFAULT_BUFLEN];
	char car[CAR_ID_LEN];
	int car_no = 0;
	int i = 0;
	int j = 0;
	int k = 0;
	int cnt = 0;
	int r_cnt = 0;

	memset(file_get, '\0', DEFAULT_BUFLEN);
	memset(reserved, '\0', DEFAULT_BUFLEN);
	memset(car, '\0', CAR_ID_LEN);

	// open reserved.txt and check if logged user 
	// have cars under his username
	FILE *fp = fopen("reserved.txt", "r");

	while(fgets(file_get, sizeof(file_get), fp) != NULL){
		give_us_car(file_get, username, user_name);
	}	
	if(feof(fp)) puts("EOF");

	fclose(fp);

	// count cars in reserved
	for(i = 8; i < strlen(reserved); i += 9){
		if(reserved[i] == '|') car_no++;
	}

	// put reserved cars in matrix for comparing
	char c[car_no][CAR_ID_LEN];

    for(i = 0; i < car_no; i++){
        for(k = 0; k < CAR_ID_LEN; k++){
            if(reserved[j] == '|'){
                j++;
            }
            c[i][k] = reserved[j];
            j++;
        }
    }

	// open cars.txt and parse reserved cars and 
	// send them to client if any reserved under his name
	if(strlen(reserved) != 0){
		FILE *fp_c = fopen("cars.txt", "r");
		printf("Reserved vehicles by %s: \n", username);	

		while(fgets(file_get, sizeof(file_get), fp_c) != NULL){
			parse_car_id(file_get, car);
			
			//comparing individual chars in car and c
			for(i = 0; i < car_no; i++){
				for(k = 0; k < CAR_ID_LEN; k++){
				    if(c[i][k] == car[k]) cnt++;
				    else break;
				}
				if(cnt == CAR_ID_LEN){
					printf("%s", file_get);
					if(send(sock, file_get, strlen(file_get), 0) < 0){
						puts("Send failed");
						return;
					}
					r_cnt++;
				}
				cnt = 0;
			}
		}

		fclose(fp_c);
	}
	else{
		if(send(sock, RESERVED, strlen(RESERVED), 0) < 0){
			puts("Send failed");
			return;
		}
	}

	if(!r_cnt) puts(RESERVED);
}

// from file line give me just car ID
int parse_car_id(const char cars[], char car[]){
	int i;
	
	for(i = 0; i < CAR_ID_LEN; i++){
		car[i] = cars[i];
	}

	return 1;
}

// give username and car ID in separate char buffers and if username and file_line user_name is equal put car ID in gloabl variable reserved
int give_us_car(const char us_car[], const char username[], char user_name[]){
	char car[CAR_ID_LEN];
	int i = 0;
	int j = 0;
	int k = 0;

	for(i = 0; i < strlen(us_car); i++){
		if(us_car[i] == '|'){
			user_name[i] = '\0';		
			break;
		}		
		j++;
		user_name[i] = us_car[i];
	}
	for(i = j + 1; i < strlen(us_car); i++){
		car[k] = us_car[i];
		k++;
		if(us_car[i] == '\n'){
            car[k-1] = '\0';
		}
	}
	car[strlen(car)] = '\0';

	if(!strcmp(username, user_name)){
		strcat(reserved, car);
		strcat(reserved, "|");
	}

	return 1;
}

// reserve_vehicle opens reserved.txt in read mode and tries to find car_id in that file
// if found sends to user error: vehicle is already reserved
// else opens reserved.txt in append mode and adds username|car_id to the file end
// and sends to client that the vehicle is now reserved by user with username
void reserve_vehicle(const char username[], char car_id[], const int c_sock, const int sock){
	int read_size = 0;
	int reserved = 0;
	char file_line[DEFAULT_BUFLEN];
	char id[CAR_ID_LEN], user_name[DEFAULT_BUFLEN];
	char us_id[DEFAULT_BUFLEN];
	FILE *fp = fopen("reserved.txt", "r");
	
	memset(id, '\0', DEFAULT_BUFLEN);
	memset(user_name, '\0', DEFAULT_BUFLEN);
	memset(car_id, '\0', DEFAULT_BUFLEN);
	// recieve vehicle id from user
	if((read_size = recv(c_sock, car_id, CAR_ID_LEN, 0)) == -1) {
		puts("Error geting vehicle ID from client.");
	} 

	puts(car_id);

	// scan reserved.txt and if car id is already reserved give user error
	// else reserve car for that user
	while(fscanf(fp, "%s", file_line) != EOF){	
		parse_reserved(file_line, id, user_name);

		if(!strcmp(car_id, id)){
			printf("Vehicle %s already reserved by: %s\n", id, user_name);
			reserved = 1;
			if(send(sock, V_ALREADY_RSVD, strlen(V_ALREADY_RSVD), 0) < 0) {
				puts("Send failed");
				return;
			}
			break;
		}
	}
	if(feof(fp)) puts("EOF");

	fclose(fp);

	if(!reserved){
		FILE *fp_w = fopen("reserved.txt", "a");

		memset(us_id, '\0', DEFAULT_BUFLEN);
		make_us_car(us_id, username, car_id);
		us_id[strlen(us_id)] = '\n';
		
		fputs(us_id, fp_w);

		if(send(sock, V_RSVD, strlen(V_RSVD), 0) < 0) {
			puts("Send failed");
			return;
		}
		printf("Vehicle %s reserved by: %s\n", car_id, username);
		fclose(fp_w);
	}

}

// pareses reserved cars from file_line in two separated buffers: id and user_name
int parse_reserved(const char file_line[], char id[], char user_name[]){
	int i;
	int j = 0;
	int k = 0;
	for(i = 0; i < strlen(file_line); i++){
		if(file_line[i] == '|'){
			user_name[i] = '\0';
			break;
		}
		user_name[i] = file_line[i];
		j++;
	}
	for(i = j + 1; i < strlen(file_line); i++){
		id[k] = file_line[i];
		k++;
		if(file_line[i] == '\n'){
            id[k] = '\0';
		}		
	}
	id[CAR_ID_LEN] = '\0';

	return 1;
}

// makes username|id combo 
void make_us_car(char us_id[], const char user_name[], const char id[]){
	strcat(us_id, user_name);
	strcat(us_id, "|");
	strcat(us_id, id);
}

// Search by parameters opens cars.txt in read mode and tries to find
// specific vehicles with parameters user asked for
void search(const int c_sock, const int sock, char parameters[]){
	int read_size = 0;
	int param_no = 0;
	int cnt = 0;
	char f_line[DEFAULT_BUFLEN];
	FILE *fp = fopen("cars.txt", "r");

	memset(parameters, '\0', DEFAULT_BUFLEN_P);

	// recieve parameters for search from client in form
	// id|manufacturer|carname|year
	if((read_size = recv(c_sock, parameters, DEFAULT_BUFLEN_P, 0)) == -1) {
		puts("Error geting search parameters from client.");
	}

	puts(parameters);

	param_no = params_no(parameters);
	printf("Number of search parameters: %d\n", param_no);

	get_line_params(parameters);

	memset(f_line, '\0', DEFAULT_BUFLEN);
	while(fgets(f_line, sizeof(f_line), fp) != NULL){
		get_f_line_params(f_line);

		// Compare client parameters with f_line parameters
		if( (!strcmp(id, f_id)) || (!strcmp(manufacturer, f_manufacturer)) || (!strcmp(carname, f_carname)) || (!cmp_y(year, f_year)) ){
			printf("%s", f_line);
			cnt++;
			if(send(sock, f_line, strlen(f_line), 0) < 0) {
				puts("Send failed");
				return;
			}
		}
	}
	if(feof(fp)) puts("EOF");

	if(!cnt){
		if(send(sock, SEARCH_e, strlen(SEARCH_e), 0) < 0) {
			puts("Send failed");
			return;
		}
	}

	fclose(fp);
}

// from f_line take out id, manufacturer, carname and year 
// and put them in global buffers id, manufacturer, carname and year
void get_f_line_params(const char f_line[]){
	int i;
	int j = 0;
	int k = 0;
	int l = 0;
	int m = 0;


	// clearing memory blocks: global buffers f_id, f_manufacturer, f_carname and f_year	
	memset(f_id, '\0', CAR_ID_LEN);
	memset(f_manufacturer, '\0', DEFAULT_BUFLEN);
	memset(f_carname, '\0', DEFAULT_BUFLEN);
	memset(f_year, '\0', YEAR);

	for(i = 0; i < strlen(f_line); i++, j++){
		if(f_line[i] == '|'){
			f_id[i] = '\0';
			break;
		}
		f_id[i] = f_line[i];
	}
	for(i = j + 1; i < strlen(f_line); i++, k++){
		if(f_line[i] == '|'){
            f_manufacturer[k] = '\0';
			j = i;
			break;
		}		
		f_manufacturer[k] = f_line[i];
	}
	for(i = j + 1; i < strlen(f_line); i++){
		if(f_line[i] == '|'){
			f_carname[l] = '\0';
			j = i;
			break;
		}
		f_carname[l] = f_line[i];
		l++;
	}
	for(i = j + 1; i < strlen(f_line); i++, m++){
		if(f_line[i] == '\n'){
			f_year[m] = '\0';		
			break;
		}
		f_year[m] = f_line[i];		
	}

	f_year[YEAR] = '\0';

}

// same as get_f_line_params
void get_line_params(const char params[]){
	int i;
	int j = 0;
	int k = 0;
	int l = 0;
	int m = 0;

	// clearing memory blocks: global buffers id, manufacturer, carname and year	
	memset(id, '\0', CAR_ID_LEN);
	memset(manufacturer, '\0', DEFAULT_BUFLEN);
	memset(carname, '\0', DEFAULT_BUFLEN);
	memset(year, '\0', YEAR);

	for(i = 0; i < strlen(params); i++, j++){
		if(params[i] == '|'){
			id[i] = '\0';
			break;
		}
		id[i] = params[i];
	}
	for(i = j + 1; i < strlen(params); i++, k++){
		if(params[i] == '|'){
            manufacturer[k] = '\0';
			j = i;
			break;
		}		
		manufacturer[k] = params[i];
	}
	for(i = j + 1; i < strlen(params); i++, l++){
		if(params[i] == '|'){
			carname[l] = '\0';
			j = i;
			break;
		}
		carname[l] = params[i];
	}
	for(i = j + 1; i < strlen(params); i++, m++){
		year[m] = params[i];
	}

	year[YEAR] = '\0';

	printf("Looking for: \n");
	printf("ID: %s\n", id);
	printf("Manufacturer: %s\n", manufacturer);
	printf("Carname: %s\n", carname);
	printf("Year: %s\n", year);

}

// compare f_year and year (file_line year and user parameter year)
int cmp_y(char f_year[], char year[]){
	int i;
	int cnt = 0;

	for(i = 0; i <= YEAR; i++){
		if(f_year[i] == year[i]) cnt++;
	}

	if(cnt == 4) return 0;
	else return 1;

}

// return how many parameters client sent to server for search by parameters
// count '-' in params string
int params_no(const char params[]){
	int i;
	int retVal = 4;

	for(i = 0; i < strlen(params); i++){
		if(params[i] == '-' && (params[i+1] == '|' || params[i-1] == '|')) retVal--;
	}

	return retVal;
}
