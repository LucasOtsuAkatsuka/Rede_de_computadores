#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>

void usage (int argc, char **argv){
    printf("usage: %s <serverIP> <server port>", argv[0]);
    printf("example: %s 127.0.0.1 12345", argv[0]);
    exit(EXIT_FAILURE);
}

int addrparse(const char *addrstr, const char *portstr, struct sockaddr_storage *storage){
    if(addrstr == NULL || portstr == NULL){
        return -1;
    }

    uint16_t port = (uint16_t)atoi(portstr);
    if (port == 0){
        return -1;
    }
    port = htons(port);

    struct in_addr inaddr4;
    if (inet_pton(AF_INET, addrstr, &inaddr4)){
        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
        addr4->sin_family = AF_INET;
        addr4->sin_port = port;
        addr4->sin_addr = inaddr4;
        return 0;
    }

    struct in6_addr inaddr6;
    if (inet_pton(AF_INET6, addrstr, &inaddr6)){
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = port;
        //addr6->sin6_addr = inaddr6;
        memcpy(&(addr6->sin6_addr), &inaddr6, sizeof(inaddr6));
        return 0;
    }
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

#define BUFSZ 500

int main(int argc, char **argv) {
    if (argc < 3){
        usage(argc, argv);
    }

    struct sockaddr_storage storage;
    if (0 != addrparse(argv[1], argv[2], &storage)){
        usage(argc, argv);
    }
    int clientTP1 = socket(storage.ss_family,SOCK_STREAM,0);
    if (clientTP1 == -1){
        perror("socket falhou\n");
        exit(1);
    }
    struct sockaddr *addr = (struct sockaddr *)(&storage);
    //conectando ao servidor
    if(0 != connect (clientTP1, addr, sizeof(storage))){
        perror("Conexão falhou\n");
        exit(1);
    }

    printf("Conectado ao servidor\n");
    
    //enviando comandos ao servidor
    char buff[500];
    char comando_cliente[500];
    
    char comando[500];
    while (1) {
        printf("Digite um comando: ");
        fgets(comando_cliente, sizeof(comando_cliente), stdin);
        //removendo caractere "\n"
        comando_cliente[strcspn(comando_cliente, "\n")] = 0;

        int id_real;
        ///////////////////////////////////////////////////////////////////////////
        // comando register
        ///////////////////////////////////////////////////////////////////////////
        if (strncmp(comando_cliente, "register", 8) == 0) {
            //atribui a id o numero da sala id, ou seja, o caractere 9, so atribui um numero
            int id = comando_cliente[9] - '0';
            //atribui o numero que esta depois da palavra register em id_real, pode ser mais de um numero
            sscanf(comando_cliente, "register %d", &id_real);
            // Verifica se o ID da sala é válido
            if (id_real < 0 || id_real > 7) {
                printf("sala inválida\n");
                continue;
            } else {
                // Constrói o comando como "CAD_REQ + id" ficando "CAD_REQ id"
                snprintf(comando, sizeof(comando), "CAD_REQ %d", id);
            }

        ///////////////////////////////////////////////////////////////////////////
        // comando init info
        ///////////////////////////////////////////////////////////////////////////
        }else if ((strncmp(comando_cliente, "init info", 9) == 0)){
            //variaveis intermediarias para atribuir o valor de cada dado em cada variavel correspondente
            int temp, um_ar, v1, v2, v3, v4;
            //atribuindo as variaveis intermediarias para que estas sejam usadas para atribuir nas variaveis da struct
            sscanf(comando_cliente, "init info %d %d %d %d %d %d %d", &id_real, &temp, &um_ar, &v1, &v2, &v3, &v4);
            //atribuindo o id da sala que é o decimo caractere do comando, so atribui um numero
            int id = comando_cliente[10] - '0';
            if (id_real < 0 || id_real > 7) {
                printf("sala inválida\n");
                continue;
            }else if (v4<40 || v4 >42){
                printf("sensores inválidos\n");
                continue;
            //verificando se os sensores são validos
            }else if (temp < 0 || temp > 40 || um_ar<0 || um_ar>100 || (v1 != 10 && v1 != 11 && v1 != 12) || (v2 != 20 && v2 != 21 && v2 != 22) || (v3 != 30 && v3 != 31 && v3 != 32) || (v4 != 40 && v4 != 41 && v4 != 42)){
                printf("sensores inválidos\n");
                continue;
            //convetendo o comando para o codigo, para enviar para o servidor
            }else {
                snprintf(comando, sizeof(comando), "INI_REQ %d %d %d %d %d %d %d", id, temp, um_ar, v1, v2, v3, v4);
            }
        ///////////////////////////////////////////////////////////////////////////
        // comando init file
        ///////////////////////////////////////////////////////////////////////////
        }else if ((strncmp(comando_cliente, "init file", 9) == 0)){
            char filename[50]; //char que ira armazenar o contudo do arquivo
            int temp, um_ar, v1, v2, v3, v4; //variaveis intermediarias
            FILE *file_ptr;

            sscanf(comando_cliente, "init file %s", filename); // extrai o nome do arquivo do comando, assim fica possivel chamar diferentes arquivos com diferentes nomes, caso o servidor tiver acesso a eles
            
            //abre o arquivo
            file_ptr = fopen(filename, "r");

            //verifica se o programa encontrou o arquivo
            if (file_ptr == NULL) {
                printf("sala inválida\n");
                continue;
            }else {
                char line[100];
                int id;
                //salva os dados do arquivo nas respectivas variaveis intermediarias
                //sistema de loop pois cada dado fica em uma linha
                for (int i = 0; i < 7; i++) {
                    if (fgets(line, sizeof(line), file_ptr) != NULL) {
                        sscanf(line, "%d", &id);
                    }

                    if (fgets(line, sizeof(line), file_ptr) != NULL) {
                        sscanf(line, "%d", &temp);
                    }
                    if (fgets(line, sizeof(line), file_ptr) != NULL) {
                        sscanf(line, "%d", &um_ar);
                    }

                    if (fgets(line, sizeof(line), file_ptr) != NULL) {
                        sscanf(line, "%d", &v1);
                    }

                    if (fgets(line, sizeof(line), file_ptr) != NULL) {
                        sscanf(line, "%d", &v2);
                    }

                    if (fgets(line, sizeof(line), file_ptr) != NULL) {
                        sscanf(line, "%d", &v3);
                    }

                    if (fgets(line, sizeof(line), file_ptr) != NULL) {
                        sscanf(line, "%d", &v4);
                    }
                }               
                fclose(file_ptr);
                //verifica se o id da sala é valido
                if (id_real < 0 || id_real > 7) {
                    printf("sala inválida\n");
                    continue;
                }else if (v4<40 || v4 >42){
                printf("sensores inválidos\n");
                continue;    
                    //verificando se os sensores são validos
                }else if (temp < 0 || temp > 40 || um_ar<0 || um_ar>100 || (v1 != 10 && v1 != 11 && v1 != 12) || (v2 != 20 && v2 != 21 && v2 != 22) || (v3 != 30 && v3 != 31 && v3 != 32) || (v4 != 40 && v4 != 41 && v4 != 42)){
                    printf("sensores inválidos\n");
                    continue;
                //convetendo o comando para o codigo, para enviar para o servidor
                }else {
                    snprintf(comando, sizeof(comando), "INI_REQ %d %d %d %d %d %d %d", id, temp, um_ar, v1, v2, v3, v4);
                }   
            }
        ///////////////////////////////////////////////////////////////////////////
        // comando shutdown
        ///////////////////////////////////////////////////////////////////////////
        }else if ((strncmp(comando_cliente, "shutdown", 8) == 0)){
            //seta os ids tanto id_real quando id, ja explicados anteriormente
            sscanf(comando_cliente, "shutdown %d", &id_real);
            int id = comando_cliente[9] - '0';
            if (id_real < 0 || id_real > 7) {
                printf("sala inválida\n");
                continue;
            }else {
                snprintf(comando, sizeof(comando), "DES_REQ %d", id);
            }
        }
        ///////////////////////////////////////////////////////////////////////////
        // comando update info
        ///////////////////////////////////////////////////////////////////////////
        else if ((strncmp(comando_cliente, "update info", 11) == 0)){
            int temp, um_ar, v1, v2, v3, v4;
            //atribuindo as variaveis intermediarias para que estas sejam usadas para atribuir nas variaveis da struct
            sscanf(comando_cliente, "update info %d %d %d %d %d %d %d", &id_real, &temp, &um_ar, &v1, &v2, &v3, &v4);
            //atribuindo o id da sala que é o decimo caractere do comando, so atribui um numero
            int id = comando_cliente[12] - '0';
            if (id_real < 0 || id_real > 7) {
                printf("sala inválida\n");
                continue;
            }else if (v4<40 || v4 >42){
                printf("sensores inválidos\n");
                continue; 
            //verificando se os sensores são validos
            }else if (temp < 0 || temp > 40 || um_ar<0 || um_ar>100 || (v1 != 10 && v1 != 11 && v1 != 12) || (v2 != 20 && v2 != 21 && v2 != 22) || (v3 != 30 && v3 != 31 && v3 != 32) || (v4 != 40 && v4 != 41 && v4 != 42)){
                printf("sensores inválidos\n");
                continue;
            //convetendo o comando para o codigo, para enviar para o servidor
            }else {
                snprintf(comando, sizeof(comando), "ALT_REQ %d %d %d %d %d %d %d", id, temp, um_ar, v1, v2, v3, v4);
            }
        }
        ///////////////////////////////////////////////////////////////////////////
        // comando update file
        ///////////////////////////////////////////////////////////////////////////
        else if ((strncmp(comando_cliente, "update file", 11) == 0)){
            char filename[50]; //char que ira armazenar o contudo do arquivo
            int temp, um_ar, v1, v2, v3, v4; //variaveis intermediarias
            FILE *file_ptr;

            sscanf(comando_cliente, "update file %s", filename); // extrai o nome do arquivo do comando, assim fica possivel chamar diferentes arquivos com diferentes nomes, caso o servidor tiver acesso a eles
            
            //abre o arquivo
            file_ptr = fopen(filename, "r");

            //verifica se o programa encontrou o arquivo
            if (file_ptr == NULL) {
                printf("sala inválida\n");
                continue;
            } else {
                char line[100];
                //salva os dados do arquivo nas respectivas variaveis intermediarias
                //sistema de loop pois cada dado fica em uma linha
                for (int i = 0; i < 7; i++) {
                    if (fgets(line, sizeof(line), file_ptr) != NULL) {
                        sscanf(line, "%d", &id_real);
                    }

                    if (fgets(line, sizeof(line), file_ptr) != NULL) {
                        sscanf(line, "%d", &temp);
                    }
                    if (fgets(line, sizeof(line), file_ptr) != NULL) {
                        sscanf(line, "%d", &um_ar);
                    }

                    if (fgets(line, sizeof(line), file_ptr) != NULL) {
                        sscanf(line, "%d", &v1);
                    }

                    if (fgets(line, sizeof(line), file_ptr) != NULL) {
                        sscanf(line, "%d", &v2);
                    }

                    if (fgets(line, sizeof(line), file_ptr) != NULL) {
                        sscanf(line, "%d", &v3);
                    }

                    if (fgets(line, sizeof(line), file_ptr) != NULL) {
                        sscanf(line, "%d", &v4);
                    }
                }               
                    fclose(file_ptr);
                    //verifica se o id da sala é valido
                    if (id_real < 0 || id_real > 7) {
                        printf("sala inválida\n");
                        continue;
                    //verificando se os sensores são validos
                    }else if (v4<40 || v4 >42){
                        printf("sensores inválidos\n");
                        continue;
                    }else if (temp < 0 || temp > 40 || um_ar<0 || um_ar>100 || (v1 != 10 && v1 != 11 && v1 != 12) || (v2 != 20 && v2 != 21 && v2 != 22) || (v3 != 30 && v3 != 31 && v3 != 32) || (v4 != 40 && v4 != 41 && v4 != 42)){
                        printf("sensores inválidos\n");
                        continue;
                    //convetendo o comando para o codigo, para enviar para o servidor
                    }else {
                        snprintf(comando, sizeof(comando), "ALT_REQ %d %d %d %d %d %d %d", id_real, temp, um_ar, v1, v2, v3, v4);
                    }  
            }
        }
        ///////////////////////////////////////////////////////////////////////////
        // comando load info
        ///////////////////////////////////////////////////////////////////////////
        else if ((strncmp(comando_cliente, "load info", 9) == 0)){
            //instanciando os ids
            int id = comando_cliente[10] - '0';
            sscanf(comando_cliente, "load info %d", &id_real);
            //verificando se a sala é valida
            if (id_real < 0 || id_real > 7) {
                printf("sala inválida\n");
                continue;
            //convetendo o comando para o codigo, para enviar para o servidor
            }else {
                snprintf(comando, sizeof(comando), "SAL_REQ %d", id);
            }
        }
        ///////////////////////////////////////////////////////////////////////////
        // comando load rooms
        ///////////////////////////////////////////////////////////////////////////
        else if ((strncmp(comando_cliente, "load rooms", 10) == 0)){
             snprintf(comando, sizeof(comando), "INF_REQ");
        }else if ((strncmp(comando_cliente, "kill", 4) == 0)){
            snprintf(comando, sizeof(comando), "kill");
            send(clientTP1, comando, strlen(comando), 0);
            break;
        }
        else{
            break;
        }


        
        
        // Envia o comando ao servidor
        send(clientTP1, comando, strlen(comando), 0);

        //atribuindo a resposta do servidor em buff
        recv(clientTP1, buff, sizeof(buff), 0);

        //lidando com os erros
        if ((strncmp(buff, "ERROR", 5) == 0)){
            int id_erro = buff[7] - '0';
            if (id_erro == 2){
                printf("sala já existe\n");
            }else if (id_erro == 3){
                printf("sala inexistente\n");
            }else if (id_erro == 5){
                printf("sensores já instalados\n");
            }else if (id_erro == 6){
                printf("sensores não instalados\n");
            }
        //lidando com as confirmações
        }else if ((strncmp(buff, "OK", 2) == 0))
        {
            int id_confirmacao = buff[4] - '0';
            if (id_confirmacao == 1){
                printf("sala instanciada com sucesso\n");
            }
            else if (id_confirmacao == 2){
                printf("sensores inicializados com sucesso\n");
            }else if(id_confirmacao == 3){
                printf("sensores desligados com sucesso\n");
            }else if(id_confirmacao == 4){
                printf("informações atualizadas com sucesso\n");
            }
        //recendo e imprimindo os dados da sala requisitada
        }else if ((strncmp(buff, "SAL_RES", 7) == 0)){
            //criando uma variavel para armazenar os dados que vao ser impressos
            char dados[BUFSZ];
            // Encontra o primeiro espaço após "INF_RES"
            char *space_ptr = strchr(buff, ' '); 
            if (space_ptr != NULL) {
                // Copia todos os dados após o espaço em branco
                strcpy(dados, space_ptr + 1); 
                printf("%s\n", dados);
            }
        //recebendo e imprimindo os dados de todas as salas
        }else if ((strncmp(buff, "INF_RES", 7) == 0)){
            //criando uma variavel para armazenar os dados que vao ser impressos
            char dados[BUFSZ];
            // Encontra o primeiro espaço após "INF_RES"
            char *space_ptr = strchr(buff, ' ');
            if (space_ptr != NULL) {
                // Copia todos os dados após o espaço em branco
                strcpy(dados, space_ptr + 1);
                printf("%s\n", dados);
            }
        }
        

        memset(buff, 0, sizeof(buff));
        memset(comando, 0, sizeof(comando));
    }

    // fecha o socket 
    close(clientTP1);
    printf("Conexão encerrada\n");

    return 0;
}