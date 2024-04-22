#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>


#define BUFSZ 500

void usage (int argc, char **argv){
    printf("usage: %s <v4|v6> <server port>\n", argv[0]);
    printf("example: %s v4 12345\n", argv[0]);
    exit(EXIT_FAILURE);
}

void addrtostr(const struct sockaddr *addr, char *str, size_t strsize){
    int version;
    char addrstr[INET6_ADDRSTRLEN+1] = "";
    uint16_t port;

    if(addr->sa_family == AF_INET){
        version = 4;
        struct sockaddr_in *addr4 = (struct sockaddr_in *)addr;
        if (!inet_ntop(AF_INET, &(addr4->sin_addr), addrstr, INET6_ADDRSTRLEN+1)){
            perror("ntop");
            exit(1);
        }
        port = ntohs(addr4->sin_port);
    }else if(addr->sa_family == AF_INET6){
        version = 6;
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)addr;
        if (!inet_ntop(AF_INET, &(addr6->sin6_addr), addrstr, INET6_ADDRSTRLEN+1)){
            perror("ntop");
            exit(1);
        }
        port = ntohs(addr6->sin6_port);
    }else {
        perror("erro protocol family");
        exit(1);
    }
    if(str){
        snprintf(str, strsize, "IPv%d %s %hu", version, addrstr, port);
    }
}

int server_sockaddr_init(const char *proto, const char *portstr, struct sockaddr_storage *storage){
    uint16_t port = (uint16_t)atoi(portstr);
    if (port == 0){
        return -1;
    }
    port = htons(port);

    memset(storage, 0, sizeof(*storage));
    if (0 == strcmp(proto, "v4")){
        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
        addr4->sin_family = AF_INET;
        addr4->sin_addr.s_addr = INADDR_ANY;
        addr4->sin_port = port;
        return 0;
    }else if (0 == strcmp(proto, "v6")) {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_addr = in6addr_any;
        addr6->sin6_port = port;
        return 0;
    }else {
        return -1;
    }
}

//criando um struct para manipular os comandos de tempeturatura, umidade e ventiladores
typedef struct {
    int id;
    int temp, um_ar, v1, v2, v3, v4;
} Sala;


int main(int argc, char **argv) {
    if (argc < 3){
        usage(argc, argv);
    }
    

    //criando a variavel salas do tipo sala, indo de 0 a 7
    Sala salas[7];

    //inicializando o dados de cada sala com -1
    for (int i = 0; i < 7; i++) {
    salas[i].id = -1;
    salas[i].temp = -1;
    salas[i].um_ar = -1;
    salas[i].v1 = -1;
    salas[i].v2 = -1;
    salas[i].v3 = -1;
    salas[i].v4 = -1;
    }

    struct sockaddr_storage storage;
    if (0 != server_sockaddr_init(argv[1], argv[2], &storage)){
        usage(argc, argv);
    }

    //criando o socket
    int serverTP1 = socket(storage.ss_family, SOCK_STREAM, 0);
    if (serverTP1 == -1){
        perror("socket falhou\n");
        exit(1);
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);

    //vinculando o socket para o ip e porta especificas
    if (0 != bind(serverTP1, addr, sizeof(storage)))
    {
        perror("bind");
        exit(1);
    }

    if (0 != listen(serverTP1, 10)){
        perror("listen");
        exit(1);
    }
    
    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);
    printf("Vinculado em %s\n", addrstr);
    printf("Aguardando Conexões...\n");

    //loop para permitir que o servidor não caia
    while(1){

        struct sockaddr_storage cstorage;
        struct sockaddr *caddr = (struct sockaddr *)(&cstorage);
        socklen_t caddrlen = sizeof(cstorage);
        //conectar ao cliente
        int clientTP1 = accept(serverTP1, caddr, &caddrlen);
        if (clientTP1 == -1){
            perror("accept");
            exit(1);
        }

        printf("conexão estabelecida\n");

        char buff[BUFSZ];
        char resposta[BUFSZ];
        //loop para o tratamento dos clientes
        while (1){
            //criando um variavel para validar o comando do cliente
            int le_msg = read(clientTP1, buff, 500); 
            if (le_msg == 0){
                printf("\ncliente desconectado\n");
                printf("Aguardando novas conexões\n");
                break;
            }
            printf("\nComando do cliente: %s", buff);
            fflush(stdout);


            int sala_id;
            ////////////////////////////////////////////////////////////////////////
            //comando para registrar uma sala
            ////////////////////////////////////////////////////////////////////////
            if (strncmp(buff, "CAD_REQ", 7) == 0) { //vendo qual comando o cliente enviou, se for CAD_REQ, ele executa o if
                int sala_id = buff[8] - '0';  //atribuindo o numero da sala instanciada, que no caso é o id da sala no comando "CAD_REQ" no qual eh o 8 caractere
                //verificando se a sala ja existe, ou seja, se o sala_id ja esta instanciada em alguma sala e salas
                if (salas[sala_id].id == sala_id) {
                    strcpy(resposta, "ERROR 02\n");
                }
                //se tudo ocorrer bem, a sala é instanciada
                else {
                    salas[sala_id].id = sala_id; //colocando o id da sala exigido no comando dentro da variavel id de cada sala
                    strcpy(resposta, "OK 01\n");
                }   
            } 


            ////////////////////////////////////////////////////////////////////////
            //comando iniciar sensores
            ////////////////////////////////////////////////////////////////////////
            else if (strncmp(buff, "INI_REQ", 7) == 0) {
                //criando as variaveis instermediarias
                int temp, um_ar, v1, v2, v3, v4;
                //atribuindo as variaveis intermediarias para que estas sejam usadas para atribuir nas variaveis da struct
                sscanf(buff, "INI_REQ %d %d %d %d %d %d %d", &sala_id, &temp, &um_ar, &v1, &v2, &v3, &v4);

                //se o id da sala não tiver sido registrada antes, o servidor manda ERROR 03
                if(!(salas[sala_id].id == sala_id)){
                    strcpy(resposta, "ERROR 03\n");
                }
                //se os sensores da sala ja tiverem sido instanciadas, o servidor manda ERROR 05
                else if(salas[sala_id].temp != -1){
                    strcpy(resposta, "ERROR 05\n");
                }
                //se não ocorrer nenhum erro, o servidor estancia os sensores e envia OK 02
                else{
                    salas[sala_id].id = sala_id;       //atribuindo os valores nas variaveis da sala correspondente
                    salas[sala_id].temp = temp;
                    salas[sala_id].um_ar = um_ar;
                    salas[sala_id].v1 = v1;
                    salas[sala_id].v2 = v2;
                    salas[sala_id].v3 = v3;
                    salas[sala_id].v4 = v4;
                    strcpy(resposta, "OK 02\n");
                }
            ////////////////////////////////////////////////////////////////////////
            //comando shutdown
            ////////////////////////////////////////////////////////////////////////
            }else if (strncmp(buff, "DES_REQ", 7) == 0) {
                int sala_id = buff[8] - '0';
            //verificando se a sala foi instanciada
                if (!(salas[sala_id].id == sala_id)) {
                    strcpy(resposta, "ERROR 03\n");
            //verificando se os sensores ja estão desligados
                }else if (salas[sala_id].temp == -1){
                    strcpy(resposta, "ERROR 06\n");
                }
                //atribuindo os valores -1 nos sensores
                else {
                    salas[sala_id].temp = -1;
                    salas[sala_id].um_ar = -1;
                    salas[sala_id].v1 = -1;
                    salas[sala_id].v2 = -1;
                    salas[sala_id].v3 = -1;
                    salas[sala_id].v4 = -1;

                    strcpy(resposta, "OK 03\n");
                }
            }

            ////////////////////////////////////////////////////////////////////////
            //comando atualizar sensores
            ////////////////////////////////////////////////////////////////////////
            else if (strncmp(buff, "ALT_REQ", 7) == 0) {
            //criando as variaveis intermediarias denovo
                //criando as variaveis instermediarias
                int temp, um_ar, v1, v2, v3, v4;
                //atribuindo as variaveis intermediarias para que estas sejam usadas para atribuir nas variaveis da struct
                sscanf(buff, "ALT_REQ %d %d %d %d %d %d %d", &sala_id, &temp, &um_ar, &v1, &v2, &v3, &v4);

                //se o id da sala não tiver sido registrada antes, o servidor manda ERROR 03
                if(!(salas[sala_id].id == sala_id)){
                    strcpy(resposta, "ERROR 03\n");
                }
                //se os sensores da sala estiverem desligados
                else if(salas[sala_id].temp == -1){
                    strcpy(resposta, "ERROR 06\n");
                }
                //se não ocorrer nenhum erro, o servidor estancia os sensores e envia OK 02
                else{
                    salas[sala_id].id = sala_id;       //atribuindo os valores nas variaveis da sala correspondente
                    salas[sala_id].temp = temp;
                    salas[sala_id].um_ar = um_ar;
                    salas[sala_id].v1 = v1;
                    salas[sala_id].v2 = v2;
                    salas[sala_id].v3 = v3;
                    salas[sala_id].v4 = v4;
                    strcpy(resposta, "OK 04\n");
                }
            }
            ////////////////////////////////////////////////////////////////////////
            //comando pegar informações de uma sala
            ////////////////////////////////////////////////////////////////////////
            else if (strncmp(buff, "SAL_REQ", 7) == 0) {
                int sala_id = buff[8] - '0';

                if(!(salas[sala_id].id == sala_id)) {
                    strcpy(resposta, "ERROR 03\n");
                }
                //verifica se os sensores estão instalados
                else if(salas[sala_id].temp == -1){
                    strcpy(resposta, "ERROR 06\n");
                }
                else{  
                    snprintf(resposta, sizeof(resposta), "SAL_RES sala %d: %d %d %d %d %d %d", sala_id, salas[sala_id].temp, salas[sala_id].um_ar, salas[sala_id].v1, salas[sala_id].v2, salas[sala_id].v3, salas[sala_id].v4);
                }
            }

            ////////////////////////////////////////////////////////////////////////
            //comando pegar informações de todas as salas
            ////////////////////////////////////////////////////////////////////////
            else if (strncmp(buff, "INF_REQ", 7) == 0) {
                //alocando dinamicamente a memoria
                int existem_salas = 0;
                char *resposta_final = malloc(500);
                if (resposta_final == NULL) {
                    perror("Erro ao alocar memória");
                    exit(1);
                }
                strcpy(resposta_final, "INF_RES salas: "); // Inicializar a string com o prefixo

                for (int i = 0; i < 7; i++) {
                    //verificando se a sala foi registrada
                    if (salas[i].id != -1) {
                        //char para alocar os dados de cada sala
                        char dados_sala[100];
                        sprintf(dados_sala, "%d (%d %d %d %d %d %d) ", 
                                salas[i].id, 
                                salas[i].temp, 
                                salas[i].um_ar, 
                                salas[i].v1, 
                                salas[i].v2, 
                                salas[i].v3, 
                                salas[i].v4);
                        strcat(resposta_final, dados_sala);
                        existem_salas = 1;
                    }
                }
                if (existem_salas == 0){
                    strcpy(resposta, "ERROR 03\n");
                }else {
                    strcat(resposta, resposta_final);
                }      

            } else if (strncmp(buff, "kill", 4) == 0) {
                printf("conexão encerrada\n");
                close(clientTP1);
                close(serverTP1);
                exit(0);
            }
            printf("\nresposta do servidor: %s\n", resposta);
            send(clientTP1, resposta, strlen(resposta), 0);
            memset(buff, 0, sizeof(buff));
            memset(resposta, 0, sizeof(resposta));
            
        }
        //fechando o socket cliente
        close(clientTP1);
    } 

    //fechando o sockets
    close(serverTP1);
    printf("\nconexão encerrada\n");
    return 0;
}